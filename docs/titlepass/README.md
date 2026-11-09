# A title screen for CATACOMB: mock-ups

Mock-ups only (9 November 2026): nothing here is in the game, and the game's code is untouched. Asked for by the owner:
a title screen that comes up when a person holds H (or a button) to enter the crypts, the name in a gothic, bloody hand,
"press a button", then the graves to choose from. The owner already has the music.

Each was drawn by `tools/titlemock.c` as the game draws: 320x200, one palette of 256 six-bit colours, built from the game's
own palette (level 3), run I's textures, the light table, and the carved stone's faces and pieces (`ui_row`, `ui_band`,
`ui_keys`). Nothing outside the palette was added. The GIFs aren't kept here, because they run to about 8 MB at native size
and 18 MB doubled. To make them again:

```sh
gcc -O2 -std=gnu11 -o build/titlemock src/crawl_art.c src/hand_img.c src/face_img.c src/cat_img.c src/font_vga.c \
    src/crawl_ui.c src/ui_art.c src/gfx.c tools/titlemock.c -lm -lz
build/titlemock <dir>                # every GIF (doubled), its four stills, overview.png and logo.png
TM_SCALE=1 build/titlemock <dir> a   # one of them, at native size
build/titlemock <dir> flash          # the lightning's frames, to check them
```

- `overview.png`: a still of each of the six.
- `logo.png`: the name, in blood and cut in stone, on one line and stacked.
- `A_beneath.png` ... `C_graves.png`: four stills from each loop.

## The name

HAUNTED.EXE is drawn as strokes of a broad nib held at 35 degrees, so the stems come out thick with their ends cut on the
slant, as a textura's are. Each letter has a few thorns, and the whole is filled at four by four under each pixel. ".EXE" is
a little smaller. There are two treatments:
- **In blood**: wet red with a dark outline, a gleam that crosses it once a loop, and drips that grow, swell, let a drop go
  and draw back. The drops fall to the ground and splash. The gleams are the skin ramp's palest steps.
- **Cut in stone**: incised letters, their walls in shadow and blood in the cuts, the stone's lip lit under them. The drips
  run down the stone.

## The three

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

Each grave screen shows the ledger's three pages as graves, then the unsigned run and a new grave as rows. The pages' words
(the Witchfinder at level 14, the Anatomist at a death) are made up. The choice moves through all five once a loop to show
each state.

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
