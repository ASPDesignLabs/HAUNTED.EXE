# Tidying the repository: plan and hand-off

Status (13 November 2026): **settled with the owner the same day. Steps 0 to 3 are done (the baseline; the pictures out of
`docs/`; `src/` sorted by kind; object files); steps 4 and 5 wait for the owner's word on
their lists of cuts (section 5a), and step 6 for their word on its outline (section 5b).** Nothing here changes the game: no version is
bumped, nothing is released, and main is left alone until the owner says.

Read the hand-off log (section 9) first. It says where the last session stopped.

## 1. What the owner asked for

13 November, in their words: "I would like your help with getting the codebase for HAUNTED to be a bit less.. of a mess.
Right now our code and images are intermingled, fine as this is a C project but it causes some conflict when we are
building due to context issues. Lets make a plan to attack this problem then backup and start our work."

## 2. What was found

Measured on main at 1.0.0b21.5 (commit `a38b19c`), in a container with four cores and gcc 13.3.

| Where | Measured | What it costs |
|---|---|---|
| `src/scene_crawl.c` | 2.2 MB, 35,484 lines, 1,681 functions, one translation unit | The generator, the creatures and their minds, the autopilot, the saves, every menu and all the drawing (its last 7,700 lines) are one file. No session can read it whole, so every session works by search and by slices, and any two sessions edit the same file (PLAN-combat needed its whole section 17 to let two agents share it). |
| `src/crawl_art.c` | 1.0 MB, 15,174 lines | All the procedural art in one file. |
| Bitmaps as C in `src/` | `face_img.c`, `hand_img.c`, `cat_img.c`, `font_vga.c`: 350 KB, lines up to 644 characters | A search that lands in them floods the context. |
| The build | `make host` 71 s; `scene_crawl.c` 35 s of it and `crawl_art.c` 20 s | The Makefile keeps no object files, so a one-line change anywhere compiles everything again, in every configuration (the harness, its debug build, the ASan build, each of the four tools). |
| `docs/` | 80 MB: 29 text files beside 289 pictures (`docs/uipass/` alone is 53 MB) | The win64 release zip is 73.7 MB for a game of 2.9 MB, because `tools/package.py` packs all of `docs/`. |
| `CLAUDE.md` | 245 KB, about 60,000 tokens: 46 KB of release history, 148 KB of *How CATACOMB works*, 27 KB of known issues, 15 KB of switches | It is loaded whole into every session and every helper it starts, before any work begins. The largest fixed cost of all. |

Two things that help:

- **Nothing is in flight.** No pull request is open, and no other branch holds work newer than main (the newest,
  `claude/sweet-carson-qxvyfi`, is main's own tree). So nothing collides with files that move.
- **A file cut into parts that it `#include`s compiles to the same object.** Tried in a scratch copy before the plan was
  offered: `scene_crawl.c` cut in two at its *Rendering* banner gave objects alike byte for byte, release and debug.
  Nothing in `src/` uses `__LINE__`, `__FILE__` or `assert`, which is why.

## 3. The owner's answers

Asked the same day, each with a recommendation:

- **Scope: all four parts.** Pictures out of `docs/`; `src/` sorted and the build made quicker; the two great files cut
  up; `CLAUDE.md` slimmed.
- **`src/` sorted by kind** (section 6 has the folders), not left flat.
- **No extra backup.** Main and the tag `v1.0.0b21.5` both stand at `a38b19c` on GitHub and this work touches neither;
  they are the way back. A backup branch and an offline bundle were offered and declined.
- **Confirm only the big steps.** I stop for the owner's word before cutting the two great files (step 4) and before
  slimming `CLAUDE.md` (step 6), and run the others through with a report after each.

## 4. The rule, and how a step is proved

**Nothing of the game moves.** What is compiled stays what it was; only where the text lives changes. So the proof is not
"the runs look about the same" but "the compiler's output is the same file":

- `sh tools/tidy_check.sh` builds this tree and the tree at `a38b19c` side by side (the old one in a worktree under
  `build/tidy/`, measured once and kept) and compares every object of the harness and the tools, release and debug: 46
  objects, each to be alike byte for byte. It finds a file by its name wherever it lives, so it holds across a move and
  across a cut.
- `sh tools/tidy_check.sh all` goes on to what the Makefile builds on Linux: the seven binaries, the smoke test's frames,
  the 84 sheets, the items and the balance sheet, the assembled site's 98 files, and two fixed runs of the dream (from
  level 1 and from level 21, twenty minutes of its time each), line for line.
- For a cut (steps 4 and 5): the parts joined in their order must be the old file, byte for byte, before anything else
  is looked at.
- For step 6: a script must find every line of the old `CLAUDE.md` in exactly one of the new files.
- The Windows and browser builds can't be made in the work container (it has neither MinGW nor Emscripten): CI builds
  and smoke-tests both on the branch, by a run started by hand, at step 7.

The harness repeats itself exactly on a fixed `CRAWL_SEED` and `CRAWL_RUNID`, but for one line: `PORTRAITS the runs'
stone kept in N ms` prints a time taken. The check leaves out lines of that shape.

Each step is one commit on `claude/peaceful-ritchie-fmeoal`, pushed as it is done, so any step can be taken back alone.

## 5. The steps

0. **The baseline.** Done: see the log.
1. **Pictures out of `docs/`.** They go to a new `media/` (section 6), so `docs/` holds text alone. Every reference is
   rewritten in the same commit (the README, the plans, `CLAUDE.md`, the tools, a comment in the code) and checked by a
   script: every picture named anywhere must exist. The assembled site must come out the same, file for file. From the
   next release the zip carries `docs/` and `media/previews/` (the README's own pictures), and leaves the approval
   sheets and the gallery out.
2. **`src/` sorted by kind.** Moves alone, with `git mv` so history follows. The Makefile gives the compiler the folders
   (`-I`), so a file still says `#include "crawl.h"`; the tools drop their `../src/`; `scene_crawl.c` asks for
   `build/version.h` as `win_main.c` already does.
3. **Object files.** The Makefile compiles each file to an object under `build/obj/<configuration>/`, with the
   compiler's own lists of what each depends on, in parallel. The targets keep their names (`build/host`,
   `build/host_debug`, `make sheets` and the rest), so every script and every command in the plans still works. A
   target for the ASan build, so nobody assembles that command by hand again. The Windows and browser builds stay
   single commands, since they can't be tried here.
4. **`scene_crawl.c` cut into parts** along its own banners: the game's parts in `src/crawl/game/`, the drawing in
   `src/crawl/draw/`. It stays one translation unit; `scene_crawl.c` becomes the list of its parts, in order. **Waits
   for the owner's word**, given the list of cuts (each part's name, its lines, what it holds).
5. **`crawl_art.c` cut the same way**, into `src/art/parts/`.
6. **`CLAUDE.md` slimmed** to a map of about 15 KB: what the project is, working with the owner, where things are, how
   to build and test, how to release, and where to read about each system. The release history, *How CATACOMB works*,
   the debug switches and the known issues move word for word into files under `docs/`, read when wanted. **Waits for
   the owner's word**, given the outline.
7. **The last checks.** ASan sessions, CI's Windows and browser builds on the branch, the README's notes on building,
   and a report. Merging to main waits for the owner's word; with `VERSION` as it is, a push to main releases nothing
   and the page is rebuilt the same.

## 5a. The cuts proposed (steps 4 and 5): waiting for the owner's word

Drawn up on 13 November from the two files as they stand, and not yet made. The files' own banners turned out not to
mark where the subject changes (under *the soul-lamps' ward* come the Wraith, the buried, the wall skull and three
bosses), so the cuts follow the sequence of functions instead, and there are more of them than the "about 35 and 20"
the plan first said: **70 parts for `scene_crawl.c`** (137 to 1,273 lines, half of them under 480) and **31 for
`crawl_art.c`** (97 to 902 lines). Every cut falls where no brace, no `#if` and no comment is open; a script checked
each. The longest parts are long because one function is: `render_sprites` is 750 lines, `update_mob` 648.

How a part looks, and how the cut is proved:

- A part is a `.c` file that is never compiled on its own. It begins with three lines of its own (a comment naming it,
  the file it is a part of, and what it holds; then a blank line) and is otherwise the old file's text, untouched.
- `scene_crawl.c` keeps its opening comment and its `#include`s of headers (lines 1 to 29), then lists its parts in
  order, one `#include "game/defs.c"` a line, each with a few words beside it. `crawl_art.c` keeps lines 1 to 13 (its
  comment, its headers and the art's five arrays) and does the same. So each is the map of itself.
- A part sees everything in the parts above it, as now. Nothing is reordered, renamed or made visible outside the file.
- Proved three ways: the parts, less their three lines, joined in the list's order are the old file from its first cut
  to its end, byte for byte; `sh tools/tidy_check.sh all` finds every object and binary alike; and the compiler's own
  dependency lists then name every part, so changing one compiles its file again and nothing else.
- Names in `game/` and `draw/` sometimes answer one another (`game/menus.c` is what the menus do, `draw/menus.c` how
  they look), and likewise `game/hazards.c` and `src/art/parts/hazards.c`.

### `src/crawl/scene_crawl.c`: the game in `game/`, the drawing in `draw/`

| Part | Lines of the old file | Lines | What it holds |
|---|---|---|---|
| `src/crawl/game/defs.c` | 30-380 | 351 | Sizes and paces; the kinds of cell, creature, item, missile and harm; cells, doors, lights; the creature (`Mob`); the contracts between combat's two tracks; packs and ambushes; `KINDS` and the ranks' tables |
| `src/crawl/game/state.c` | 381-1280 | 900 | The bosses' arenas and `BOSSES`; hazards, things, puzzles and trials as structs; shots, secrets, items; the levels' names; the record, a stone, the Alignment's lines; `RUN`; the console's and the ledger's state; `G` |
| `src/crawl/game/input.c` | 1281-1488 | 208 | `IN` and the pad (`PADX`), touch's and the wheels' state, the pad's layout (`PL`) and its saving, rumble, the hints in the pad's words (`pk`) |
| `src/crawl/game/attacks.c` | 1489-1647 | 159 | The pockets' state (`PK`, `G_LEVEL`); a creature's blow by level; `ATTACKS`; footprints; what a cell blocks (light, a body, a step) |
| `src/crawl/game/generate.c` | 1648-2219 | 572 | Level generation: carving, themes and room types, decorations and static lights; the plan (maze, vaults, widths); secrets placed; the sliding wall's box |
| `src/crawl/game/faces.c` | 2220-2473 | 254 | A drawn run's walls by face: materials by the stretch, blends, what stands on the wall (`lay_faces`) |
| `src/crawl/game/gen_level.c` | 2474-3011 | 538 | `gen_level` itself (with `con_carve` and `gen_dress`), then the dressing: props and their footprints, crystals, fires, litter (`dress_level`) |
| `src/crawl/game/place.c` | 3012-3605 | 594 | What a level is given: the arena stamped, treasure, keys and the exit, and the creatures that wait in their places (wall skulls, spiders, the buried, imps, gargoyles, guardians) |
| `src/crawl/game/light.c` | 3606-3776 | 171 | The light map: dynamic lights, the torch's flicker, `build_lightmap` |
| `src/crawl/game/spawn.c` | 3777-4146 | 370 | Lines of sight, ranks picked, `spawn_mob`, what comes on a level (`random_type`), wanderers, the file and the roost placed, the readout's pop-up |
| `src/crawl/game/harm.c` | 4147-4708 | 562 | Harm that reaches the caster: its one road (`el_resist`, `armour_keep`), the caster's statuses (af_*), `hurt_ex`, the dead's blows and bones (`foe_hit`, `throw_to`, `update_bones`); then souls: the vessel, motes, spirit spent |
| `src/crawl/game/record.c` | 4709-5213 | 505 | The record and its achievements, the portraits' part of it, experience, Levels of Atonement |
| `src/crawl/game/caster.c` | 5214-5890 | 677 | The caster's own: the Unholy Confessions, the Alignment (align_*, `caster_sync`), confessing, the classes' sheets and caps |
| `src/crawl/game/augments.c` | 5891-6452 | 562 | The spells' augments: the other five lines' (augd_*), then the frame and Fire's (aug_*, fire_*) |
| `src/crawl/game/damage.c` | 6453-7415 | 963 | Harm that reaches the dead: `kill_mob`, weaknesses and strengths, elemental creatures, `damage_mob`, the dead's statuses and their gate (`mob_gate`), trails, venoms, blasts; the spectre; the hunt field (`build_hunt`, `alert_near`) |
| `src/crawl/game/minds.c` | 7416-8272 | 857 | How the dead think: senses and memory, packs (calls, turns, flanks, morale), roles and rounds, hunters on the trail, the dead's ways named and their deeds |
| `src/crawl/game/ambush.c` | 8273-8587 | 315 | Lures and ambushes: kill rooms, teeth, bait, the spring |
| `src/crawl/game/noise.c` | 8588-8878 | 291 | Noise and who hears it, alarms, the Sexton's bell, searching, `mind_tick`, `mind_goal`, `mind_frame`, footsteps |
| `src/crawl/game/creatures.c` | 8879-9594 | 716 | Each kind's own ways: the Wraith (and the wards' field it walks by), the buried, pools, wall skulls, spiders and leaps, sacs, shades, gargoyles; then the bosses' own: the knight, the spider, the Wraith Unbound |
| `src/crawl/game/cunning.c` | 9595-9933 | 339 | Cunning by rank (dodges, cover, doorways held, feints), and the new attacks (the Bone Thrower, the charge, the spew, the grapple) |
| `src/crawl/game/update_mob.c` | 9934-10581 | 648 | `update_mob`: one creature's step, every kind's branch |
| `src/crawl/game/spells.c` | 10582-11277 | 696 | The caster's spells: shots, lightning, the BFS9000, the flask, `cast_spell`; charging and the ultimates; `update_shots` |
| `src/crawl/game/runes.c` | 11278-11539 | 262 | The caster's defences: runes, each waiting for its moment (`def_trigger`, `cast_defense`), `update_status` |
| `src/crawl/game/abilities.c` | 11540-12118 | 579 | The balance sheet (`crawl_combat_sheet`); a creature's blow; the bat's latch; the caster's boss abilities (web, Grave Call and allies, Wraithwalk, Swift Dodge); runes raised by hand; doors (`update_doors`) |
| `src/crawl/game/goals.c` | 12119-12532 | 414 | The dream's goals: secrets pushed, keys, the altar, chains, a boss, motes; places to mend |
| `src/crawl/game/dream.c` | 12533-13350 | 818 | The dream (the autopilot): its counters, `pick_goal`, decide, its steps and turns, `find_target`, `choose_spell`, aim, `update_player` |
| `src/crawl/game/budget.c` | 13351-13712 | 362 | A level's creatures placed: the caster's state reset, the debug map, the Black Altar's place, the old placements, bands, and the encounter budget (`place_creatures`) |
| `src/crawl/game/marked.c` | 13713-14690 | 978 | Things that want a spell: their kinds, where they go, what a strike does, sequences and gates, things for fights; the lull's watch |
| `src/crawl/game/drawn.c` | 14691-15145 | 455 | Drawn levels (the tutorial's two): the maps, `gen_drawn`, their items, dead and marks; the burning floor; the ice |
| `src/crawl/game/start_level.c` | 15146-15393 | 248 | `start_level`: a level begun; the debug build's gear and gifts |
| `src/crawl/game/pockets.c` | 15394-15917 | 524 | Pockets: the caster carried across, `gen_pocket`, the way in and back; `crawl_enter`; collect (what picking a thing up does) |
| `src/crawl/game/hazards.c` | 15918-16608 | 691 | Hazards: the bosses' six, the runs' own by element, what they mark for the dream, their harm to the caster and to the dead, `update_hazards`, `af_tick` |
| `src/crawl/game/sites.c` | 16609-17053 | 445 | The fixed hazards: where they stand (`place_sites`), `update_sites`, the arising ones, their lights |
| `src/crawl/game/things.c` | 17054-17241 | 188 | Ways to touch the world: things carried, set down and used (`thing_use`) |
| `src/crawl/game/puzzles.c` | 17242-17969 | 728 | Puzzles: which a level has, where the pieces go, solved and wrong, the shapes they take; the Crypts' (`pz_make_run1`); `pz_make`, `pz_touched`; the dream at a puzzle |
| `src/crawl/game/puzzles_runs.c` | 17970-18972 | 1003 | The Catacombs' puzzles, the Hollows', the Deep's, and the two of any run |
| `src/crawl/game/message.c` | 18973-19233 | 261 | The hidden message, Tab's documents, the Black Shrine |
| `src/crawl/game/trials.c` | 19234-20017 | 784 | Trials: kinds, waves, `tri_begin`, `tri_run`, the reward, `tri_end`, slow work, where a trial is placed |
| `src/crawl/game/lamps.c` | 20018-20287 | 270 | The soul-lamps: laid with the level, lit by souls, `ward_tick` |
| `src/crawl/game/omens.c` | 20288-20535 | 248 | The dream at a trial (`tri_next`, `tri_goal`); omens; a boss's hazards |
| `src/crawl/game/boss.c` | 20536-20832 | 297 | A boss's fight: fonts, `boss_begin`, waves and adds, `boss_fall`, `update_boss` |
| `src/crawl/game/altar.c` | 20833-20969 | 137 | The Black Altar: its wisp called home, `update_altar`, waking, sending, blessing, anointing; `update_abilities` |
| `src/crawl/game/wisps.c` | 20970-21377 | 408 | The map's noting (`map_note`), items and the tally, the hand raised, wisps (targets, routes, `summon_wisp`, `update_wisps`), the L list's choices, the altar's rows, undead turned |
| `src/crawl/game/update.c` | 21378-21826 | 449 | The debug build's stages (`CRAWL_MEET`, `CRAWL_PACK`, `CRAWL_STAGE`), and `crawl_update`: the frame's order of things |
| `src/crawl/game/merchant.c` | 21827-22259 | 433 | The merchant's rooms: her wares' sprites, the pedestals' arc, the portraits hung, `gen_shop`, the dream's shopping, `begin_shop`, purchases |
| `src/crawl/game/hall.c` | 22260-22796 | 537 | Her hall's things: the Rune Shrine, the Hexed Totem, the Accursed Portal (runs, replays, challenges), the bell's and font's errands, `update_shop` |
| `src/crawl/game/human.c` | 22797-23694 | 898 | A person at the controls: captions, doors, the exit and the reckoning, E (`human_use`), the body against walls, the target looked at; Restoration; the wheels; the sprint, `human_walk`, `open_ui`, `human_cast` |
| `src/crawl/game/lessons.c` | 23695-24106 | 412 | The lessons (`TU`): their moments, the view turned, the page; `update_human` |
| `src/crawl/game/her_rooms.c` | 24107-24707 | 601 | A person in her rooms: the portraits in play, `shop_human`, the full bag's question, the buying; taking over and letting go (`human_begin`, `human_end`); Tab's actions; Esc's question and a new run; the debug build's teleports |
| `src/crawl/game/console.c` | 24708-25037 | 330 | The console |
| `src/crawl/game/controls.c` | 25038-25516 | 479 | The pad into keys (`pad_keys`), touch into keys (`touch_keys`), `crawl_input`, `crawl_control` |
| `src/crawl/game/save.c` | 25517-26144 | 628 | Saves: the gear, the record, the hall, the shrine and the stones packed and unpacked, a page's sections, `run_pack`, `run_unpack`, the progress, the mark, `ledger_first` |
| `src/crawl/game/ledger.c` | 26145-26710 | 566 | The ledger's pages (signing, opening); the title screen's logic (`TI`); death's cost; her claim and the mark; the ledger's and the cornerstones' menus |
| `src/crawl/game/menus.c` | 26711-27255 | 545 | The menus' logic: the Alignment and its gifts and augments, the class, the bell and font, confessing and ascending, her menu, a stone, the L list, the altar, the shrine |
| `src/crawl/game/menus_hall.c` | 27256-27742 | 487 | More menus' logic: the Compendium, the Repository, the Codex, the Totem, the Portal; the map's keys; `update_ui` |
| `src/crawl/draw/floor.c` | 27743-28284 | 542 | Rendering begins: shading and fog, the floor and ceiling (`render_floor`), secrets' cracks, the floor's marks (hazards, omens, wards, ice: `fm_pixel`) |
| `src/crawl/draw/walls.c` | 28285-28497 | 213 | The walls: the sliding secret's ray, `render_walls` |
| `src/crawl/draw/sprites.c` | 28498-29770 | 1273 | Sprites: eyes and vapour, beams, the cat, which sprite a creature shows (`mob_sprite`, `mob_side`), `render_sprites` |
| `src/crawl/draw/hud.c` | 29771-30211 | 441 | Her wares' tags, the portrait's face, glyphs, the status bar, text wrapped, her prompts and cards, the wards' rings |
| `src/crawl/draw/hand.c` | 30212-30650 | 439 | The hand: built and posed, the charge, Restoration's glow, `render_hand`; the latched bat, the vessel's slip, wisps |
| `src/crawl/draw/cards.c` | 30651-31313 | 663 | The carved stone's hooks; cards (the level's, a boss's, her claim), the web, the boss's bar, the trial's glass, the tally, the readout, the statuses, sockets, the sprint, the view's tint, what is carried and read, messages |
| `src/crawl/draw/tab.c` | 31314-31943 | 630 | Tab's and her hall's frames; a piece's card, the full bag's question, the buying, Tab's pages (elements, the caster, documents, the wardrobe), the Black Shrine, Esc's question |
| `src/crawl/draw/screens.c` | 31944-32319 | 376 | The controller's screen (its logic too) and the look; the ledger, her menu, a stone's page |
| `src/crawl/draw/bestiary.c` | 32320-32645 | 326 | Pop-ups (toasts); the bestiary: its logic and its pages |
| `src/crawl/draw/menus.c` | 32646-33602 | 957 | Menus drawn: the L list, the altar, the shrine, the Compendium, the Repository, the Codex, the Totem, the Portal |
| `src/crawl/draw/caster.c` | 33603-34298 | 696 | Experience and Second Wind shown; the target's health; the cornerstones, the Alignment, the bell and font, the class; the Confessions and ascending |
| `src/crawl/draw/map.c` | 34299-34661 | 363 | The map on its scroll |
| `src/crawl/draw/overlays.c` | 34662-34994 | 333 | `render_ui` (which screen is drawn); the wheels; Toughness's cracks; the warded; the things that want a spell |
| `src/crawl/draw/portraits.c` | 34995-35218 | 224 | The portraits of the slain, drawn: what each shows, their overlays, their cards |
| `src/crawl/draw/frame.c` | 35219-35484 | 266 | `crawl_render`: the frame's order; the music's side (`music_frame`); touch's stones (`crawl_stage`); the scene itself |

### `src/art/crawl_art.c`: in `src/art/parts/`

| Part | Lines of the old file | Lines | What it holds |
|---|---|---|---|
| `src/art/parts/palette.c` | 14-110 | 97 | The palette's ramps, the light colormap, and noise (hsh, vnoise, fbm) |
| `src/art/parts/walls.c` | 111-650 | 540 | Wall textures: stone, rubble, brick, moss, the skull wall, niches, banners, torches, doors and locks, gates, the ice, the Portal, fonts, the sign, a secret's passage, the sealed doorway, the Repository |
| `src/art/parts/pockets.c` | 651-910 | 260 | The pockets' walls, floors and ceilings: the Backrooms and Severance; the exit |
| `src/art/parts/canvas.c` | 911-1226 | 316 | The sprite canvas (cv_*), the master skull and its stamps; the caster's hand in motion (`crawl_hand_frame`) |
| `src/art/parts/skeleton.c` | 1227-1917 | 691 | Flame; the skeleton and the shield-bearer; the dressing a rank puts on a creature (kit_*); their backs; the wall skull; the web spider |
| `src/art/parts/spells.c` | 1918-2484 | 567 | Spells and what they leave (bolts, shards, the spear, wisps, the flask, the orb, the lance, rime, motes, burns); the pedestal and the items; the altar's fire; the Rune Shrine and its runes; the Cursed Coin |
| `src/art/parts/hazards.c` | 2485-2731 | 247 | The runs' hazards: what stands in the wall, and their sprites (grit, the censer, votives, snares, spikes, ash) |
| `src/art/parts/puzzles.c` | 2732-3215 | 484 | The puzzles' things; the things a message hides among; the Black Shrine |
| `src/art/parts/documents.c` | 3216-3410 | 195 | A document's page and its ink; the bosses' hazards (rock, rubble, the eruption, the void, the hands); the scrip; the Hexed Totem |
| `src/art/parts/marked.c` | 3411-4096 | 686 | Things that want a spell: run I's and one for each line, the moving and timed ones, sequences and things for fights; `crawl_mark` (the seal and its motes) |
| `src/art/parts/hall.c` | 4097-4486 | 390 | Her hall's things: the Lampstand, the Dead Bell, the Weeping Font, the lectern; the tutorial's rimed seal and its map; the map on sale; Severance's desks and cooler; the caster's double |
| `src/art/parts/first_kinds.c` | 4487-4807 | 321 | The spectre and the bat; fire, the blast, a bone in flight; gems and the key; a chain, the tombstone |
| `src/art/parts/risen.c` | 4808-5275 | 468 | The bestiary begins: rot and soil; the shambler and the Drowned; the shield-bearer's charge, the spew, dust |
| `src/art/parts/hunters.c` | 5276-5912 | 637 | The ghoul, the bone hound, the skeleton knight, the deacon, with their backs and dressing |
| `src/art/parts/bosses.c` | 5913-6257 | 345 | The Ill-Fated Knight and the Black Deacon |
| `src/art/parts/newkinds.c` | 6258-7159 | 902 | The Whipper-In, the Sexton and the Lightless, with their side views; the bestiary's bones |
| `src/art/parts/spiders.c` | 7160-7684 | 525 | The hunting spider; the Wandering Spider; the brood mother, her sacs and spiderlings |
| `src/art/parts/wraith.c` | 7685-8194 | 510 | The Wraith, the shade, the ember imp and its ember, the gargoyle |
| `src/art/parts/dressing.c` | 8195-8425 | 231 | The dressing that comes in as you go down: carcasses, armour stands, timber, mine lamps, carts, fungus, crystal, magma, obsidian, the altar |
| `src/art/parts/sides.c` | 8426-9039 | 614 | Side views: each creature walking in profile; the glints found |
| `src/art/parts/ageing.c` | 9040-9172 | 133 | Ranks' colours (`build_rank_luts`); each level's age: `decay_tex`, `level_palette`; a pocket's theme (`crawl_art_theme`) |
| `src/art/parts/props_bones.c` | 9173-9720 | 548 | Props: bones, litter, heaps and skull stacks; the totem post; candles, prickets, the corona, hearths, braziers, lanterns |
| `src/art/parts/props_graves.c` | 9721-10374 | 654 | Props: graves and what is carved on them; chains, manacles, the hook beam, the gibbet, the bronze bell |
| `src/art/parts/props_bells.c` | 10375-10671 | 297 | The bell puzzles' four bells, run II's dead bell, the soul-lamp |
| `src/art/parts/props_growth.c` | 10672-11314 | 643 | Props: hanging roots, webs and cocoons, ice, the gothic pillar, flowstone, the altar stone, urns, broken arms, the spiked collar; the roost asleep |
| `src/art/parts/treasure.c` | 11315-11953 | 639 | The ambushes' signs; the pool; treasure (coins, the chalice, gems, the crown, the chest), the phial, the soul urn, the ring; `props_level`; footprints measured; `crawl_art_level` |
| `src/art/parts/run_crypts.c` | 11954-12722 | 769 | Each run's own look begins: the course grid; the Crypts' materials, blends, what stands on their walls, floors, ceilings, doors and keys |
| `src/art/parts/run_catacombs.c` | 12723-13437 | 715 | The Catacombs' materials, what stands on their walls, floors and ceilings, doors, keys and treasure |
| `src/art/parts/run_hollows.c` | 13438-13947 | 510 | The Hollows' materials, what stands on their walls, floors and ceilings, doors, keys and treasure |
| `src/art/parts/run_deep.c` | 13948-14499 | 552 | The Deep's materials, what stands on its walls, floors, ceilings, doors, keys and treasure |
| `src/art/parts/runs.c` | 14500-15174 | 675 | Each run's set made (`crypts_run`, `catacombs_run`, `hollows_run`, `deep_run`); the specials on the run's stone; the trials' doors and things; the places to mend; `crawl_art_run`, `crawl_art_stone`, `crawl_art_init` |

## 5b. The outline proposed for `CLAUDE.md` (step 6): waiting for the owner's word

`CLAUDE.md` is read whole by every session before it does anything, and by every helper a session starts. Today that is
252 KB, about 60,000 tokens. Most of it is reference that a given session needs a tenth of. The proposal is to keep in
it only what every session needs, and to move the rest, word for word, into files that are read when wanted.

| New home | What goes there | From today's section | Size |
|---|---|---|---|
| `CLAUDE.md` | What the project is and where it stands; the standing rules; where things are (the map, with the parts); the commands to build and test; **where to read about each system** (a table: the system, its source parts, its reference, its plan); how to release | new writing, from all of them | about 15 KB |
| `docs/HISTORY.md` | The account of each release and plan, from 1.0.0b18.1 back to 1.0.0b4 and on to the tidy-up, as it stands | the opening | 47 KB |
| `docs/HARNESS.md` | The harness's commands and environment, every debug switch, the events it logs, how the long runs are made and read, the sanitizers, Windows under Wine | *Building and testing* | 12 KB |
| `docs/SAVES.md` | The save's format, version by version, and the ledger's sections | *Building and testing* | 5 KB |
| `docs/catacomb/` | *How CATACOMB works*, a file a subject, each bullet whole: `levels.md` (state, the generator, runs and their looks, laying the walls), `creatures.md` (the kinds, ranks, how the dead think), `combat.md` (PLAN-combat's two tracks), `bosses.md`, `hazards.md` (hazards, puzzles, trials, things that want a spell), `dream.md` (the autopilot), `player.md` (a person at the controls, the pad, the wheels, the lessons), `caster.md` (items, spirit, runes, the Alignment, classes, experience, cornerstones), `secrets.md` (secrets, pockets, wisps, the Black Altars), `hall.md` (the merchant, the ledger, death's cost, her claim, the shrine, the Compendium, the Repository, the Codex, the portraits), `drawing.md` (rendering, the art, the map), `music.md` | *How CATACOMB works* | 148 KB, in about twelve files |
| `docs/KNOWN-ISSUES.md` | Known issues and loose ends, with the balance runs' numbers | the last section | 27 KB |

The rules of the move:

- Every paragraph and bullet moves whole and unreworded. A script must find each line of the old file in exactly one
  new file; nothing is dropped, and nothing is said twice.
- The standing rules stay in `CLAUDE.md` itself, gathered where a session will see them, since they are instructions and
  not reference: how the owner works; honest status; British spelling and the house voice; no model identifiers; work
  goes to main as one squashed commit; the console's codes are never written down; release notes say only what a player
  needs; the sanitizers stay clean; the page describes the released build only; read a plan's hand-off log first.
- The slim `CLAUDE.md` is the only new writing. It is shown to the owner with the step's report, to amend.
- Later sessions then pay about 4,000 tokens for it, not 60,000, and read one file of ten to twenty for the system
  they are working on.

## 6. Where things went

Filled in as each step is done. File names never change; only folders do.

### Pictures (step 1)

| Was | Is |
|---|---|
| `docs/preview_*.png` (24: the README's and the page's strips) | `media/previews/` |
| `docs/gallery/` (55: the page's *Then and now*) | `media/gallery/` |
| `docs/artpass/`, `b18-1/`, `combat/`, `minds/`, `plan-b7/`, `play/`, `titlepass/`, `touch/`, `tutorial/`, `uipass/` | `media/sheets/<the same name>/` |
| `docs/portraits/*.png`, `*.gif` | `media/sheets/portraits/` |
| `docs/portraits/ranks.csv`, `bosses.csv` (numbers, not pictures) | stay where they are |

`media/sheets/` holds what each plan's pass made or was given: approval sheets, mock-ups, frames, the owner's level
drawings. `media/sheets/titlepass/README.md`, the mock-ups' own caption, went with its pictures.

### Sources (step 2)

| Was | Is | What it holds |
|---|---|---|
| `src/core.h`, `app.c/h`, `gfx.c/h`, `snd.c/h` | `src/core/` | What both scenes stand on |
| `src/win_main.c`, `web_main.c` | `src/platform/` | The Windows and browser shells (the harness stays `tools/host.c`) |
| `src/scene_haunt.c`, `haunt_art.c/h` | `src/haunt/` | HAUNT.EXE |
| `src/scene_crawl.c`, `crawl.h`, `crawl_gear.c/h`, `crawl_tutor.c/h` | `src/crawl/` | CATACOMB, the game |
| `src/crawl_art.c`, `crawl_title.c/h`, `crawl_portrait.c/h`, `crawl_ui.c/h`, `ui_art.c` | `src/art/` | CATACOMB's art: what only draws |
| `src/face_img.c`, `hand_img.c`, `cat_img.c`, `font_vga.c` | `src/data/` | Traced and generated bitmaps: never read, never searched |

The plans' older logs still name files as `src/<name>`; the name is right and the folder is in this table.

## 7. What is left alone, and why

- **Separate modules.** Cutting a file into parts leaves one translation unit, so `scene_crawl.c` still takes 35 s to
  compile when any of its parts changes. Making the parts into units of their own (a private header for `G`, `RUN` and
  the 1,681 functions' prototypes) would mend that, but the binary would no longer be the same file, and the proof
  would fall back to the dream's runs. A plan of its own, if the owner wants it, once this one has landed.
- **Git's history.** The clone stays as large as it is (71 MB), since the old pictures are in history; taking them out
  means rewriting it, which is not proposed.
- **The plans.** `docs/PLAN-*.md` stay where they are: the code's comments name them 463 times.

## 8. Found on the way

- `tools/package.py` has never packed `web/` or `site/`, so the zip's "full source" can't `make web` or `make cards`.
  Left as it is; the owner's to decide.
- `CLAUDE.md`'s repository map said `scene_crawl.c` was "about 15,300 lines". It is 35,484.
- `web/haunted.js` names `src/web_main.c` in a comment (it is `src/platform/web_main.c` now). Left as it is, so that
  the browser's page stays the same three files; mend it with the next change to that file.
- `docs/PLAN-ui.md` names a `src/font_ui.c` that was never made, and `docs/PLAN-tutorial.md` a `pages.png` that
  became `pages1.png` and `pages2.png`. Both left as written.

## 9. Hand-off log

- **13 November, session 1.** The survey, the plan and the owner's answers (sections 2 to 5). **Step 0 done:** the
  baseline taken from the untouched tree: 46 objects, 7 binaries, 10 smoke frames, 84 sheets, 98 site files, the
  dream's runs; no compiler warnings. `tools/tidy_check.sh` written so the proof outlives the container: it measures
  `a38b19c` again in a worktree whenever its kept numbers are gone.
- **13 November, session 1, step 1 done.** The pictures are out of `docs/`: 290 files moved with `git mv`, every one a
  pure rename (289 pictures and the title mock-ups' caption), into `media/previews/`, `media/gallery/` and
  `media/sheets/<pass>/`. `docs/` went from 80 MB to 1.4 MB: 27 documents and the portraits' two tables of numbers.
  181 mentions rewritten in 20 files (the README's 24 previews, `CLAUDE.md`, fourteen plans, `tools/build_site.sh`,
  `gallery.sh`, `uishots.sh`, `tutorial_read.py`, one comment in `scene_crawl.c`), and the links said from inside
  `docs/` now climb out (`../media/sheets/...`). Checked: every picture and document named anywhere exists, but for one
  mention that was already dangling (`PLAN-tutorial.md` names the lessons' sheet `pages.png`; it has been `pages1.png`
  and `pages2.png` since it was split; left as written). `sh tools/tidy_check.sh all`: all alike, the site's 98 files
  among them. `tools/package.py` now packs `docs/` and `media/previews/` and leaves the sheets and the gallery out: made
  both ways with the same stand-ins for the Windows binaries, the zip went from 70.4 MB and 384 files to 9.1 MB and
  120, so the next release's should be about 12 MB against 1.0.0b21.5's 73.7. The page says nothing of the zip's size,
  and the README's line on what the zip holds is still true.
- **13 November, session 1, step 2 done.** `src/` is sorted: thirty files moved with `git mv` into `core/`, `platform/`,
  `haunt/`, `crawl/`, `art/` and `data/` (section 6), none left in `src/` itself. The Makefile gives the compiler the
  folders (`INC`), keeps `SRC` in the order it was (the order of linking), and takes its headers from `src/*/*.h`. Three
  kinds of line changed in the code, none of them code: `scene_crawl.c` asks for `build/version.h` (it was
  `../build/version.h`); the six tools ask for `crawl.h` and the rest by name (they said `../src/`); four comments
  name a file's new place. `tools/portrait_sheet.py` reads `KINDS` from `src/crawl/scene_crawl.c` and writes the same two
  tables. `CLAUDE.md` (its map and 33 mentions), the README's layout and the title mock-ups' caption say the new
  places. `sh tools/tidy_check.sh all`: all alike. The plans' logs still name 105 files the old way, in sixteen
  documents, as section 6 says; a pass over them would be mechanical, and is the owner's to ask for.
- **13 November, session 1, step 3 done.** The Makefile keeps objects: `build/obj/<rel|debug|asan>/<the file's path>.o`,
  each with the compiler's own list of what it read (`-MMD -MP`), linked in `SRC`'s order; `MAKEFLAGS` asks for as many
  jobs as cores unless `clean` is a goal, and `make -j1` still gives one. Every target keeps its name. New: `make asan`
  (`build/host_asan`), so the sanitizers' command is no longer put together by hand; `make clean` prunes git's record
  of worktrees it removed (`tools/gallery.sh` and `tools/tidy_check.sh` keep theirs under `build/`). The Windows and
  browser rules are untouched but for their lists' paths. Measured here (four cores): all seven binaries from nothing
  43 s, against 226 s for the old Makefile, which compiled all of it again after any change; after a change now,
  `gfx.c` 0.3 s, `crawl_title.c` 4 s, `crawl_art.c` 18 s, `scene_crawl.c` 35 s (release and debug at once), a header
  35 s when `scene_crawl.c` reads it, nothing changed 0.0 s. `sh tools/tidy_check.sh all`: all alike, the seven
  binaries byte for byte. `make asan` 101 s, no warnings; the dream 300 s on it, no reports. **Next: step 4, which
  waits for the owner's word.** For it: the Windows and browser rules name their sources and `$(HDR)`; give them the
  parts too (a wildcard over `src/*/*/*.c`), since those two builds keep no lists of what they read.
  `tools/portrait_sheet.py` reads `KINDS` out of `scene_crawl.c`'s text: point it at the part that holds it.
- **13 November, session 1, the big steps proposed.** The lists of cuts for both files (section 5a: 70 parts and 31,
  every cut checked safe) and the outline for `CLAUDE.md` (section 5b) are written here and put to the owner. Nothing of
  either is done. The lists were resolved by scripts kept in the session's scratchpad (`cuts.py`, `funcs.py`,
  `resolve.py`, with the two specs); if that is gone, section 5a's tables are the specs: each part's first line in the
  old file is its cut.
