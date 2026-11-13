# Tidying the repository: plan and hand-off

Status (13 November 2026): **settled with the owner the same day. Steps 0 to 3 are done (the baseline; the pictures out of
`docs/`; `src/` sorted by kind; object files); step 4 waits for the owner's word on its list of cuts, and step 6 for their word on its outline.** Nothing here changes the game: no version is
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
