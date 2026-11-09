# A title screen for CATACOMB: the mock-ups

These began as mock-ups (9 November 2026) and went into the game the same day, in 1.0.0b21.1: the art is now
`src/crawl_title.c`, which the game and `tools/titlemock.c` both draw with. The plan and its hand-off log are in
[`docs/PLAN-title.md`](../PLAN-title.md). The owner kept all three, to come round in turn at each
start, and asked for a refinement pass. These stills are from after it. Asked for by the owner:
a title screen that comes up when a person holds H (or a button) to enter the crypts, the name in a gothic, bloody hand,
"press a button", then the graves to choose from. The owner already has the music.

Each was drawn by `tools/titlemock.c` as the game draws: 320x200, one palette of 256 six-bit colours, built from the game's
own palette (level 3), run I's textures, the light table, and the carved stone's faces and pieces (`ui_row`, `ui_band`,
`ui_keys`). Nothing outside the palette was added. The GIFs aren't kept here, because they run to about 8 MB at native size
and 18 MB doubled. To make them again:

```sh
make build/titlemock
build/titlemock <dir>                # every GIF (doubled), its four stills, overview.png and logo.png
TM_SCALE=1 build/titlemock <dir> a   # one of them, at native size
build/titlemock <dir> flash          # the lightning's frames, to check them
```

- `overview.png`: a still of each of the six.
- `logo.png`: the name, in blood and cut in stone, on one line and stacked.
- `A_menu.png`, `B_menu.png`, `C_menu.png`: the menu after the key (docs/PLAN-title.md, 4b), four stills from each loop:
  the title, the menu coming in, the choice going down. `pad.png`: the words when the pad is the input.
- `parts.png`: the built pieces of the grave screens alone, three times size: the stele, the wheel-cross, the timber cross
  and its board, a kerb, a broken piece, the rows' slabs and the worn key caps.
- `A_beneath.png` ... `C_graves.png`: four stills from each loop.

## The name

HAUNTED.EXE is drawn as strokes of a broad nib held at 35 degrees, so the stems come out thick with their ends cut on the
slant, as a textura's are. Each letter has a few thorns, and the whole is filled at four by four under each pixel. ".EXE" is
a little smaller. There are two treatments:
- **In blood**: wet red with a dark outline, a gleam that crosses it once a loop, and drips that grow, swell, let a drop go
  and draw back. The drops fall to the ground and splash. The gleams are the skin ramp's palest steps.
- **Cut in stone**: incised letters, their walls in shadow and blood in the cuts, the stone's lip lit under them. The drips
  run down the stone.

## The menu (after the owner's answers)

A key or a button (not a stick) brings a menu into the middle, over the scene, which goes on: CHOOSE A GRAVE, BEGIN A NEW
RUN, THE CONTROLS, HELP. In A a stone with a steep gable rises out of the earth into the graveyard. In B a tablet comes
down on chains from the lintel into the arch. In C the great stone's face is cut again, the name on one line over the four,
the old face dissolving into the new in a dither. The chosen choice is gilt with a notch either side, and a short note
under the four says what it does. The prompt reads PRESS ANY KEY, or PRESS ANY BUTTON when the pad is in use, and the
hints follow the input. The grave screens now hold only the three graves: the unsigned run is not a choice.

## The refinement pass

The owner's notes on the first mock-ups were that the graves and the plots' markers looked like two shapes, and wanted
chips, cracks and destruction. So the graves are now built of parts in a small sculpt kit in the tool: each part has a
height, and gets bevels, cast shadows, seams, grooves and relief from it. Then come breaks (the new edge pale), cracks with
a lit lip, chips along every edge, ivy, lichen yellow at its heart, water stains from the top and damp from the foot. Wood
and iron are materials too. Words are cut deep and fill with gold leaf, a little at a time, while their grave is chosen.
The pieces:
- **I**, a gothic stele: a stepped plinth, pinnacles with finials, a steep gable with crockets and a cross on top, a
  moulded border, the Witchfinder's mark in relief in a sunk quatrefoil. One shoulder and its pinnacle are snapped off and
  lie at its foot, and cracks run from the break.
- **II**, a wheel-cross on a gabled slab: the ring broken away on one side, a crack across the slab, ivy climbing it, the
  Anatomist's mark on the boss.
- **III**, the empty plot: a rough timber cross lashed with rope, its top split, a board of two planks hung from its arms on
  cords, nails rusting down it.
- C's plots have kerbs with a finial gone. B's loculi are slabs on iron cramps under arches of voussoirs with a skull for
  each keystone, and the empty niche's slab lies fallen in two pieces.
- The screens' title is cut into a lych-gate's beam out of doors (A, C) and into a tablet set in the wall indoors (B). The
  extra rows are chipped slabs, and the hints are worn stone caps.

The titles were given more detail too:
- **A** has built small stones (leaning, snapped, gone over, an angel among them), one lit window in the church, a crow
  that moves, the earth in bands (topsoil and roots, clay, gravel, the rock over the vault) with a burst coffin and its
  skeleton, the crypt's arches in voussoirs with capitals, a sarcophagus and chains, and the walker with a staff, a rope
  belt and a ragged hem, throwing a big shadow on the vault's wall.
- **B** has the game's own props in the hall at their depth (bone heaps, candles, a skull stack, the occupied gibbet, chains,
  roots), a spectre crossing at the edge of the fog, and the arch built block by block with a skull on its keystone, roots
  and cobwebs in the opening, ivy up the piers and candles on the worn step.
- **C** has the great stone rebuilt: a double moulding, a winged death's head, volutes, one shoulder snapped off, ivy, and
  MEMENTO MORI on the plinth. The hand has a torn shroud cuff and its knuckles' bones showing. There are railings and a
  mausoleum behind, leaves on the ground, dead flowers, and dark grass in the foreground.

## The three, as first drawn

- **A, BENEATH.** A graveyard in cross-section under a rising moon: hills, a church, a dead tree, a mausoleum, fog. Under it
  are the earth's strata, roots, stones, loose bones and three coffins, and under those a buried crypt of arches. A walker
  with a lantern crosses it once a loop (12 s). The lantern is the only light down there, and a pair of red eyes in a niche
  shut while it passes. Lightning comes once a loop. The name is in blood across the sky, and its drops fall onto the graves.
  *Its graves*: two headstones and a wooden board for the empty page, each grave's coffin below. The chosen stone has its
  candles lit and mist coming off it, and its coffin's lid is cracked with light.
- **B, THE WALK.** Run I's hall in the first person, walked slowly into the dark (8 s a loop). It is a small raycaster with the
  game's own walls, torches, niches, banner, floor, ceiling and light table. A step makes the view bob by about a pixel. It
  is seen through a stone arch, and the name is cut into the lintel and bleeds down into the hall. *Its graves*: a crypt wall
  with three loculi. The chosen slab is drawn aside a little, with a cold light behind it and its candle lit, and the empty
  page is an open niche.
- **C, THE RISEN.** One great headstone close up, a skull cut in its head and the name cut into it and bleeding. Six candles
  burn at its foot, autumn leaves and a bat cross the sky, lightning comes once a loop (8 s), and out of the grave beside it
  a hand claws at the air while earth falls off it. *Its graves*: three plots. A hand rises out of the chosen grave's mound,
  and the empty page is an open pit with a spade in its earth.

As first drawn, each grave screen showed the ledger's three pages as graves, then the unsigned run and a new grave as rows;
the owner's answers took the rows out. The pages' words in the mock-ups (the Witchfinder at level 14, the Anatomist at a
death) are made up; the game's come from the ledger.

## What in it is 13h's own

- Light picks a step within a colour's ramp. There are no new colours. Warm light moves stone and iron into R_WARM,
  dithered by how much of the light is warm.
- For the night, the sky and the earth borrow R_CEIL and R_FLOOR, as a menu borrows them for the marble. B keeps the game's
  palette as it is.
- Lightning is the DAC rewritten for a few frames: the sky's ramp is raised, and the stone's a little. Each GIF frame carries
  the palette it was drawn in.

## For the eyes

Nothing blinks. "PRESS ANY BUTTON" breathes in and out over about four seconds. The lightning is once a loop, a flash and a
second flash in a third of a second, then a fade. The sky goes to a mid blue-grey, not white. Any of it could have a still
setting.
