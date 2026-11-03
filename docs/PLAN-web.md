# HAUNTED.EXE in the browser

**Status: a draft (3 November 2026); discovery only, nothing built.** The owner asked whether HAUNTED could
be offered as something to play in the browser, on the project page, and what the work would be. The answer
is yes, and for less than it sounds. The game was written against a narrow platform boundary, so a browser
build is a third shell beside the Windows program and the test harness, with no change to the game itself.
The questions at the end (W1-W9) carry a recommended answer each.

## What was asked

- HAUNTED.EXE playable in a browser, on the GitHub Pages site.
- A discovery and planning pass first: is it possible, and how much work? No code yet.

## The short answer

Possible, and most of the risk is in the browser's own rules, not in the game. The game compiles to
WebAssembly with Emscripten more or less as it is, and a page of JavaScript stands in for `win_main.c`. Both
scenes, the dream, the takeover, sound, saves and the controller can all come across. What changes is at the
edges. Two keys the game uses belong to the browser (Esc and Ctrl). Saves live in the browser's storage, which
can be lost. A phone can watch but not play.

## What was found

**The boundary.** Everything the game does goes through `core.h` and `app.h`: `app_init`, `app_update(dt)`,
`app_present` (the frame as RGBA bytes), `snd_render` (stereo 16-bit at 44.1 kHz), an `Input` struct the
platform fills, `pad_rumble`, and `app_slot_load`/`app_slot_take` for the saves. `tools/host.c` (300 lines)
already drives the whole game through exactly that boundary on Linux, and a browser shell would follow it.
`win_main.c` is 1,005 lines, but most of that is the settings dialog, multiple monitors, the `/p` preview,
GDI and OpenGL. A browser needs the loop, the picture, the sound thread's job, keys, the mouse, the pad and
the registry's job, and little else.

**Portability checks made in this session** (on the Linux harness; none of this is a browser build yet):

| Check | Result |
|---|---|
| The fourteen game sources built with clang 18 (Emscripten's compiler is clang) | Clean but for 4 trivial warnings (unused variables, an empty format string) |
| The crypt, frame cost, clang build | 2.2 ms a frame (gcc: 2.7 ms) |
| Heavier frames (level 36 at gear 8; level 10's boss fight) | About 3.0 ms a frame |
| HAUNT.EXE, frame cost | 0.7 ms |
| Starting up (texture and level generation) to the first frame | 0.2 s for the crypt, 0.01 s for HAUNT.EXE |
| Static memory | 19.4 MB (the frame, the maps, the fields, the textures); a 32 MB wasm memory covers it |
| The largest stack frame (`-fstack-usage`) | 2.9 KB (`map_scroll_make`); Emscripten's 64 KB default stack would do, 1 MB to be safe |
| 64-bit assumptions | No `long` anywhere, no pointer-sized ints in saves; saves are packed byte by byte, little-endian, which wasm is too |
| Calls to the operating system in the game code | `time()` (run ids, a stone's date: Emscripten has it), `getenv` (debug switches only), `fprintf(stderr)` (debug logs: they go to the browser's console), and `snd.c`'s lock, which exists only under `_WIN32` (the browser build is single-threaded and needs none) |

**Not tried:** an actual WebAssembly build. There is no wasm C library in this container, and downloading the
Emscripten SDK was not allowed in this session. That is step 0 below, and the go/no-go.

## How it would work

A new `src/web_main.c` (the C side: a handful of exported functions over `app.h`) and a `web/` folder (a page,
its script and an audio worklet), built by `make web` with Emscripten. Each Windows piece and the browser's
stand-in:

| Windows (`win_main.c`) | Browser |
|---|---|
| The main loop, `QueryPerformanceCounter`, dt capped at 0.1 s | `requestAnimationFrame`, the same cap |
| OpenGL or GDI, integer scaling, letterboxed | `app_present(dst, 0)` writes RGBA straight into wasm memory; the page puts it into a canvas the size of the mode (320x200 or 720x400) and scales it by a whole number with `image-rendering: pixelated`, counting `devicePixelRatio` so every pixel is the same size |
| waveOut on its own thread, the device owning the music's clock | An `AudioWorklet` fed from the main thread: each frame `snd_render` tops up a queue of about 80 ms that the worklet plays from, so the clock still follows the sound card (`app_init(cfg, 1)`, as on Windows). A `SharedArrayBuffer` ring would be tighter, but it needs headers GitHub Pages can't send |
| Keys by virtual key; the console by scan code 0x29 | `KeyboardEvent.code`, which is positional: `Backquote` is the key left of 1 on every layout, as the console wants. Space, Tab, the arrows, PgUp/PgDn and Backspace have their page scrolling and focus moving stopped |
| Raw mouse, cursor clipped and hidden | Pointer Lock, with `unadjustedMovement` (raw counts, no acceleration) where the browser has it, which is Chromium; elsewhere a sensitivity setting, since the counts arrive accelerated. The right button's menu is suppressed |
| XInput loaded at run time | The Gamepad API. Its "standard" layout *is* XInput's (A B X Y, bumpers, triggers, View, Menu, the stick clicks, the D-pad), so `pad_keys` and the hints carry over. Rumble through `vibrationActuator` (Chrome and Edge; Firefox has none). PlayStation and Switch pads come free here, which the Windows build left for later |
| The registry: `Slot1`-`Slot3`, `Progress`, `Mark`, `PadSprint` | `localStorage` under the site's origin, written from `app_slot_take` each frame as `slots_flush` does. The bytes are the registry's bytes |
| The `/c` settings dialog | A small panel on the page: sound, volume, brightness, scenes, minutes a scene, the controller, the pad's sprint button, and mouse sensitivity (new) |
| `SetThreadExecutionState`, `SC_SCREENSAVE` refused while a pad plays | The Screen Wake Lock API while the page is in front |
| Losing focus mid-game: it waits at the menu | `blur` and `visibilitychange` call `app_focus(0)`, the same path |
| FLEE TO THE SURFACE exits the program | Nothing to exit to: back to the dream, or a title (W5) |

So the game's own code needn't change at all. Dropping Ctrl (below) is done in the shell by not passing the
key on, and so is fleeing. Nothing in the simulation, the saves or the dream's numbers moves.

## Where the browser gets in the way

These are the real work, and where the owner's answers are wanted.

1. **Esc.** The browser keeps Esc for leaving pointer lock and full screen, and the page may never see the
   press that does it. The game reads Esc in 32 places: the question in play, and back out of every menu.
   - With the mouse locked, the first Esc only unlocks it. Treated as Esc in play, that opens the question,
     which is what Esc does there anyway. In a menu it would close the menu, but leave the mouse free until
     the next click. Chrome won't lock again for about a second after an Esc, and only on a click.
   - In full screen on Chrome and Edge, the Keyboard Lock API gives Esc back to the page (holding Esc leaves
     full screen). Firefox and Safari don't have it.
   - So the browser's Esc will feel a little different from the Windows one; how much depends on W2.
2. **Ctrl is a cast key** (with the left button), and the browser owns Ctrl+W (close the tab), Ctrl+T,
   Ctrl+N and Ctrl+1-9 (switch tabs). With W walking forward and 1-9 choosing spells, a person casting with
   Ctrl would close or leave the game within a minute. The page can't stop those combinations, except under
   Keyboard Lock in full screen on Chromium. No on-screen text names Ctrl (checked), so taking it away
   costs no words (W3).
3. **Taking the mouse needs a click.** Pointer lock must be asked for from a fresh click or key press. After
   three seconds of holding H the press may count or may not, depending on the browser. If it doesn't, the
   game is yours with the mouse still free, and a plate reads *CLICK TO TAKE THE MOUSE*. Sound has the same
   rule: nothing plays until the page has been clicked once (W8).
4. **Saves can be lost.** `localStorage` belongs to the browser and the origin. Clearing site data wipes the
   ledger. Safari deletes a site's storage after seven days without a visit. The page should ask for
   persistent storage (`navigator.storage.persist()`) and offer an export and import of the ledger as a file
   (W4). The origin is `aspdesignlabs.github.io`, shared with any other Pages site of the organisation, so
   the keys need a prefix.
5. **Two tabs, one ledger.** Two tabs playing at once would write over each other's pages. The Web Locks API
   can let one tab play and tell the second so.
6. **Phones and tablets.** They can watch: both scenes and the dream, with sound after a tap. Playing needs a
   keyboard and mouse, or a controller (Android's Chrome and iOS's Safari both see Bluetooth pads). Touch
   controls would be a separate and larger piece of work, not proposed here (W6). How fast a phone runs it is
   unknown until step 0.
7. **Speed.** WebAssembly usually runs at one to two times native time, so about 3-6 ms of the 16.7 ms a
   frame has on a desktop, from the 2.2-3.0 ms above. Laptops are slower and phones slower again; to be
   measured.
8. **Download size.** Probably 1.5-2.5 MB of wasm (the x86 build's code is 2.1 MB, most of it the art
   generators), something like 0.6-0.9 MB compressed. Pages compresses. To be measured in step 0.
9. **The screensaver's own rules.** In the dream on Windows, a key or the mouse ends the saver. In the
   browser nothing should end it: keys other than H, and the pad's held button, are ignored until a person
   takes over. The plate's words (*HOLD H TO TAKE CONTROL*) stand.
10. **The console** works as on Windows. Its codes stay hashed inside the wasm as they are inside the exe, and
    nothing about the browser build may name them.

## The page and CI

- **Where.** A page of its own at `/play/` (`aspdesignlabs.github.io/HAUNTED.EXE/play/`), filling the window
  with a full-screen button and the settings panel. The download bar gets a fourth cell, *PLAY IN THE
  BROWSER*, and "What has been verified" and the page's *Beta status* table get a row for it (W1).
- **It plays the released build.** The page describes the released build only, and main sometimes carries
  unreleased work (as it did from 1 November). So the Pages workflow mustn't build the game from main.
  Instead, `build.yml`'s release job builds the browser version with a pinned Emscripten
  (`mymindstorm/setup-emsdk`) and attaches `HAUNTED-<version>-web.zip` to the release beside the Windows
  files. `pages.yml` unpacks the newest release's zip into `/play/`. The two workflows run at once on a
  release push today, so the Pages run would follow the build's (`workflow_run`) instead of racing it.
- **A smoke test in CI.** Playwright's Chromium loads the built page headless, plays twenty seconds of the
  dream and holds H through a takeover. It checks that the canvas isn't black, the console shows no errors
  and a save reaches `localStorage`. `make smoke` stays as it is.

## Steps

0. **The probe (the go/no-go).** Emscripten pinned, the fourteen sources compiled as they are with a
   bare-bones shell: the frame size, the download, ms a frame in Chrome and Firefox on a desktop, and if the
   owner can, on a phone. A short report, then the owner decides.
1. **The picture and the loop.** `src/web_main.c`, `web/index.html`, `web/haunted.js`, `make web`: both
   scenes rotating, the dream, integer scaling, full screen.
2. **Sound.** The worklet and its queue, the click to begin, quiet in a hidden tab.
3. **Keys, mouse and the takeover.** Pointer lock, the Esc and Ctrl rules as W2 and W3 settle them, the pause
   on losing focus, fleeing (W5).
4. **Saves and settings.** `localStorage`, persistent storage, export and import (W4), one tab at a time, the
   settings panel.
5. **The controller.** The Gamepad API, dead zones as `win_main.c` has them, rumble where there is any, the
   wake lock.
6. **CI and the page.** The release asset, the Pages job, `/play/`, the download bar's cell, the README and
   the beta table.
7. **The checks.** Playwright in CI; by hand in Chrome, Edge, Firefox and Safari (the owner's, as Windows is
   now); the harness and ASan runs unaffected, since no game code moved.

As a size: the C side some 150 lines, the page's script some 600-800 lines of JavaScript, a little HTML and
CSS, and some 60 lines of CI. That is smaller than `win_main.c`, and about the size of 1.0.0b8's controller
work.

## Questions

**W1. Where does it live?**
(a) its own page at `/play/`, filling the window, linked from the download bar *(recommended: the main page
stays light, and the game gets the whole window and the keys)*; (b) a canvas embedded in the main page; (c)
both: a small one in the page that opens out to full window.

**W2. Esc.**
(a) as well as it can be: the mouse unlocking counts as Esc, and in full screen on Chrome and Edge, Keyboard
Lock gives Esc back whole *(recommended)*; (b) (a) and a second key for Esc's jobs in the browser (Backspace
already closes some screens); (c) leave Esc to the browser and use another key for the question.

**W3. Ctrl.**
(a) not a cast key in the browser; the left button casts *(recommended: no way to lose the tab by
accident)*; (b) a cast key only in full screen under Keyboard Lock, where the browser lets it be; (c) as on
Windows, with a warning.

**W4. Saves.**
(a) `localStorage`, persistent storage asked for, and the ledger exported and imported as a file
*(recommended)*; (b) `localStorage` alone; (c) (a), and the file reads on Windows too (an import there, or a
reader for a `.reg` export of `HKCU\Software\HalloweenVGASaver`), so a run can cross between the two.

**W5. Fleeing to the surface.**
(a) back to the dream, with a line that you fled *(recommended: the page is a screensaver that never ends)*;
(b) a title screen with *ENTER THE CRYPT*; (c) the page reloads.

**W6. Phones and tablets.**
(a) watch only, with a note that playing needs a keyboard and mouse or a controller *(recommended)*; (b) touch
controls as a plan of their own, later.

**W7. Which release?**
(a) with the next release, whatever it carries, since it moves no game code and so no numbers *(recommended)*;
(b) a beta of its own, moving the plans' numbers again; (c) on the page before a release, marked as a preview
of the released build.

**W8. How the page starts.**
(a) a dark plate in the page's DOS prompt style (*C:\>HAUNTED.EXE*, click or press a key to boot) that
unlocks sound, then the BIOS POST *(recommended: HAUNT.EXE begins with a boot anyway)*; (b) start silently in
the dream with a sound button.

**W9. The settings panel.**
(a) the `/c` dialog's settings and mouse sensitivity *(recommended)*; (b) only sound, volume and sensitivity;
(c) none: the Windows defaults.
