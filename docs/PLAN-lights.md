# PLAN-lights: lights placed by hand, the generator's shown, new hanging lights; the torches at three left

The owner (25 November 2026): "I'd like the ability to place each themed ceiling light, being able to pick from each style. We might
require a few additional gothic styles to our hanging lights." And a new rule: "When a player has only three enemies to eliminate on
the level we cause all torches to pulse a little bit, or shift their color to one in our palette." Asked, they settled:

1. **Pulse and recolour**, both, at three left.
2. **Four new hanging lights** for a sheet: an **iron corona** (a great wheel of wrought iron, spikes and candles, on four chains),
   a **sanctuary lamp** (one lamp of red glass on three fine chains, a small steady glow), a **bone candelabrum** (antlers, ribs and
   skulls bound into a chandelier, candles in the eye sockets), a **pierced cage lantern** (a tall lantern of pierced iron throwing
   dappled light). The sheet goes to the owner first; nothing goes in the game before it is approved.
3. **The generator's lights shown in the editor**, to be moved, removed or replaced: "Show me placements that the generator will use so
   I can move them. Allow me to remove, move, or replace them as I see fit." A drawing that carries lights of its own gets only those.
4. The world editor's rolls of creatures and of gold are rolled over the whole level (docs/PLAN-mapedit.md).

## 1. The format the game reads (the editor's export; built to, not the other way round)

```c
enum { DL_CHANDELIER, DL_LANTERN, DL_CANDLESTAND, DL_BRAZIER, DL_MINELAMP, DL_CRYSTALS, DL_FIREPIT, DL_MAGMA, DL_TOTEM,
       DL_CORONA, DL_SANCTUARY, DL_BONES, DL_CAGE };           /* the last four: the new hanging lights, each its own since
                                                                  step 4 (till then a chandelier or a lantern stood in) */
typedef struct { int style, x, y; } DrawnLight;                 /* its cell in the drawing; hanging ones hang over its middle,
                                                                  standing ones stand in it as the generator's do */
/* Drawn's row, after the hazards: light, nlight. None: the generator's own, as now. Any: those alone, and none of the
   generator's light props (chandelier, lantern, candlestand, brazier, mine lamp, crystals, fire pit, magma, totem). */
```

Each style is laid as `gen_level.c` lays it (`chandelier`, `lantern`, `candlestand`, `brazier`, `minelamp`, `crystals`, `fire_pit`,
`magma`, `totem`): the same sprite, frames, sway and light. The torches on the walls are the events' (docs/PLAN-events.md), not these.

**The generator's lights, listed.** The game lays a drawing as it would (the drawing's seed, its dressing) and lists the light props it
hung or stood: `int crawl_drawn_lights(DrawnLight *out, int max)` after `crawl_drawn_text`, the browser's door `web_drawn_lights` (a
static array: the count, then style, x, y for each, in the drawing's cells), and the harness's `build/host_debug drawnlights <export>`
printing them. The editor loads the game (`play/haunted-core.js`) in its own page to ask, and gives the owner what comes back as
lights of their own, to keep, move or remove.

## 2. Three left

On any level, generated or drawn, when the creatures still to kill (`G.kills_total - G.kills`) fall to three or fewer and above none,
every torch (the walls' `T_FEAT + F1_TORCH` frames, the events' torches, and the light props above) takes a colour from the palette
(cold and pale, chosen on a sheet of frames for the owner) and its light breathes slowly, brighter and dimmer, a little. It is drawing
only: nothing in the simulation, the saves or the dream's numbers may move, and the dream's log must match the build before, line for
line (CLAUDE.md). If a creature's senses read the light (`minds.c`), the pulse is applied where the frame is drawn, not to what they read.

## 3. Steps

| | Step | Who |
|---|---|---|
| 1 | The editor: layers; the rolls; the lights' tools, styles and the generator's lights asked of the game | the editor's track |
| 2 | The sheet: the four hanging lights (and the barred door, relic, socket, lever and plate owed from docs/PLAN-events.md) | an agent; **stop for the owner** |
| 3 | The game: `DrawnLight`, the reader, laying them, the listing door; three left | an agent |
| 4 | The new lights in the game, once the sheet is approved | after step 2 |

## 4. Hand-off log

**Step 2: the sheets, for the owner (25 November 2026).** `make build/lightmock`; `build/lightmock lights
media/sheets/lights/hanging_lights.png` and `build/lightmock things media/sheets/events/event_things.png`. The tool #includes
`crawl_art.c` whole, so the new drawings use its canvas, candles, chains, skulls and bones as a part does and can move into
`src/art/parts/` as they stand (`lm_keep` where the game's would be `cv_store`). Nothing went in the game: no sprite, texture or
slot was added, and the game's build is untouched.

*The lights,* each beside today's chandelier and lantern for scale, then in each run (levels 3, 13, 23, 33): every frame twice its
size in its own light, the first darker, and at the game's own size in a crop of its view, hung two and a half cells off. The
**iron corona**: a wheel 58 pixels across on four chains, two bands with a zigzag of bars between, eight barbed spikes and eight
candles on prickets, six spokes to a boss and a long pointed drop; three frames. The **sanctuary lamp**: a brass rose and smoke
bell, three fine chains, a gadrooned brass cup holding a beaker of red glass with a small flame seen through it; two frames, barely
different, as it should burn steady. The **bone candelabrum**: a great skull (the hand-drawn one), a stag's antlers bound to its
temples, a small skull hung on cord under each antler's end, ribs round a spine below, a jaw at the foot; a stub of candle in each
eye of each skull, the flame rising from the socket over the brow; three frames. The **pierced cage lantern**: tall, of black iron,
a crocketed spire, a lancet over a quatrefoil in front and a lattice of holes round the sides, the light coming through them; three
frames. The runs: the Catacombs' water by `prop_wash` (rust and drips, as the chandelier gets), the Hollows' soot (iron darker,
brass tarnished), the Deep's black candles (as its altar's) and a violet glint in its iron; bone and stone follow each level's
palette as everything does. The cage's view throws its holes' light in spots on the walls and floor, to show what "dappled" might
mean; the game has no such light yet.

*Left for the owner:* whether the four read as meant; whether the corona is far enough from the chandelier (both are rings of
candles; the corona is wider, barbed and spoked, with no skull); whether the sanctuary lamp's glow, red glass in brass, is the
colour wanted (its light's colour is step 4's); whether the Deep's violet glint is welcome or noise; whether the cage's dappled
light is wanted, which would be new work in the light code.

**The owner on the sheets (25 November 2026).** The corona is different enough from the chandelier; the sanctuary lamp's red is right;
the Deep's violet glints are welcome; the cage lantern's **dappled light is wanted** (new work in the light code: a light that throws
spots through the cage's holes on the walls and floor; a step of its own once the sheet is approved); one detail pass more over both
sheets "to reduce the simple shapes", then back to the owner.

**The detail pass (25 November 2026), back to the owner.** `tools/lightmock.c`, both sheets drawn again, each drawing looked at
twice its size and at 1x before and after. The corona: the outer band a twisted bar, its turns lit by turns, both bands hammered
unevenly, a collar where each spoke meets the band, a ring of rivets on the boss. The sanctuary lamp: the rose in five lobes; the
smoke bell ringed with a moulding, pierced for the smoke, beaded at its rim, tarnished; the cup's gadroons each turned to the
light, a bead under its lip, verdigris in the seams; the knop banded, the drop an acorn. The cage lantern: the spire laid in
fish-scale plates, the eave and the ribs riveted, the bands doubled, the faces hammered and turned a little across, soot over each
hole and the plate's edge lit under it, the cone ringed. The bone candelabrum was left as it was: bone, cord and flame already.
Nothing in the game.

**The sheets approved (25 November 2026).** After the detail pass the owner approved both, `media/sheets/lights/hanging_lights.png` and
`media/sheets/events/event_things.png`: "Beautifully done!" Next: the four lights and the five things into the game's art from
`tools/lightmock.c` (as the unlit torch went), the unbarred look shown on an unlock, then the cage lantern's dappled light.

**Step 3, the game's side (25 November 2026; its own worktree branch, for the work branch).** Built to section 1 as written; nothing
in the format changed. Harness only: nobody has played it, nothing ran under Wine, the Windows and browser builds were not compiled here
(no MinGW or Emscripten in the session; `clang -fsyntax-only` on `scene_crawl.c` with and without the debug switches, and on
`web_main.c` against a stub `emscripten.h`, shows no new warning).

*The data and the reader.* `DL_*` and `DrawnLight` are in `generate.c`, beside `g_drawn`, so that the generator can note its own light
props by style as it lays them; `Drawn` ends `light, nlight`. `drawn_text.c` reads `LIGHT` (`{ DL_..., x, y }`, at most 96), the thirteen
names, and a row of 33 values (the hazards' four, `NULL, 0, NULL, 0` when there are none, then the lights and their count). It refuses,
by name: a style it has not, a cell off the drawing, a count in the row that is not what was read, lights the row does not name, a row a
value short. `drawncheck` compares the lights. The editor's `fixtures/lights.c` reads as written.

*Laying.* Each light prop (`chandelier` ... `totem`) notes itself as `dress_level` lays it (`GLP`: its style, cell, picture and light; the
dice untouched). A drawing with lights of its own has the generator's taken off again once the dressing is done (`glp_strip`: the picture
gone, the cell what it was, the light and its share of the light map out of the list), so the rest of the dressing lies as it would have
with them; then, after the drawing's things are cleared of the dressing, its own are laid by the same functions, footprints and all, and
the dice they took given back, so what follows on the drawing's seed (its spawn points) lies as it would. The four new ones are a
chandelier (corona, sanctuary lamp, bone candelabrum) or a lantern (the cage) till their art is in. The debug build says
`DRAWN level n: the generator's k light props taken off` and `DRAWN level n: k lights`.

*The listing.* `crawl_drawn_lights(out, max)` lays the drawing read as `start_level` would for its level (its seed, its dressing, its
torches and events' fields), without its own lights, and returns the generator's light props that keep their picture, as `DL_*` and the
drawing's cells, in the order laid; -1 if nothing was read. Everything the laying writes (`G`, the plan, lanes, fire, ice, `EV`, `DH`, the
faces' and the dressing's counts) is copied aside first and put back after, byte for byte: about 3 MB, most of it `G`. It needs nothing set
up: no art, no settings, no `app_init`. The harness's `build/host_debug drawnlights <export>` prints it (`DL_CHANDELIER 14 3`); the
browser's `web_drawn_lights()` returns a static array, the count (-1: nothing read, or no memory for the copy) then style, x, y. **What the
editor's page calls, in order:** load `play/haunted-core.js` (`HauntedCore()`), nothing else first (no `web_config`, no `web_start`); put
the export's bytes at `_web_drawn_buf()` (fewer than `_web_drawn_cap()`); `_web_drawn_load(len)`, which answers `""` when read, else why
not (`UTF8ToString`); then `p = _web_drawn_lights()` and read `new Int32Array(M.HEAPU8.buffer, p, 1 + 3 * n)` (the count at `p` first;
`HEAP32` is not among the exported runtime methods). Loading a drawing in that module makes it its level's drawing there, as PLAY's does.

*Three left.* With one to three creatures still to kill (`G.kills_total - G.kills`), outside a pocket, her rooms and the tally, every torch
(the walls' `T_TORCH0` and `T_FEAT + F1_TORCH` frames, the events' torches lit, guttering and snuffed) and every light prop takes the cold
colour, and its light breathes: one breath in two seconds, a light's share up to 18% brighter and dimmer, the flame a step up or down its
ramp at either end of the breath. The colour is laid over a pixel already shaded (`tl_lut`), so a cold flame is as bright as the fire was,
whatever ramp it is; fog still covers it. The dead read the light (`G.lit_here` in `minds.c`, a Shade's showing, the spiders and the
hidden message's sight), so the breath is never in the light map that plays: `build_lightmap` notes each light's share (`lm_f`), and
`crawl_render` adds the breath for the frame drawn and puts the map back byte for byte after the hand (`tl_begin`, `tl_end`). A light
knows it is a torch's (`Light.torch`, never saved).

*The colour.* `media/sheets/lights/three_left.png`: each run from two places (a wall torch and a light prop; the Deep's crystals and its
magma), the usual flame beside four candidates at the same moment, and the breath. Chosen for now: **A, the ghost ramp** (`R_GHOST`, from
its sixth step up), a pale grave-light green-white. It reads as a sign rather than a weaker fire, which the bone ramp (C) does; the glow
ramp (B) is the frost's and the crystals' own blue, so in the Deep it hides among them; violet (D) sinks into the Deep's amethyst walls and
is the spirit's colour. `CRAWL_THREECOL=k` shows the others; `TL_DEFAULT` in `floor.c` changes it. **The owner's to pick.** The magma
floor and the Deep's fire seam go cold with the rest, as the plan's list has them; the crystals have no flame and keep their colour, only
their light breathes.

*Read where the plan left room.* The listing is what the generator would lay, whether or not the drawing has lights of its own, so the
editor can offer them again. A thing the run's dice pick into a room is taken where drawn. A generator light whose picture the drawing's
clearing took off (on a lane, or where a thing is set down) is not listed: its light stays there with no lamp, as it did before this
work. Only the plan's nine are the generator's light props: the floor candles, the altars' candles, the saints' and the arena's lights
stay. A standing light of the drawing's takes the place of any dressing in its cell; on a lane or where a thing is set down it is not
solid, so it never shuts a way; one in the stone is not laid. A hanging one hangs over anything but stone. Debug switches:
`CRAWL_THREELEFT`, `CRAWL_THREECOL`, `CRAWL_LIGHTLIST`, `CRAWL_LIGHTLISTAT`, `CRAWL_TPPROP=-2` (docs/HARNESS.md).

*The proof.* (a) The dream, `apl crawl 3000`, `CRAWL_RUNID=7`, seeds 1, 3, 5, 7, 11, 13, 15 and 17, against `2534937` built in a worktree:
line for line the same but the two `PORTRAITS ... kept in` lines each, 7 to 9 levels a run, the tutorial's two among them. (b) A hand
export of every style (the hazards' fixture with 15 lights: one on a lane, one in the stone) and the same lights on the events' sample:
the 5 generator's light props taken off, 14 laid (one not solid, one refused by the stone); frames from six places beside the fixture
without lights show the generator's chandeliers gone and the drawing's chandelier, lantern, candlestand, brazier, corona and magma where
drawn. Pasted into a scratch copy as `DRAWN[2]` to `[5]` with `fixtures/lights.c` and `hazards.c`: "the same, field for field" each, and
"the lights" against the wrong one. `drawnlights` on `hazards.c`, `lights.c` and the sample: the same five each time (four chandeliers,
a fire pit), and on the tutorial's level 1 export four (chandeliers at (10,17), (12,25), (8,27), the fire pit at (0,23)), each the same on
two runs, the same as `CRAWL_LIGHTLIST` prints as the levels are laid in play, and where the frames stand before them. `CRAWL_LIGHTLISTAT`
asking for the listing in the middle of a run on the drawing (seed 3, 400 s): its log the same as without but the asking. (c) `make asan`,
`ASAN_OPTIONS=detect_leaks=0`, 600 s, seeds 1 and 4: `lights.c`, the hand export and `hazards.c`, each finishing level 3 (4 or 5 reached),
and the hand export with `CRAWL_THREELEFT=1` on every frame: no report, and that last log the same as without it but the time-taken line.
`drawnlights` and `drawncheck` on `lights.c` cut every 5 bytes and at each of its last 1,500 (2,296 cuts), and on the hand export the
same with its last 2,000 (2,923): every one refused or read, none crashed; nine broken texts refused by name. `make smoke` passes. (d)
Saves untouched: `save.c` and `ledger.c` did not change; the lights, `GLP` and `Light.torch` are the level's and never saved.

*Left:* the owner's pick of the colour, and their hand on all of it; the four new lights' art (step 4); the editor's page to call
`web_drawn_lights` (above); the browser's and Windows builds compiled by CI.

**The owner's colour for three left (25 November 2026): A, the ghost ramp** (`R_GHOST`), the default already (`TL_DEFAULT`). And: "go
ahead with the art": step 4 (the four lights and the events' things into the game), the unbarred look on an unlock, then the cage
lantern's dappled light.

**Step 4, the art in the game, the unbarred look and the dappled light (25 November 2026; its own worktree branch, for the work
branch).** Harness only: nobody has played it, nothing ran under Wine, the Windows and browser builds were not compiled here (no MinGW or
Emscripten in the session; `clang -fsyntax-only` on `scene_crawl.c` and `crawl_art.c`, with and without the debug switches, shows no
warning).

*The art.* The drawings moved from `tools/lightmock.c` into `src/art/parts/lights_events.c` as they stood (`lm_keep` became
`cv_store`, the Catacombs' wash `prop_wash` on the slot itself), listed in `crawl_art.c` after the unlit torch and made each level by
`crawl_art_lights` once the run's door is made, in the run's look as the sheets have them. New ids only at the ends of their lists:
sprites `SP_CORONA0`-`2`, `SP_SANCT0`-`1`, `SP_BONELAB0`-`2`, `SP_CAGE0`-`2`, `SP_EV_RELIC`, `SP_EV_SOCKET0` (empty) and `1` (holding),
`SP_EV_LEVER0` (up) and `1` (pulled) (NSPR 681 to 697); textures `T_PLATE`, `T_PLATE_DOWN` (the plate as an overlay, C_CLEAR where the
cell's own floor shows round it) and `T_BARRED`, `T_UNBARRED` (the run's door, `T_RDOOR`, barred and unbarred) (NTEX 119 to 123, under
the byte `Cell.fover` holds). `build/lightmock` draws both sheets from the game's slots now, and both are byte for byte the approved
PNGs (`cmp`); `build/torchmock`'s sheet too. Every existing sprite, texture, the palette, colormap, glints, eyes and every existing
sprite's foot (`crawl_foot`) were hashed after `crawl_art_level` for each of the levels 1 to 40, against `37c0fac` built in a worktree:
the same. The new feet, in texels: corona 6, sanctuary lamp 5, candelabrum 11, cage 8 (14 in the Catacombs, its drips), relic 34, socket
30, lever 28. None is laid as a prop, so none is read.

*In the game.* `DL_CORONA`, `DL_SANCTUARY`, `DL_BONES` and `DL_CAGE` hang their own (`corona`, `sanctuary`, `bonelabrum`,
`cage_lantern` in `gen_level.c`, beside `chandelier`), swaying as the chandelier does, noted for three left and the listing as the rest.
The three with candles change frame at four a second (`DF_SLOW`; three frames would otherwise run at the fire's nine); the sanctuary
lamp's two at the chandelier's three. Their light: the corona 1.45 over 6.5 cells (the chandelier's 1.35 over 6), the candelabrum 1.25
over 5.6, the cage 1.5 over 5.4 (the lantern's 1.1 over 5: its spots take light away between them), the sanctuary lamp a small 0.65
over 3.4. The light map holds no colour, so the lamp's red is laid on as the frame is drawn, as the dapple is (below). The events' things
take their looks: the relic, the socket (holding when a relic is set in it, the relic then drawn as part of it and not over it), the
lever up and pulled, the plate (an overlay on the cell's floor, pressed while stood on). `B` and a door an event shuts (`g_gate`) show
`T_BARRED` while barred and `T_UNBARRED` from the moment an event unlocks or opens it; the tutorial's gate and the puzzles' gates keep
the portcullis, and the generated levels' things keep theirs.

*The dappled light.* A light now has a look, as it is drawn (`Light.look`: `LK_DAPPLE` the cage's, `LK_RED` the sanctuary lamp's;
the level's, never saved). Each frame `lk_begin` gathers each such light's share of the light map, as `build_lightmap` gave it (and the
breath at three left), into a map of its own, and `lk_end` clears it after; the light map itself is never touched, so `G.lit_here`
and all the dead sense are as they were. A floor or wall pixel the cage's share reaches has that share thrown through the holes: spots
where a hole looks out, the iron's shade between, by the pixel's bearing and height from the lantern (`lk_spots`: fourteen holes about
and a dozen bands up its height, the cone shading what is straight under it and the eave what is straight over); the ceiling is left
plain. A pixel the red glass lights is tinged red (each colour's nearest red-cast colour in the level's palette, dithered as far as the
glass's share of its light, at most 80%). `CRAWL_LIGHTLOOK=0` draws both plain; `CRAWL_THREELEFT=-1` holds three left off for frames.
*The sheet:* `media/sheets/lights/cage_dapple.png` (`python3 -I tools/dapple_sheet.py`: the editor's `lights.c` fixture read, with two
cage lanterns hung in its south room, in each run's level 3, 13, 23 and 33; by a wall and over the floor, without and with, at 1x and
the lantern's corner three times over). Looked at: the spots read plainly on the walls in every run, the Deep's amethyst and the Hollows'
timber least; on the floor as a fan of spokes and rings under the lantern, quieter, best seen near. **For the owner:** whether the spots
are wanted so strong on the walls, and the floor's as they are; the sanctuary lamp's red is a light warm cast round it, not more.

*Read where the brief left room.* A barred door unlocked shows unbarred and stays shut till walked into or opened; once open it stays open
as before. The plate also stays down while a switch or timed trigger it set off still holds (`ev_plate_held`, drawing only): a plate
is never seen while stood on, so without it the pressed look would never show; a plate that fires once rises when stepped off. The
event socket's holding look is its own sprite, so the relic in it is not drawn twice. The cage's spots fall on the floor and walls, not
on the dead or the props standing in them, and not on the ceiling. Light powers above are mine; the owner's to change.

*The proof.* (a) The dream, `apl crawl 3000`, `CRAWL_RUNID=7`, seeds 1, 3, 5, 7, 11, 13, 15 and 17, against `37c0fac` built in a
worktree: line for line the same but the two `PORTRAITS ... kept in` lines each; 8 to 11 levels a run, the tutorial's two among them.
(b) `tools/mapedit/fixtures/lights.c` (its corona) and an export of my own (not committed: the fixture with all four new lights, two
more cages, a lever unbarring a second barred door with `TA_DOOR_UNLOCK`, a timed plate, two more lit torches by the doors), laid as
level 3 and as level 33. Frames from the harness, each looked at: every new light in both runs; the cage by a wall and over a floor, with
and without; the relic; the lever up, then pulled and the north door unbarred before it is opened; the socket empty, then holding,
then the east door sliding open unbarred on `TA_DOOR_OPEN`; the plate up, pressed while its trigger holds, up again. The dream on my
export, 500 s: level 3, seeds 1, 3 and 5, every trigger in order (plate, compass, lever, socket, south room) and level 4 reached; level
33, the triggers in the same order each life but a new run's caster dies to the dead there, four to six times, exactly as the build
before does on the same export with its stand-ins (`37c0fac`: the same deaths, the same triggers); with `CRAWL_GEAR=4` it clears 33 on all
three seeds, every death after on 34. The fixture: seeds 1 and 3, its three triggers, level 4, no death. Footprints on the drawn levels
the same as with the stand-ins (`FOOT level 33: 24 whole ...`, seeds 1, 3, 5); the 33 log against the stand-ins differs by one creature
losing the caster 0.3 s later (the new lights' powers). (c) `make asan`, `ASAN_OPTIONS=detect_leaks=0`, 600 s, seeds 1 and 4: my export as level 3 and as 33, and the fixture: no
report (`apl` draws a frame twice a second, so the looks were drawn wherever the dream went); and every scripted frame above through
`build/host_asan`: no report. (d) Saves untouched: `save.c` and `ledger.c` did not change;
`Light.look`, the deco flag and the looks' maps are the level's or the frame's and never saved. `make smoke` passes; its 60 s bench 1.74 to 1.92 ms a frame over four runs against
1.77 to 1.95 for `37c0fac`, the same within the noise. On my export, where three cages and the lamp hang, 2.04 to 2.13 ms with the
looks and 1.92 to 1.95 without (the debug build).

*Left:* the owner's look at the dapple's sheet and at the four lights and the things in play, and their hand on all of it; the light
powers and the red; twelve relic looks (PLAN-events); the browser's and Windows builds compiled by CI.

**The owner on the dapple and the lights (25 November 2026):** "Looks good!" The cage lantern's dappled light, the new lights' powers and
the sanctuary lamp's red tinge stand as built. Still harness only: on main (a76f4d2), not released, not played by hand.
