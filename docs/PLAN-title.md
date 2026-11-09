# The title screen: plan and hand-off

Status (9 November 2026): **the refinement pass is done (steps 1-8) and the owner approved it. The menu is under way
(section 4b, M1-M4): done, and approved. 9 November: the owner asked for the title in the game, released as
**1.0.0b21.1, "The Title Screen Update"**, with main's music pass (section 7, G1-G6): under way.** Everything here is mock-ups drawn by `tools/titlemock.c`, outside the game.

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
- 9 November, on the refinement: **"All look amazing."** Their answers to section 5: the unsigned run is **not** a choice;
  the prompt reads **PRESS ANY KEY or PRESS ANY BUTTON by the input in use**, and on the title **any key or button counts,
  but not a stick's movement**; the lightning **stays**. And a menu: **after the button, a selection in the middle of the
  screen with four options**: choose a grave, begin a new run, review the controls, and an empty one that leads to a help
  section still to come. **The scene's animation and music go on while it is up.** In the owner's words: "You have most
  of the assets for this, just modify a few so they allow for this selection screen to appear in the middle."
- 9 November: **the music for the title is Shop4 on `claude/beautiful-einstein-tzed36`.** That is the fourth of her hall's
  five attempts, *the lament ground* (G minor, 3/4, 72, a passacaglia on the falling bass G, F, E-flat, D, 2:40). That
  branch already gives it to a title screen: `MU_TITLE`, `SC_GROUND`, `ground_step` in its `src/snd.c`, played for now when
  the ledger is opened to choose a page, and `snd_set_menu(3)` / `g_menu_mu` let a menu play the title's piece. Its
  `docs/PLAN-music-work.md` describes it. `build/host music title <secs> out.wav` on that branch renders it. The mock-ups'
  MP4 previews were made that way, with the branch built in a detached worktree in the scratchpad, then removed.
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

## 4b. The menu, step by step (asked for 9 November)

The flow is the title, then any key or button (not a stick), then the menu in the middle over the scene, which goes on with
its music. From the menu: CHOOSE A GRAVE leads to the grave screen, BEGIN A NEW RUN starts one, THE CONTROLS shows them, and
HELP is a page still to be written. In each scene the menu comes in its own way, built from that scene's pieces:

- **A** has a stone rising out of the earth into the middle of the graveyard, below the name in the sky.
- **B** has a tablet lowered on chains from the lintel into the arch's opening, with the hall walking on behind it.
- **C** has the great stone's face cut again: the name goes onto one line at the top of the slab, the four choices are cut
  under it, and the old face dissolves into the new in a 13h dither, with dust falling.

The chosen choice gilds while it is chosen. A one-line note says what it does: on the menu stone in A and B, on C's plinth.
The hints at the foot follow the input: keys on worn caps, or the pad's buttons in their colours.

M1. **Record the answers; graves without the extra rows; the prompt by input.** **Done** (9 November): `NCHOICE` 3;
    `PADMODE` and `any_prompt` (PRESS ANY KEY / PRESS ANY BUTTON); the hints' caps in the pad's colours (A green, B red, as
    the game's `hint8`) when it is the pad.
M2. **The menu's stones**, with the kit: A's standing stone, B's hanging tablet, C's face re-cut. Each row gilds on its
    own. **Done**: `build_menu_stone`, `build_menu_tablet`, `build_great_stone` with `C_FACE` 1; `menu_rows` (the four, gilt
    notches either side of the chosen, a rule and the note); the kit's `S.gk` and `sc_gk` gild word by word.
M3. **The menu scenes**: a second of the title, the key, the menu coming in, then the choice going down the four. Their
    GIFs: `A_menu`, `B_menu`, `C_menu`. **Done**: `menu_time` (`T_PRESS` 1 s, `T_IN` 0.7 s; the prompt flares as the key is
    taken; at the loop's end Esc, and the menu goes, so the loop comes back to the title), `title_foot_pre` and `title_foot`;
    A's stone rises out of the earth shedding clods, B's tablet comes down on its chains with a small bounce, and C's face
    dissolves in a dither (`scene_c_menu` draws both faces and mixes them by `bay`), dust falling off it. The notes: ONE OF
    HER PAGES, FROM THE BEGINNING, WHAT EACH KEY DOES (or BUTTON), NOT WRITTEN YET.
M4. **Checks and hand-over**: a still of each with the pad's words, the overview, docs, commit and push, sent to the owner.
    **Done**: `overview.png` (each scene's title, menu and graves), `pad.png` (the pad's words), the stills in
    `docs/titlepass/`, and MP4 previews of the three menus with the lament ground under them, sent to the owner (not in git).

THE CONTROLS in the game: the controls card the game already has (`render_hud`'s `more[]`), or the controller's own screen
(`UI_PAD`, `ui_pad`) while a pad is the input. HELP leads to a page saying it isn't written yet. Neither is drawn in these
mock-ups.

## 7. Into the game: 1.0.0b21.1, "The Title Screen Update" (asked for 9 November)

Main now has the music pass (the lament ground, `MU_TITLE`, played by `snd_set_menu(3)`). The owner asked to release the
title as 1.0.0b21.1, "The Title Screen Update". Decisions taken as defaults (say so to the owner):
- **When it shows.** Every time a person takes over: H held three seconds, or any button on a pad. Before, that handed the
  dream's run straight over. Now the title comes first, and the dream's run is not one of its choices (section 1). The
  crypt waits behind the title.
- **Which one.** The title comes round in turn, A, B, C, once per start of the program. The last one shown is a 24th byte
  of the prefs record (`SLOT_PREFS`, the pad's layout), which older 23-byte records lack and still read.
- **Esc.** Esc counts as "any key" on the prompt (the owner: any key or button). In the menu, Esc (B) goes back to the
  prompt; in the graves, the controls or help, back to the menu. Untouched for 90 s at the prompt, the title gives the
  screen back to the dream, so a screensaver left alone goes on dreaming.
- **The choices.** CHOOSE A GRAVE opens the three pages (`page_info`, a page's class from its `LG_ALIGN`); a used page
  rises at once (`ledger_open`); an empty one says nothing is written there. BEGIN A NEW RUN is `run_new` at once. THE
  CONTROLS is the keys' whole list on a slab, or the controller's own screen (`ui_pad`, read-only) with a pad. HELP is a page
  that says it isn't written yet.
- **The art at level 3**, as the mock-ups had it: `crawl_art_level(3)` while the title is up; a choice starts its level's
  art as ever; going back to the dream puts the dream's level's art back.
- **Debug.** `CRAWL_TITLE=0` skips the title in the debug build (as before: H gives the dream's run). `tools/uishots.sh`
  and `tools/gallery.sh` set it.

G1. **The art as a module**: `src/crawl_title.c/.h` (`title_draw(Frame *, float t, const TitleView *)`), moved out of
    `tools/titlemock.c`, which becomes its driver for the mock-ups. **Done** (9 November): `make build/titlemock` remakes
    the approved mock-ups from the module, frame for frame (the overview checked by eye). `title_debug_frame` serves the
    logo and parts sheets. The view (`TitleView`) carries the scene, the pad, the graves or not, the menu's coming,
    the key's flare, each choice's and each grave's chosen-ness, the pages' words (`title_page_set`), a foot word and the
    version. `src/crawl_title.c` is in the Makefile's `SRC`, so the Windows and web builds take it.
G2. **The title in the game**: its state (`TI` in `scene_crawl.c`), input (any key or button, not a stick), render, music
    (`snd_set_menu(3)`), the rotation, the idle return. **Done** (9 November, harness only). The block sits just
    before "death's cost" in `scene_crawl.c` (`TI`, `title_enabled`, `title_next_scene`, `title_pages`, `title_open`,
    `title_to_dream`, `title_to_play`, `title_any`, `title_nav`, `title_pick`, `title_tick`, `title_controls`, `title_help`,
    `title_up`, `title_render`), with prototypes after `pad_look_render`'s. Its hooks: `crawl_control` (CTRL_TAKE opens it
    unless `CRAWL_TITLE=0`; CTRL_PAUSE leaves it be), `crawl_input` (no `pad_keys` while it is up), `crawl_update` (its
    `title_tick` first, then return), `crawl_render` (`title_render` before anything else), `music_frame` (`snd_set_menu(3)`,
    no fight or failing). The prefs' 24th byte is `PL.title` (`PL_BYTES_TITLE`). `Page` gained `cls`, read from the
    page's `LG_ALIGN` in `page_info`. Debug switches: `CRAWL_TITLE=0`, `CRAWL_TITLESCENE=k` (0-2); `EVENT title open`,
    `menu`, `pick`, `rise`, `new`, `dream`.
G3. **The four**: the graves from the ledger, a new run, the controls, help. **Done** (9 November, harness only):
    `title_pick`, `title_to_play`, `title_controls`, `title_help`.
G4. **The checks**: frames of every state in the harness; scripted input down every path into play; an ASan session; the
    smoke test; a bench of the title's frame; the mock-ups made again from the module. **Done** (9 November): with the
    keys and with a scripted pad, the prompt, the menu, the controls (the keys' list; the controller's screen with a pad),
    help, the graves (empty: NOTHING IS WRITTEN HERE; a page signed through her menu rises with `ledger_open`, its class read
    as the Witchfinder), a new run into her room and its class question, the rotation (A, then B, then C, the prefs record
    24 bytes), and 90 s untouched giving the screen back to the dream, which went on. ASan and UBSan clean in each scene down
    every path, and with the pad into a new run. `make smoke` passes. `build/titlemock <dir> bench`: a title's frame is
    19-28 ms here (the crypt's is 1.8 ms), since the earth, sky and stone are drawn afresh every frame; caching the still
    parts is a follow-up if the owner finds it slow. Not run: the Windows and browser builds (no MinGW or Emscripten here;
    CI compiles them), nothing by hand on Windows.
G5. **The docs**: README (features, what was verified), CLAUDE.md, the page, the release notes (the music pass's too).
    **Done** (9 November): README's *CATACOMB: the title screen* (with `docs/preview_catacomb_title.png`), the play
    section, the music and *New in 1.0.0b21.1*; the page's play section, figure, music, upgrading and beta status; the release
    notes rewritten; CLAUDE.md's release line, a paragraph and the map.
G6. **The release**: VERSION 1.0.0b21.1, squashed onto main as one commit; CI publishes it; check the release and the page.

## 5. Questions still open for the owner

None open now (answered 9 November: see section 1).

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
- **9 November, session 1, after the owner's look.** Approved ("All look amazing"). The answers and the menu are
  recorded in sections 1 and 4b. M1 is under way.
- **9 November, session 1, later still.** M1-M4 done and pushed; the owner was sent the menus' GIFs and MP4s (the lament
  ground under them). Waiting on their look. The next is whatever they say, then step 9 (into the game). Its plan should
  start from section 4b: the title scene and its rotation, any key or button but a stick to go on, the menu, the grave
  screen from `page_info`, THE CONTROLS from the controls card and `UI_PAD`, HELP's empty page, and `MU_TITLE` from the
  music branch.
- **9 November, session 1, at the owner's word to release.** Main's music pass merged into the branch (no conflicts). G1
  done. G2 and G3 are written and the first build is running. Usage is nearly spent for the week, so if a new agent takes
  this up: build (`make build/host build/host_debug build/titlemock`), fix what fails, then G4's checks. Take over with
  `HOST_INPUT="1:+H;4.3:-H;6:+ENTER;6.1:-ENTER"` and shots at 5, 7 and 9 s; `CRAWL_TITLESCENE=1` for B; pick the menu's
  rows with `+S`/`-S`. Then G5 and G6 as written. The release notes must also cover main's unreleased music pass (commit
  789978b, "The game's music: her hall's pieces, run I's and run II's, the Lampstand's"; its plan is
  `docs/PLAN-music-work.md`).
- **9 November, later.** G2-G4 done (harness only) and pushed. Next: G5 (README, CLAUDE.md, the page, release notes with
  the music pass), then CI on the branch by hand (`actions_run_trigger` on build.yml) to compile Windows and web, then G6.
- **9 November, later still.** G5 done and pushed. CI started by hand on the branch (run 42709106409) to compile Windows
  and web. Next: when it is green, G6. Set VERSION to 1.0.0b21.1, squash the branch onto main as one commit (main is
  789978b, already merged into the branch), push main, then check the release and the page.
- **9 November, release in hand.** CI on the branch: the Windows and browser builds compiled; their smoke tests were still
  running at this entry. The release commit is staged locally on `release-b21-1` (from origin/main, the branch squashed,
  VERSION 1.0.0b21.1). If this session ends before it is pushed: `git fetch origin main && git checkout -B release-b21-1
  origin/main && git merge --squash origin/claude/compassionate-dirac-6ui1wb`, set VERSION to 1.0.0b21.1, commit as
  "1.0.0b21.1: The Title Screen Update" with the attribution lines, `git push origin release-b21-1:main`, then check the
  release (`gh api repos/ASPDesignLabs/HAUNTED.EXE/releases/tags/v1.0.0b21.1`) and the page.
