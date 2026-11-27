# Small things that move: vermin and three new kinds (PLAN-vermin)

The owner, 27 November 2026: "We need a few 1 cell sized enemies that primarily move around to cause damage or encourage a player
to look around an area." Of the eleven offered they chose five:

| # | What | As | Where |
|---|---|---|---|
| 1 | **Rat**: runs a floor back and forth, turns at walls and edges, bites on touch (a nudge) | vermin | side passages |
| 2 | **Spiderling**: scuttles along floors and up walls and over ceilings; drops on you from above | vermin | side passages |
| 5 | **Skull hopper**: a loose skull bouncing in arcs between two points | a kind (`MOB_HOPPER`) | side passages |
| 6 | **Bile leech**: lives in standing poison, leaps out in an arc at you, falls back | a kind (`MOB_LEECH`) | side passages |
| 9 | **Coin crab**: a coin till you come near, then it runs on little legs along the path you paint, to the place you paint; cornered, it is yours | a kind (`MOB_CRAB`) | side passages and the crypt |

"I should be able to paint the path it takes and the place it runs to" (the crab). "Full creatures for 5, 6", then "full creature for
9, I'll use it in the Crypt as well".

## 1. Settled by default (said to the owner; theirs to change)

- **Vermin** are not kinds: no record, no souls or experience, nothing saved. A spell or a flask kills one; it bites for a little and
  nudges rather than throws. They live in side passages only, placed in the World Editor.
- **The three kinds** are full creatures: the record (kills by rank, by element, harms, seen), a page in the bestiary once met (as the
  charnel rat's, hidden till then), souls and experience. None is spawned by the crypt's generator or the passages' `ANY`: they are
  where the owner puts them. They come in one rank (`RK_I`, no family to recolour) for now. No portraits, no slayer achievements: the
  hall's portraits and the achievements' bits are laid out for the first 24 kinds.
- **The crab** is harmless. Caught (touched once it has stopped, or at any time in a side passage) or slain, it leaves gold: coins by
  default, its kind of gold set in the editor.
- The dream is not taught the vermin (as the kit). It meets the three kinds as it meets the dead.
- Their pictures are drafts, to the owner as a sheet.

## 2. The record widened (a new section of the ledger, no new version)

`REC_KINDS` stays 24: the save's fixed part, the achievements' bits (`ACH_SECRET0 = REC_KINDS * NRANK`) and the portraits' tapestry bits
(the bosses' from bit 24) are laid out by it. The record's arrays in memory are `REC_MEM` (32) kinds, `RUN.top_rank` and the portraits'
too, so every loop over `NKINDS` stays in bounds. Everything saved as before writes and reads the first 24; the kinds from 24 go in a
new section, `LG_REC2`: a byte of how many kinds, then for each its kills by rank (u16 each), kills, hits and harms by element (a byte
each), the ranks seen and the top rank. An older page has none, and reads as none met. The achievements, the portraits and the hall
look at the first 24 only (`k < REC_KINDS`).

## 3. Steps

| Step | What | State |
|---|---|---|
| 1 | The three kinds in `KINDS` (24, 25, 26), `REC_MEM`, `LG_REC2`, the guards; old pages load, the new section reads back, the dream's log unchanged | Done, harness only |
| 2 | Side passages: the hopper's and the leech's ways (`side_dead.c`), the crab's run on its painted path (`CPATH`), vermin (`VERMIN x y RAT|LING`) | Done, harness only |
| 3 | The crypt: the crab on a drawn level, its path painted, its run along it in the 3-D view, caught or slain for its gold | Done, harness only |
| 4 | Art: each kind's pictures (crypt sprites, the bestiary's), the vermin's side tiles; the sheet | Drafts on the sheet (`media/sheets/vermin/vermin_draft.png`), awaiting the owner |
| 5 | The World Editor: vermin, the hopper's span, the crab's path and its gold, in passages and on crypt levels | Done, harness and headless browser only |
| 6 | Tests, `make asan`, docs, the editor republished | Done |

## 4. Hand-off log

**Session 1 (27 November 2026).** This plan, from the owner's choices.
- Step 1. `MOB_HOPPER`, `MOB_LEECH`, `MOB_CRAB` (24, 25, 26) in `KINDS`, each one rank, its page hidden till met (`KIND_PLACED`), placed
  only; draft pictures in `src/art/parts/vermin.c` (`SP_HOP_*`, `SP_LEECH_*`, `SP_CRAB_*`). The record's arrays, `RUN.top_rank` and the
  portraits' in memory are `REC_MEM` (32) kinds; whatever is saved as before copies the first 24 by count, not by size (the fixed part's
  `seen`, `LG_ELEM`, `LG_PORTRAIT` did by size: caught); `slayer_need` is 0 from kind 24, so no achievement bit is reached past them; the
  portraits keep to the first 24. `LG_REC2` holds the rest (`REC2_KIND_BYTES` 32 a kind, 97 bytes for three); a page without it has met
  none. Proved, harness only: the page both builds would write at each level's start (`CRAWL_PAGEDUMP`, the build before in a worktree
  with the same switch), seed 1, levels 1 to 5: the fixed part the same, every section the same but `LG_PAGE`'s time written, and
  `LG_REC2` new; that build's page loaded by both builds, 900 s: the same log, line for line; the dream, seeds 1 and 4, 1,500 s: the build
  before's lines; `CRAWL_SAVETEST`: 17 cases, none bad (the new section's numbers back as they went, and as an older page, none met).
- Step 2. In a passage (`side_dead.c`): the hopper (`SW_HOP`) bounds between its home and SPAN tiles either side (4 its own), 2.5 tiles
  high, two along, settling 0.45 s, turning at an end, a wall or a missing floor, and bites by its pixels as the others; the leech
  (`SW_LEECH`) waits under its pool (`SA_HIDE`, nothing of it to strike), bubbles 0.4 s when the caster is within five tiles and over
  the surface, leaps to meet it at the top of its arc, back into any standing poison, stranded on stone it flops home; the crab
  (`SW_CRAB`) is a coin (`SA_COIN`) till the caster is within four tiles, then runs its painted way (`CPATH x y` after its MOB line,
  `cpc` in the level) at 1.7 times its pace, on rails, and waits at its end (`SA_CORNER`); caught by its pixels touching the caster's,
  or slain, it is a kill and its gold (`GOLD IT_*` on its line) is yours. Vermin (`side_vermin.c`, `VERMIN x y RAT|LING`): rats run a
  floor and turn at walls, edges and spikes; spiderlings follow the stone tile by tile, round every corner, and drop from a ceiling (or
  a block's underside) on the caster passing under; each bites for 2 + 0.4 a level as a nudge and dies to a spell, a flask's splash or
  lightning, struck by its drawn pixels (turned as drawn). Draft tiles `ST_RAT0` to `ST_LING1`. Tested, harness only: a scripted person
  (`CRAWL_SIDESTILL`) near the crab: it ran its fourteen cells up onto a ledge and was cornered, and reached there was caught (slain,
  its gold given); the hopper traced between 136 and 200 (two tiles either side), rising 40 pixels; the leech leapt as the person
  jumped its pool and bit; rats and spiderlings bit; a spiderling round a block: top, side, underside, side, top, again; one on a
  ceiling dropped on the person under it; the dream's flasks splashed and struck spiderlings.
- Step 3. On a drawn level a coin crab (`DT_DEAD, MOB_CRAB`) is a coin on the floor till the caster is within three cells, then runs the
  cells painted for it (`DRAWNn_STEP[]`: its thing's index in `_THING` and a cell of the drawing, in order; the row's last two values,
  after the relics' looks, so a level without one writes nothing new), at 1.2 times its pace (2.4 cells a second), cell middle to cell
  middle, and waits at the last (`update_crab` in `drawn.c`, dispatched from `update_mob`). Reached within half a cell, or slain, it is a
  kill and its coins (`crab_gold`, from `kill_mob`): the crypt's drawn things carry no gold kind, so a crab there leaves coins. Its
  picture is the coin till it runs, then its two strides (`mob_sprite`). `KIND_COST`: the hopper 0.5, the leech 0.75, the crab nothing
  (the editor offers only kinds with a cost; a placed one counts against the level's budget, as the tutorial's dead). The dream shoots
  it if it stands in front, and does not chase it. Tested, harness only: the editor's own level with a crab two cells from the start
  (`tools/mapedit/fixtures/crab.c`, written by `test.js`) through `HOST_DRAWN`: it ran its six cells as the dream came in and was
  cornered; a scripted person who took over and walked to it caught it (`EVENT crab caught by player`, 14 gold). The dream's log, seeds
  1 and 4, 1,500 s, line for line the build before's.
- Step 4. The draft pictures on one sheet, `media/sheets/vermin/vermin_draft.png`: the crypt's sprites (`build/sheet vermin`), the
  vermin's passage tiles from `build/sidemock`, and the small-things passage in play. In passages the three kinds show these same
  pictures until the creatures' own side art (PLAN-side, 13).
- Step 5. The World Editor. Passages (`side.js`, `side_ui.js`): SMALL THINGS THAT MOVE among the tools (rat, spiderling, skull hopper,
  bile leech, coin crab); `VERMIN x y RAT|LING`, `SPAN n` and `GOLD IT_*` on a MOB line and `CPATH x y` after a crab's, read and written
  as the game does; a crab placed goes straight to painting its way (PAINT ITS WAY in its pane, the cells numbered, a gold thread from
  it); the hopper's span drawn as its two ends; the checks: a crab with no way (warned), a way through stone (refused), a way that skips
  tiles (warned), more than 512 tiles of ways (refused), vermin in stone (warned). Crypt levels (`core.js`, `editor.html`): a fixed dead
  given the coin crab has ITS WAY in its pane, painted cell by cell (`crabway`), drawn as a numbered gold thread; the export writes
  `DRAWNn_STEP`; the checks: no way (warned), stone or a door on it (refused), skipped cells (warned), more than 63 (refused).
  `node tools/mapedit/test.js`: the crab's export and its checks; `fixtures/side_small.txt` read back letter for letter, the reader's
  refusals. Driven in a headless Chromium: both panes, both paintings, no page errors.
- Step 6. `make asan`: the crab's level with the scripted catch, 400 s, and the small-things passage under the dream, 300 s: clean.
  Left for the owner: the pictures (the sheet), and playing them by hand.

