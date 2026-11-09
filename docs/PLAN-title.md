# The title screen: plan and hand-off

Status (9 November 2026): **the first mock-ups are done; the refinement pass is under way: steps 1-8 done; the GIFs sent to the owner for a look.** Nothing in the
game is changed yet. Everything here is mock-ups drawn by `tools/titlemock.c`, outside the game.

Read the hand-off log (section 6) first. It says where the last session stopped.

## 1. What the owner asked for

- A title screen that comes up when a person holds H (or a button) to enter the crypts. It uses the 13h method the game
  uses, with animation, lighting and movement. The name, HAUNTED.EXE, is in a gothic, bloody hand. The mood is a buried
  crypt walker. The owner already has the music, so only the visuals are wanted.
- The screen asks the player to press a button to start, then shows their graves (the ledger's pages) to load from.
- 9 November, on the first mock-ups: **"I like all three, they should rotate when the player starts the game."** Each start
  shows the next of A, B and C, so all three are kept.
- Also 9 November: **each one gets a refinement pass**, "a little more detailed and focused with their designs", and **the
  grave screens are to look more weathered and detailed.** In the owner's words: "the graves look very simplistic, made of
  maybe two shapes. They should be a bit more structured than that, with chips, cracks, and destruction denoting a haunted
  gothic look. Same with the markers for plots, they look like they are made from simple shapes."
- The owner's standing preferences (CLAUDE.md, *Working with the owner*): options and questions before big features; honest
  status about what was tried and how; British spelling; the plain, dry voice. They also asked for backups to be encouraged
  and edits to be confirmed. So keep this work in new files, and commit and push each step.

## 2. Where things are

- `tools/titlemock.c`: the mock-up tool, a standalone C program. It is built from the art's sources and the carved stone's,
  and nothing of the game's scene goes into it. It draws each mock-up as the game draws: 320x200, one palette of 256
  six-bit colours, light as a step within a ramp, lightning as a rewrite of the DAC. It writes looping GIFs, four stills of
  each, `overview.png` (one still of each of the six) and `logo.png`.

  ```sh
  gcc -O2 -std=gnu11 -o build/titlemock src/crawl_art.c src/hand_img.c src/face_img.c src/cat_img.c src/font_vga.c \
      src/crawl_ui.c src/ui_art.c src/gfx.c tools/titlemock.c -lm -lz
  build/titlemock <dir>                # everything, the GIFs doubled
  build/titlemock <dir> a              # a, b or c: one title and its graves; logo; overview; flash (the lightning's frames)
  TM_SCALE=1 build/titlemock <dir> b   # the GIFs at native size
  ```
- `docs/titlepass/`: the stills, `overview.png`, `logo.png` and a README describing the first mock-ups. The GIFs aren't
  committed: they come to about 8 MB at native size and 18 MB doubled. Send them to the owner instead, and keep them out of
  git unless the owner asks.
- How the tool works (`titlemock.c`, top to bottom):
  - **Materials and light.** `MR`/`MV`/`MF`/`MD` (ramp, value 0..1, flags, a direct colour) and `LC`/`LW` (cold and warm
    light). `resolve()` turns these into palette indices, warm light dithering stone and iron into `R_WARM`.
  - **Palette.** `pal_night` lends `R_CEIL` to the sky and `R_FLOOR` to the earth. `pal_flash` is the lightning.
  - **Output.** The GIF writer (`gif_begin`/`gif_frame`/`gif_end`) sends only the rectangle that changed, uses index 255 for
    "unchanged" and gives each frame its own palette. A PNG writer serves the sheets.
  - **The name.** `logo_build` makes it from `GLYPHS`, broad-nib strokes at 35 degrees. `logo_draw` draws it in one of
    two styles: `LS_BLOOD` (letters of blood) or `LS_CARVED` (cut into the stone under it), with drips.
  - **Scenes.** `scene_a`/`a_graves`, `scene_b`/`scene_b_graves`, `scene_c`/`scene_c_graves`. `run()` loops one into a GIF
    and its stills. `GR[]` holds the made-up graves. `sel_k()` is how chosen each of the five choices is over the loop.
- Each scene loops cleanly: everything that moves comes round a whole number of times a loop, and the clouds and fog
  crossfade two drifts. Keep it that way when adding motion.

## 3. Decisions so far

- All three titles stay and **rotate per start** (the owner). How the game remembers which came last isn't settled. A byte
  in `SLOT_PREFS` (the prefs) would be the obvious place. That belongs to the in-game step, not this pass.
- The grave screens show the ledger's three pages as graves. An empty page is an unmarked plot. There are two more rows:
  the unsigned run (the one the screensaver was in) and a new grave. The questions below are still open.
- The mock-ups' words are made up: the Witchfinder at level 14, the Anatomist at a death.
- For the eyes: nothing blinks. The prompt breathes over about four seconds. The lightning is once a loop, soft (the sky
  goes to mid blue-grey), a flash and a second flash. B's bob is about a pixel.

## 4. The refinement pass, step by step

Mark each step as it lands (**done**, with the date), and add a line to the log in section 6.

1. **This plan.** **Done** (9 November).
2. **The sculpt kit.** A small canvas in the tool for built things: parts with heights (a face, raised, sunk); bevels and
   cast shadows by height; grooves and borders inset along an outline; relief (the class marks, a quatrefoil, a wheel-cross);
   breaks (a jagged chunk gone, its edge fresh and pale, the piece lying at the foot); cracks (branching, dark, with a lit
   lip); chips along every edge; ivy; weathering (grain, pits, water run down from the top, rising damp at the foot, lichen
   yellow at its heart, moss). Wood (planks, grain, knots, splits, nails with rust run) and iron are materials in it.
   Carved text: sunk, a little worn, and gilded while chosen, the gold coming in dithered. It renders leaning or turned a few
   degrees. **Done** (9 November): `sc_*` in `titlemock.c`; `build/titlemock <dir> parts` draws every built piece three
   times size (`parts.png`). Small leans are sheared row by row (`sc_draw`), so cut letters keep their shape; a turn
   past 3.5 degrees is a true rotation, for things without words.
3. **The weathered UI.** A title tablet hanging on chains: chipped, cracked, its border cut, and the words gilt but worn.
   The two extra choices on chipped slabs, their words gilded while chosen. Worn stone key caps for the hints. **Done**
   (9 November). Out of doors (A, C) the title is cut into a lych-gate's beam across the top, with slates over it and
   iron bands (`ui_beam`), not a hanging tablet: a tablet hanging from nothing in the open sky looked wrong. Indoors (B)
   it is a tablet set in the wall (`ui_wall_tablet`). The rows are `row_slab`, the hints `ui_keys_worn` and
   `ui_keys_words`, and `ui_light` keeps them readable whatever the scene's light.
4. **The graves, rebuilt with the kit.**
   - **A and C, the headstones.** I is a gothic stele: two-step plinth, pinnacles with finials, a pointed arch, a moulded
     border, the class's mark in relief in a sunk quatrefoil, its numeral on the plinth. Its arch's corner is broken off and
     lies at its foot, with cracks running from the break. II is a wheel-cross on a slab: the cross raised, the ring broken
     on one side, a crack across the slab, ivy up one side.
   - **III, the empty plot.** A rough timber cross, leaning, lashed with rope, its ends split, and a plank hung crooked
     from one nail with the words scratched in. An open pit (C), or the earth undisturbed (A).
   - **Each used plot** gets a stone kerb with corner posts, one finial gone, and dead flowers or a guttered candle.
   - **B's loculi.** Slabs with a moulded border, iron cramps, the mark carved, cracks and a broken corner. The empty niche
     has its slab fallen in two pieces, leaning in it.
   - **Names up to thirteen letters** (PLAGUE DOCTOR) must fit on a stone. If the line is too long, the name line closes up
     to 7 pixels a letter.
   **Done** (9 November): `build_stele`, `build_wheelcross`, `build_timber_cross`, `build_plank`, `build_kerb`,
   `build_fragment` and `build_loculus_slab`, drawn by `grave_drawn` (A and C) and `scene_b_graves`. As built: I's head is a
   steep gable with bowed sides, crockets and a cross finial, on square shoulders. A true pointed arch over the full width
   came out round at this height. Its right shoulder and pinnacle are broken off, and two pieces lie at its foot. In B the
   empty niche has the hung board too, and its slab lies fallen in two pieces under it. A's plots have no kerbs, since its
   cross-section shows the coffins instead. C's have them. Unchosen words are cut deep and dark. Chosen ones fill with gold
   (the board's with bone). Close-up, 7 pixels a letter is enough for WITCHFINDER (77 px); PLAGUE DOCTOR (91 px) would
   need two lines, and that isn't tried.
5. **A's title, the detail pass.** Small stones made with the kit: broken, leaning, crosses, an obelisk snapped. An angel on
   a plinth, the church with one window lit, crows that move. The earth in distinct bands (topsoil and roots, clay, gravel,
   rock over the vault), an old skeleton in a burst coffin. The crypt's arches in separate voussoirs, piers with capitals,
   a sarcophagus. The walker's shadow thrown big on the back wall, the walker with a staff and folds in the cloak. A
   vignette to hold the eye. **Done** (9 November): `small_built` (eight small stones: gable, wheel-cross, angel, round,
   obelisk and its fallen top, a slab gone over, a stump), the church's window, a crow that shakes its wings, the earth's
   bands, the burst coffin, `a_crypt`'s voussoirs, capitals, sarcophagus and chains, the walker's staff, rope, ragged hem
   and lit hood rim, and its shadow (`WMASK`, cast from the lantern on the back wall).
6. **B's title, the detail pass.** The game's own sprites in the hall, against its depth: bones, skull heaps, candles,
   chains, cobwebs. A spectre crossing at the edge of the fog once a loop. The lintel weathered, with a keystone carved with a
   skull, cobwebs and roots in the arch's corners, and the step worn in the middle with candles on it. A vignette.
   **Done** (9 November): `b_sprites` (bone heap, candles, skull stack, the occupied gibbet, chains, roots, an urn, a
   cobweb; none nearer than 1.5 cells, where the piers hide them) and the spectre, `b_build_frame` (the whole arch from
   the kit, each block its own, with a keystone wedge and the skull in relief, a bite out of the opening, ivy up the piers).
   The kit's pieces are drawn undithered (`MF_FLAT`), because dither made carved stone look see-through.
7. **C's title, the detail pass.** The great stone rebuilt with the kit: a moulded border, a winged death's head in its
   head, a broken shoulder with pieces at its foot, cracks, lichen and water stains, MEMENTO MORI cut under the name. The hand
   with its knuckles' bones showing and a torn cuff. Dark grass in the foreground, wilted flowers, a fence and a mausoleum
   behind. A vignette. **Done** (9 November): `build_great_stone` (a double moulding, a winged death's head from `WING` and
   `KEY_SKULL`, volutes, the left shoulder broken off and its pieces at the foot, MEMENTO MORI on the plinth), `grass_fg`,
   the hand's shroud cuff and knuckles (`hand_lift` brightens it where it is small).
8. **Checks and hand-over.** Render everything. Look at each still: the words readable, nothing over the name, the loops
   clean. Update `docs/titlepass/` (stills, overview, logo, README), commit and push, and send the GIFs to the owner with
   short notes. **Done** (9 November): the stills, `overview.png`, `logo.png` and `parts.png` in `docs/titlepass/`, and its
   README rewritten for this pass. The GIFs (about 20 MB doubled) went to the owner, not into git.
9. **Later, not this pass: into the game.** Its own plan: a title scene between the screensaver and play, the rotation,
   the music hook, the graves read from the ledger (`page_info`), and the questions below. It wants the owner's answers
   first.

## 5. Questions still open for the owner

1. Is the unsigned run (the screensaver's) one of the choices, or does taking over always go through the graves?
2. Should the prompt say PRESS ANY BUTTON, PRESS ANY KEY, or show the actual key and button?
3. Should the lightning stay as it is, go softer, or go behind a setting (a "still" title for the eyes)?

## 6. Hand-off log

- **9 November, session 1.** First mock-ups: three titles (A Beneath, B The Walk, C The Risen) and their graves, in
  `tools/titlemock.c`, with stills in `docs/titlepass/`. Committed as "Title screen mock-ups ..." on
  `claude/compassionate-dirac-6ui1wb`. The owner liked all three, asked for them to rotate per start, and asked for the
  refinement pass above. The graves were their main complaint ("two shapes"). Plan written (step 1). Step 2 begun.
- **9 November, session 1, later.** Steps 2-4 done and pushed: the sculpt kit, the weathered UI, the graves rebuilt in all
  three. Next is step 5 (A's title). Iterate with `build/titlemock <dir> s <A|Ag|B|Bg|C|Cg> <seconds>`, which writes one
  still doubled as `s_<name>_<t>.png`, and look at it before rendering the GIFs (about 30 s for all of them).
- **9 November, session 1, end.** Steps 5-8 done and pushed. All six sent to the owner. Waiting on their look at the
  refinement and on the questions in section 5. The next work is whatever they say about this pass, then step 9 (into the
  game), which wants its own plan and the owner's answers first. Known small things: in C's graves the hand rising from
  grave I covers part of its plinth's numeral, and PLAGUE DOCTOR is untried on a stone.
