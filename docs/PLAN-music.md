# The music: a piece for each run, her hall, the menus, the fight and the failing

*Planned 3 November 2026, settled the same day. Built on `claude/vibrant-keller-bdeun7` from main at 1.0.0b14 (with the browser's
build). Nothing of the game moves until the owner has heard the pieces.*

## Status

Released as 1.0.0b15 (4 November). Steps 1-4 are built, harness only: the engine and the pieces (`build/host music` to hear them), the game's side (the march
retired) and the sliders. The owner had the renders and said to wire them in (4 November). Step 5's checks are below.

## What there was

One piece for the whole crypt, the march (`crawl_step` in `src/snd.c`): 32 bars at 92 BPM, about 83 seconds, organ chords,
a marching bass, harpsichord arpeggios, a flute tune then a brass one and a snare roll back to the top. A level of six
minutes hears it four times. Beside it on their own buses: Severance's muzak, the four bosses' waltzes (on their own
clock) and the notes of the fanfare and the stinger. A trial made the march a quarter quicker with harder drums; the void
muffled everything. Nothing followed the run, her hall, a menu, a fight or the caster's health.

## The owner's answers (3 November)

- **M1, the pieces' characters:** as proposed (below).
- **M2, the march:** retired. Its opening phrase lives on in run I's piece and in the menus' tune.
- **M3, what counts as a menu:** every screen that stops the crypt (Esc, Tab's pages, the bestiary, the Codex, the altar, the
  reckoning and the rest); not the map, under which the crypt runs; her own screens keep her music; a short look (the L
  list) switches only after half a second. The one exception is the console (the key left of 1): it is not a menu. Taken
  as: the console keeps what it does now (the game's sounds wait, the music plays on). *Default taken; the owner may want
  something of its own for it, such as the run's piece dropping to a PC speaker's voice while it is down.*
- **M4, a menu closed:** the run's piece takes up where it stopped.
- **M5, the fight:** layers, and the tempo rising as a fight goes on.
- **M6, the screensaver's dream:** the fight and the failing play for it too.
- **M7, volumes:** Music and Effects sliders beside the overall one.
- **M8, the number:** main is at 1.0.0b14, released, with the browser's build added to that release because nothing of the
  game changed. This changes the game, so it goes out as **1.0.0b15**, and the plans after it move up one (the minds'
  showing and the rest of the art pass to 1.0.0b16, the lower storey to 1.0.0b17). Confirmed by the owner, 4 November.

## The pieces

| Piece | Key, metre, pace | What carries it | Sections |
|---|---|---|---|
| Run I, the Order's crypts | D minor, 4/4, 66 | organ held, a low choir, a bell; the march's phrase as chant (flute, or the choir an octave down), a brass chorale | intro, the phrase, the chorale, the crypt (a drone and E-flat over D) |
| Run II, the drowned catacombs | C minor, 6/8, 50 (dotted) | a harp rolling a barcarolle, a soft pad, a drowned bell, drips; the tune on vibraphone, a low flute in the deeper part | the water rises, the boat song, deeper, the drowned bell |
| Run III, the charnel mine | E minor (Phrygian), 4/4, 84 | a work rhythm in the bass, an anvil, low brass held in fifths; a hammered dulcimer | the shift, the work song, the seam (brass calls, dulcimer answers), the shaft |
| Run IV, the black glass | F-sharp minor, 4/4, 56 | a drone, glass harmonica chords, a celesta glinting; G and C major over F-sharp | the glass, the tune (glass), the magma (vibraphone), stillness |
| Her hall | A minor, 4/4, 100 | a habanera on plucked bass, harpsichord marcato, a sly flute with chromatic slides; a music box answering | her entrance, the purr |
| The menus | the run's key, 3/4, 60 | the march's opening as a lullaby over a pad: music box (I), harp under water (II), dulcimer (III), glass (IV) | two of eight bars |

Each run's piece is four sections of eight bars, played in an order of eight (the first only the first time), with the tune
moving between instruments on alternate passes: 3-4 minutes before it comes round. Her back rooms (the Totem, the
Lampstand, the plot) thin her piece to its bass and music box.

New instruments in the synth: glass harmonica, hammered dulcimer (two courses, a hair apart), low strings, pizzicato, an
anvil, a celesta. Voices go from 36 to 48 so that the crossfades don't cut notes off.

## The engine (`src/snd.c`)

- **Players.** A piece is data (`Piece`: pace, metre, key, sections, order) and a step function; a player (`Player`) runs
  one on its own clock and bus. Three players: the run's (`BUS_RUN`, which the march shares while it lives), her hall's
  (`BUS_SHOP`), the menu's (`BUS_MENU`); the bosses' waltzes and the muzak keep theirs.
- **Changing piece.** `snd_set_track(MU_*)`: the run's bus fades out over a second, the new piece starts from its top. The same
  piece again changes nothing, so the music runs on across levels of one run. `snd_set_place(0 crypt, 1 her room, 2 her back
  rooms)`: her piece crossfades in over a second and a half and the run's player stops where it was until the caster comes
  back down. `snd_set_menu(1)`: after half a second the run's (and her, and a boss's) clock stops and the menu's tune fades
  in from its top; `snd_set_menu(0)` fades it out and the stopped clock takes up again.
- **Layers.** Every voice and drum carries a layer (`LY_BASE`, `LY_FIGHT`, `LY_COLOUR`) as it carries a bus. The fight layer's
  gain follows the fight; the colour layer (tunes, arpeggios, glints) thins when failing.
- **The fight.** `snd_set_fight(0-3)`: on the next beat after it becomes non-zero, a crash and a timpani stroke, and the
  piece's fight arrangement (drums, a doubled bass or an ostinato in low strings, brass stabs, a counter-line) fades in over
  a quarter second. The level sets how much of it plays and the tempo: 1 as written, 2 a twentieth quicker, 3 a tenth. The
  tempo glides over a bar or two. At the first bar line after it falls to 0, a closing chord and a soft stroke, and the
  layer fades over two seconds. A trial counts as level 2 at least, and adds an eighth to the tempo (its old quickening).
- **The failing.** `snd_set_frail(1)`: over a second and a half the music buses (not the game's sounds) go through a low-pass
  that comes down to about 700 Hz, the colour layer falls to a quarter, and a heartbeat sounds on each beat of the run's
  piece. Over a boss's waltz too.
- **Volumes.** `snd_set_mix(music, effects)`: the music buses and their drums by one, the game's sounds, the fanfare and the
  hum by the other, under the master volume.

## The game's side (step 3)

- `start_level`: `snd_set_track` by `crawl_run` (pockets keep their rooms as now: the Backrooms' hum, Severance's muzak).
- `begin_shop` and leaving her room: `snd_set_place`; moving between her room and her back rooms, 1 or 2.
- `open_ui` and every way back to `UI_NONE`: `snd_set_menu`, but not for her screens while in her hall (her music plays on),
  and never for the console.
- Each frame (the dream's and a person's alike): the fight's level from `in_combat()` or anything of the new minds hunting
  the caster within nine cells. Into a fight at once at level 1; level 2 after 20 s of it or with three engaged; level 3
  after 45 s or with five engaged or under two fifths of health. Out of it after 6 s with none of that. In her hall, a
  pocket's room, a boss's arena and a trial, 0 (the waltz and the trial have their own).
- The failing below a fifth of health, lifted above a quarter; not in her hall.
- The four calls are cheap enough to make every frame; the engine does nothing when nothing changes.

As built: a cast in the last three seconds with a creature in sight within nine cells counts as a fight too, since the dream
(and a careful person) often kills things that never noticed; a fight's level never falls back until it ends.

## Settings (step 4)

The Windows dialog's *Volume* stays as the overall one, with *Music* and *Effects* under it (registry `MusicVolume`,
`EffectsVolume`, 0-100, both 100 at first). The browser's settings get the same two. `AppConfig` carries them.

## Steps

1. **The engine** (players, layers, the fight's tempo, the failing's filter, the menu's clock, the mix), the new instruments,
   48 voices, and `build/host music <piece> <seconds> out.wav [event@t ...]` to render any piece through any changes:
   `fight0`-`fight3`, `frail`/`well`, `menu`/`back`, `trial`/`untrial`, `place0`-`place2`, `run1`-`run4`. *Built.*
2. **The pieces**: the four runs, her hall, the menu in its four guises. Rendered for the owner to hear (each piece plain
   for three minutes, and through a fight, the failing and a menu). *Built; waiting for the owner's ear.*
3. **The game's side**, as above; the march retired from the crypt (its code goes).
4. **The sliders**: Windows, the browser, the harness.
5. **The checks**: `make smoke`; `build/host bench` and the browser's smoke test for the synth's cost (48 voices, six buses);
   ASan over long sessions in and out of fights and menus; the dream's runs are untouched (nothing in the simulation moves);
   the README's *What has been verified*, the page and the release notes.

## The hand-off log

- **Steps 1-2** (3 November): the engine and the pieces in `src/snd.c` after the bosses' waltzes; `build/host music`. Renders
  are made with `tools/music.sh` into `build/music/` (MP3 by ffmpeg when it is there). Not yet heard by the owner, and not by
  me: what was checked was measured. Calm, the pieces sit 1-3 dB under the march (-18.5 dB RMS; it had hats throughout),
  each with a gain of its own (`Piece.gain`) to get there; a fight at its third level brings them 2-4 dB up, nothing clips.
  The fight's pace measured from the renders: run III's bar 2.865 s calm, 2.605 s at level 3 (2.857 and 2.597 meant). The
  failing halves the energy above 1.5 kHz. No voice was stolen in any render (48 voices). Rendering costs 1.7-1.8% of a core
  in a fight, against 1.4% for the march. The game still plays the march: its output in play matches the old build's but for
  rounding (one step in the last bit, 0.08% of samples), since the buses are now summed apart. ASan and UBSan clean over six
  renders driven through every change. The Windows and browser builds are left to CI (no MinGW or Emscripten here).
- **Steps 3-4** (4 November): the march is gone from `snd.c` (`crawl_step` keeps the cave's drips and Severance's muzak on the
  old clock; `MU_NONE` until the game says which piece). `music_frame` (end of `scene_crawl.c`, wrapped round `crawl_update` by
  `crawl_frame`, so the early returns don't skip it) says all of it every frame. Her room is cells x 10-20, y 12-17 and the way
  on east of it; anywhere else in her hall is her back rooms. The sliders: `AppConfig.music`/`effects`, `snd_set_mix` in
  `app_init`, the registry's `MusicVolume`/`EffectsVolume` and two trackbars under Volume in the Windows dialog (it grew 76
  px), the browser's settings and `web_config`'s two new arguments, `HOST_MUSIC`/`HOST_EFFECTS` in the harness. A dream's run
  at level 13 (seed 7): fights came every 20-50 s, grew to 2 after 20 s or three engaged and to 3 as health fell under two
  fifths, through a trial and three deaths; a person taken over at 15% health was failing at once; Tab, Esc, B, C and the L
  list each brought the menu's tune and her own screens didn't.
- **Step 5, so far** (4 November): `make smoke` passes; ASan and UBSan clean over a dream at the knight's level (a trial on
  the way), a person through Tab, Esc, the L list, B and C at 15% health, and a trial three times; the dream's run on one seed
  is the old build's line for line over 500 s (the music only reads the game's state). CI, started by hand on the branch
  (run 95), built and smoke-tested the Windows build and the browser's (Chromium). Nothing heard on a real machine yet. Left for
  the release: the README's *What has been verified*, the page and the release notes.
- **Released** (4 November) as 1.0.0b15 at the owner's word, squashed onto main as one commit: the README's *Sound*, *Settings*
  and *What has been verified*, the page's run folds (*And its music*) and *Beta status*, and the release notes. Still to come:
  the owner's ear on the music in play, and whatever tuning that asks for (how often a fight comes in is the likeliest).
