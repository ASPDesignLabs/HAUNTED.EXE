# 1.0.0b18.1: the trial's door, and the crypt made kinder

**Settled; step 1 built** (harness only; 7 November 2026). Written on `claude/funny-lamport-onhkrp` from main at 1.0.0b18, while the two
tracks of [`PLAN-combat.md`](https://github.com/ASPDesignLabs/HAUNTED.EXE/blob/claude/inspiring-archimedes-dbj5n6/docs/PLAN-combat.md)
build 1.0.0b19 on `claude/inspiring-archimedes-dbj5n6`. The owner gave one fault and six improvements, asked for them as an
update of their own ahead of that work, and answered every question in one round (section 12). Every number below that isn't
in the owner's words is a default, marked as one and gathered in section 13, to be changed by playing. All of it is tested in
the harness only until the owner plays it.

*The brief, in a line: a trial that can't shut its door on you; the wheels at the merchant's; a reward left in a trial's room;
bells you can tell apart; a lamp that keeps the Wraith out of a challenge for souls; a controller you can lay out yourself and
look at any time; and props that take half a cell or a quarter, where their art is that small.*

## 1. Staging, and the work beside it

- **One release, all of it together** (S2 b), as **1.0.0b18.1** (S1 a). CI takes it: its pre-release test matches `b[0-9]`, the
  Makefile's Windows file version takes the first three numbers (1,0,0,0), and the tag is `v1.0.0b18.1`. The combat plan keeps
  its numbers (1.0.0b19 to b21). The door's mend is the first step, so it could still be cut alone if the rest ran long.
- **The combat tracks.** Their section 17.2 gives each track a list of functions it alone changes. This update changes **none**
  of them. It touches a few lines of what that section calls shared, and the Wraith's movement, which track D changed on its
  branch (`wraith_cornered`); section 10 lists every place, line by line where it matters.
- **Sprites** go into the middle of the `SP_*` enum and of `crawl_art_init`'s list (after the things that want a spell), not
  at the end, where the tracks keep their marks. Sprite numbers are never saved.
- **The trial merge.** After step 1 and again before the release: main with this work merged into a scratch worktree of their
  branch, never pushed, and the conflicts written into section 14 for whoever next merges main into theirs (track C cuts their
  releases). Best landed before their joint checks (J19), so those run over it.
- **The art pass's P7 and P8** (run III's and IV's props redrawn) may come before or after this (F2 a): footprints are read
  from the art, so redrawn props bring their own.
- **CLAUDE.md** gets a paragraph of its own among the plans at its top, placed where the trial merge shows it clean.

## 2. The trial's door (the fault)

### What happens

- A trial begins the moment the caster's centre is in any cell of its room but the door's (`update_trial`: `in_trial` and one
  cell from the door).
- `tri_begin` sets the door `TRIALED` with `idle` 99, so it starts shutting at once and blocks (`open` under 0.9) within about a
  tenth of a second.
- A shutting door counts itself blocked only by the caster's *centre* in its cell (`update_doors`, line 9329). The caster's body
  is a box `BODY` (0.24) each side of the centre, so a person who has just put a step inside still has a quarter of a cell
  over the door's cell.
- Once shut, `body_blocked` refuses every step: after any small step the box still touches the door. The caster stands
  there until the sand runs out (`TRI_TIME`, sixty seconds) and the door opens again.
- The spell gates' winch has the same test (`mk_door_busy`, line 11610). The boss arch's trigger is already 0.6 cells into the
  arena, so the body is clear of it; it is tested too.

### The mend

1. **The trigger goes deeper** (T1 a). The trial begins only when the whole body is clear of the door's cell and the centre is
   1.5 cells or more from the door's centre. A peek no longer begins it. The dream's own spot inside (`G.tri_ix/iy`) is two
   steps past the door, so it still begins it.
2. **No door shuts on a body.** `door_body(d)`: the caster's box overlaps the door's cell. It replaces the centre test for
   `SEALED`, `TRIALED` and `GATED` doors in `update_doors`, and in `mk_door_busy`. Creatures as now.
3. **A safety net.** At the head of `human_move`: if the box already overlaps something that blocks, the caster is moved to the
   nearest place within a cell where it fits (searched in rings), and the debug build logs `EVENT unstuck` with what it was in.
   Not during a dash. Whatever else might ever close on a person (a rock's rubble, a secret's slide) can't freeze them.

### Tested

Reproduced first: `CRAWL_TPTRIAL=1` and a scripted step in and back (`HOST_INPUT`); on 1.0.0b18 the caster doesn't move for the
trial's sixty seconds. Then the same with the mend: the peek begins nothing, walking in begins it with the door shut behind. The
boss arch (`CRAWL_TPBOSS=1`) and a winch's gate with a person half in the doorway.

## 3. The wheels in her rooms

- The frame's wheel gate (`!G.shop`, line 17769) lets them open in her rooms; death, the tally and leaving still close them.
- **Q.** In her rooms a tap of Q holds a ware on the *press* today (line 19733). That moves to the *release*, when Q was only
  tapped (the tap branch of `wheel_input`): looking at a ware, it is held; otherwise the next rune or gift is readied, as
  anywhere. Held, the gifts' wheel. The pad's X is Q.
- The D-pad: down held, the spells' wheel; up held, the runes' and gifts'. Their taps as now (the map says *NOTHING HERE TO
  MAP*; the bar).
- While a wheel is open nothing in her rooms answers a press: no buying, no patting, no looking at a ware; the click that closes
  it buys and casts nothing (`G.click_spent`). The room goes grey under it as the crypt does.
- The dream never opens a wheel.

## 4. The trial's reward

On a win (R1 a, R2 b), two things appear on free floor in the middle of the room, in sight of its door, with a chime and a flare
of light. Lost (R3 a): nothing, as now; the curse stands.

- **The trial's hoard.** The run's own treasure (run I's chest, II's reliquary, III's strongbox, IV's idol), worth three times
  the level's chest: `3 x treasure_value(IT_CHEST)`, 750 gold on level 1, 3,600 on level 20, with Treasure Hunter and the
  caster's Treasure on top as on any pickup. It **glows**: a gold light of its own, slowly pulsing; a gold edge (`DR_EDGE`); the
  twinkle (`DR_GLINT`). It is kept out of the level's treasure count, as a secret's loot is (`Item.secret`), so the tally's share
  and the dream's sums for leaving don't change; the tally names it on the trial's line. No new sprite.
- **The trial's draught.** A potion, which replaces the win's restoring of health (R2 b): health no longer comes back the moment
  the trial is won. Defaults: it restores in full (not a potion's usual share), and it isn't taken at full health, so it waits
  in the room until it is wanted (a caption says so the first time). The win still fills bones and spirit at once, as now.
- **The dream** takes both: the hoard as treasure it has seen, the draught when it is hurt.
- Debug: `EVENT trial_reward <gold>`, `EVENT draught`.

The dream takes every trial with two thirds of its health; without the restore it may come out of one lower and die more on
that level. The checks will say (section 9).

## 5. The bells (B1 a + c, B2 b + c)

**Today.** Run I's knell (`PZ_KNELL`: THE LEAST to THE GREAT BELL, told apart by size, 0.6 to 1.11) and run II's drowned bells
(`PZ_DBELLS`: THE SHALLOWEST to THE DEEPEST, by how low they hang) are all one sprite, `SP_BELL`. The same sprite hangs in the
corners of run II's halls as decoration (`dress_level`, swaying) and makes the Toll's four small bells.

### The puzzles' bells

Four of their own (`SP_PBELL0..3`, and each swinging as it is rung), told apart four ways at once:

| | I | II | III | IV |
|---|---|---|---|---|
| Shape (a default) | a handbell on a yoke, narrow | tall and waisted | wide-mouthed | the great bell, crowned, thick at the lip |
| Bands cast round it | one | two | three | four |
| Metal (a default) | silver | bronze | iron | black iron |
| Numeral cut in it | I | II | III | IV |

- The knell's are numbered by size (I the least), the drowned bells' by depth (I the shallowest). Their sizes and depths stay.
- Defaults: the banner's and the carving's words gain the numerals ("THE LEAST (I), THE GREAT (IV) ..."), and so do the bells'
  names (THE LEAST BELL (I)), so the reading and the bells meet without guessing.
- Any one of shape, bands and numeral tells them apart alone, so a person who can't tell the metals apart loses nothing.

### The hanging bell (B1 c)

Run II's decoration gets a look of its own that can't be taken for one you ring: `SP_BELL_DEAD`, cracked through, its clapper
gone, hung askew from a broken beam, still (a default: it no longer sways). So in the whole crypt a whole bell can be rung, and
a broken one is only there. The Toll's four small bells keep `SP_BELL` (they are rung in any order).

### Sheet first

`make sheets` draws `bells.png`: today's bell at the knell's four sizes and the drowned bells' four depths beside the four new
ones, still and rung, in each run's light, and the dead bell beside today's decoration. To the owner before anything goes in.

## 6. The Wraith barred (W1 a + c, W2 a, W3 a, W4 a)

### The areas

At the level's start, once trials, puzzles and the things that want a spell are placed, `ward_make` lays out each challenge's area
as a bit in `G.ward[y][x]` (eight lamps at most):

- **a trial:** its room;
- **a puzzle:** its sealed wing, and the ground its pieces lie on: every floor cell within two steps of the wing's door or of any
  of its pieces (a default);
- **a sequence of things that want a spell** (W1 c: the tablet and its plinths, the reliquary, the ward-stone, the relay): every
  floor cell within two steps of any of its pieces (a default).

Each then grows a ring of one cell, the cells immediately round it, through floor and doorways.

### The lamp (W4 a)

- One by each challenge: a trial's beside its two red lights, a puzzle's by the wing's door, a sequence's beside its first piece.
  It hangs on a wall where there is one, so it takes no floor. Drawn in each run's look, out and lit (`SP_WLAMP*`), on the sheet
  with the bells.
- **E at it offers a soul for two minutes**, one a press, up to five banked (ten minutes). Five pips on it, burning down as the
  time goes. A caption: *THE WARD HOLDS: 6 MINUTES*. Without a whole soul, it says so. Each lamp keeps its own.
- **The clock** (W2 a) burns only while the Wraith is on the level: from its coming, and not while it is in the stone between
  being cornered and coming again. Souls offered before it comes wait. Anything that stops the level (a pocket, a menu) stops it.
- Defaults: not saved; a level begun again begins with every lamp out, the souls offered gone.

### The Wraith

- While a lamp burns, the cells of its area are stone to the Wraith: its next cell is never one of them (`wraith_barred`).
- **Its way round.** While any lamp burns it follows a field of its own (`ward_field`, made every half second) in place of
  the caster's: from you, through cells not warded; with you inside an area, from that area's edge, each edge cell seeded with its
  distance to you. So it walks to the part of the edge nearest you, and as you move it moves along the edge with you, as the owner
  asked: repelled along the border.
- One inside when a lamp is lit is driven out (as `wraith_back`, until it is outside; warded cells only on steps outward).
- At the edge: a hiss, and a violet line flares on the floor where it stands.
- Its touch reaches 1.1 cells and the ring keeps it a cell from the area, so inside you are out of reach. Standing in the ring
  yourself, you can be touched.
- Track D's `wraith_cornered` (on their branch, not main) must never land it inside: one call to `wraith_barred` in its choice of
  landing, for whoever merges (section 14).

### Seen

- The area's edge drawn faint on the floor while its lamp burns; on the map the area washed violet.
- Nothing on the status bar, which is track C's (`render_statusbar`). The lamp's pips and the caption tell the time. A glyph on
  the bar can come after their release if the owner wants one.

### The dream (W3 a)

Coming to a challenge with the Wraith come or due within a minute, and 15 spirit or more, it offers what the challenge wants
(defaults: two souls for a puzzle or a sequence, one for a trial) and one more when a lamp it works by has under a minute left,
never going under 10. It may still fire at a Wraith held at an edge, which only drives it back; if the checks show time lost to
that, the fix is a line in `find_target`, which is track D's, agreed with them first. Debug: `CRAWL_WARD=0` (no lamps), `EVENT
ward_offer`, `ward_lit`, `ward_out`, `ward_held` (the Wraith at an edge), `ward_driven`.

## 7. The controller (P1 a, P2 a, P3 a, P4 a)

**Today.** The sprint's button is the only one that moves (Esc's question, `Q_PAD`; `SLOT_PREFS` a byte; the registry's
`PadSprint`). The controls card shows for six seconds after taking over (`G.hint_t`) and never again.

### The screen: *THE CONTROLLER*

- Reached from Esc's question, where its row takes the place of the sprint's. The settings dialog's choice of sprint button stays.
- A controller drawn in the Xbox layout, with no maker's mark, its face buttons in their colours, in the stone kit
  (`docs/PLAN-ui.md`), the crypt dimmed behind. A line from each button to its job's name: the left hand's on the left, the
  right hand's on the right.
- The D-pad or the left stick moves between the buttons (the one chosen lit on the drawing); A opens the list of jobs; choosing
  one **trades**: the job comes to this button, and this button's job goes to the one that had it. No job is ever left without a
  button. B goes back. *RESTORE THE DEFAULTS*, asked twice.
- **The jobs** (today's layout is the default; the right stick's click, unused today, has nothing):

| Button today | Job |
|---|---|
| A | use (E) |
| B | Swift Dodge |
| X | the next rune or gift readied (Q); held, the gifts' wheel |
| Y | the wisps (L): tapped and held as now |
| LB / RB | the gift readied / the rune readied |
| LT / RT | the right hand (the readied rune or gift) / cast |
| left stick's click | the sprint |
| right stick's click | nothing |
| D-pad up | the map; held, the runes' and gifts' wheel |
| D-pad down | the bar; held, the spells' wheel |
| D-pad left / right | the bestiary / the Codex |

  A job with a tap and a hold goes with its button whole.
- **Fixed** (P2 a): Menu (the question, and back) and View (Tab), since the two together are the look (a default: View is read as
  part of the look); in every menu A chooses and B goes back, with the D-pad, the stick, Y and the bumpers as now; with the map
  up, the D-pad pans it and B closes it.
- **Options** (P3 a): the sticks swapped; the turn speed, from half today's to twice, by tenths; rumble on or off.
- Keyboard layouts are not in this update (P4 a).

### The look (P1 a)

**View and Menu pressed together** (within 0.15 s), anywhere a person plays: the crypt, her rooms, any menu, the map. The same
drawing with the jobs as bound now and nothing to change; the crypt waits under it; B, Menu or the two again close it. So that
neither button's own job fires on the way, View and Menu now act when let go, if the other wasn't pressed: a fifth of a second
later than today.

### Where it shows

The hints (`pk` turns a key into the button that has its job now; `pad_sprint_hint`), the controls card's pad lines and the
look all read the layout.

### Saved

- `SLOT_PREFS` grows from a byte to a small record: `"HVQ1"`, a version, a job for each of the sixteen buttons, flags (sticks
  swapped, rumble off), the turn speed; under 32 bytes. An old byte reads as today's layout with the sprint's button traded
  with the left stick's click, as `pad_keys` does it now.
- Windows: `PadMap` (binary) beside `PadSprint`; `PadSprint` is still written, from the layout, so the settings dialog shows the
  right button, and a choice there trades in the layout. The browser: `localStorage`, as now. The harness: `prefs.bin`.
- Nothing in the run's save changes.

### Code

The layout is applied at the head of `pad_keys`, where the sprint's trade is made today: the buttons' states become the jobs',
and the rest of `pad_keys` reads jobs. `PADX`, `quit_rows` and the hints with it. None of it is either track's.

### Sheet first, then tested

The screen and the look drawn with today's layout for the owner (`tools/uisheet.c`, with its fit check). Then tested with a
scripted pad in the harness: a trade, the defaults, the look from play, a menu and her rooms, a save and its reading, an old byte
read. Untried with a real controller until the owner plays it (there is none here).

## 8. Footprints (F1 b, F2 a)

**Today.** About thirty kinds of prop (tombs, altars, braziers, carts, candlestands, lecterns, the obsidian) are placed by one
function, `solid_prop`, which makes the whole cell `K_PROP`: nothing walks it, though missiles fly over and sight passes. Things
in the walls' rank, `K_PILLAR` (stalagmites, totems, mine lamps, the arenas' pillars), also block sight. Smaller things (urns,
skull piles, candles) block nothing but keep anything else out of their cell (`G.bulky`).

### The rule (F1 b: by drawn width)

- Every prop placed in a level as `K_PROP` takes a footprint from its own art: the width of its drawn foot as a share of a cell
  (its widest frame or look, the lowest eighth of its drawn rows, at the scale it is drawn). Defaults: 0.7 or more, **a full
  cell** as now; 0.45 to 0.7, **half** (the half against the wall it stands by, its long side along it); under 0.45, **a
  quarter** (the corner it stands in).
- `crawl_foot[NSPR]` is measured in `crawl_art_init` after the sprites are drawn, so redrawn props bring their own (F2 a).
- **Left as they are** (defaults): `K_PILLAR`, which hides you from sight (the Deacon's timber props are how you hide from his
  Litany); the boss arenas and her rooms, laid by hand; a gargoyle's plinth while one stands on it; a prop in the open with no
  wall beside it.

### In the cell

- The cell stays floor, with a mask of the quarters taken (`G.foot[y][x]`). A lane cell never takes one, as no solid prop sits on
  one now.
- The sprite stands at its footprint's middle, so what you see is what you bump.
- **The caster:** `body_blocked` tests the box against the quarters exactly, as against walls, so sliding along them feels the same.
- **Creatures** still go cell to cell, but into a part-filled cell they aim at the middle of its free part (`cell_pass`), so they
  visibly go round: the walk in `update_mob` (shared), `update_ally`, the Wraith's. A leap (track C's) lands as now and may brush
  one.
- **The dream** walks the lanes, which stay clear: unchanged.
- **Sight and missiles** as now.
- **Placement:** a cell with half or more free can hold a small thing in its free part: an item, a puzzle's or a trial's thing, a
  thing that wants a spell, another small prop. `bulky` and the placement helpers (`pz_free`, `mk_free`, the items' spots) learn it.
- Debug: `CRAWL_FOOT=0` (every footprint full, as today), `FOOT level n: ...` (how many of each).

### Sheet, then frames

`make sheets` draws `footprints.png`: every prop with the footprint its art gives it drawn at its foot. F1 b lets the art decide;
the sheet shows the owner what it decided. Then frames from levels 1, 15, 25 and 35.

## 9. Steps

1. The trial's door (section 2), and the first trial merge.
2. The wheels in her rooms (section 3).
3. The trial's reward (section 4).
4. **Sheets to the owner:** the bells and the lamps (`bells.png`, the lamps on it too).
5. The bells in the game.
6. The ward.
7. **Sheet to the owner:** the controller's screen and look; then the screen, the look, the layout and its saving.
8. **Sheet to the owner:** `footprints.png`; then the footprints in the game, and frames.
9. **The checks:** the dream's runs on sixteen seeds (1-31 odd) from levels 1, 11, 21 and 31 against 1.0.0b18 on the same seeds
   (`CRAWL_XPBANK` for the deep starts, as before); the heads and treasure on a level against 1.0.0b18 (six seeds, levels 1-39);
   ASan on three sessions (a trial walked into and peeked at, the ward with `CRAWL_WRAITH=30`, a scripted pad through the
   screen); CI's Windows and browser builds and smoke test on the branch, started by hand; the second trial merge.
10. **The release, at the owner's word:** the README (its features and *What has been verified*), the page, the notes (what a
    player needs to know only), `VERSION`; squashed onto main as one commit.

Each step committed, pushed and logged in section 15. While a sheet waits, the work goes on with what doesn't hang on it.

## 10. What this touches, beside the combat tracks

| Where | Whose, by PLAN-combat 17.2 | What changes |
|---|---|---|
| `update_trial`, `tri_begin`, `tri_end`, `tri_place` | neither | the trigger; the reward |
| `update_doors`, `mk_door_busy` | neither | a body, not a centre |
| `human_move`, `body_blocked` | neither (C has `update_human` and `human_walk`) | the safety net; footprints |
| the frame's wheel gate, `wheel_input`, `shop_human` | neither | the wheels in her rooms |
| `pad_keys`, `PADX`, `quit_rows`, `pk`, `pad_sprint*` | neither | the layout |
| `solid_prop`, `dress_level`, the placement helpers | neither | footprints |
| the walk in `update_mob` (`gx = m->tx + 0.5f`, line 7782) | shared | `cell_pass`, two lines |
| the Wraith's step and field in `update_wraith` | shared; D changed the lines below the step's loop | `wraith_barred` in the loop's test, the field it reads |
| the props' part of `render_sprites`, `render_floor`'s marks | shared | where a prop stands; the ward's edge |
| the `SP_*` enum, `crawl_art_init` | shared, marked at the end | added mid-list |
| `CLAUDE.md`, the README, the page | shared (C at a release) | at this release |

Not touched: `render_statusbar`, `find_target`, `choose_spell`, anything of `damage_mob`'s or `hurt_ex`'s, the per-kind branches of
`update_mob`, `update_leap`.

## 11. Saves

Only the pad's settings change (section 7). The run's save, the ledger and the mark are as they were: the ward, the reward and the
footprints belong to a level and are never saved.

## 12. The owner's answers (7 November)

S1 a (1.0.0b18.1), S2 b (all together), T1 a (1.5 cells, the body clear), R1 a (three times the level's chest), R2 b (the potion in
place of the restore), R3 a (nothing for a loss), B1 a + c (the two bell puzzles, and the hanging bells made different), B2 b + c
(shape and bands, metal, and a numeral), W1 a + c (the trial's room and the puzzle's ground and wing, with a ring, and the
sequences), W2 a (the clock only while the Wraith is on the level), W3 a (the dream uses it), W4 a (a lamp by each challenge),
P1 a (View and Menu together), P2 a (Menu and the look fixed, menus as now), P3 a (sticks swapped, turn speed, rumble), P4 a (no
keyboard layouts), F1 b (every prop by its drawn width), F2 a (the art pass's P7 in either order).

## 13. Defaults taken, for the owner to change

1. The trial's draught restores in full and isn't taken at full health; bones and spirit still fill at the win.
2. The bells' shapes and metals as in section 5's table; the numerals in the readings and the names.
3. The hanging bell is still, cracked and without its clapper.
4. A puzzle's ground: two steps round the wing's door and its pieces; a sequence's: two steps round its pieces.
5. The lamps aren't saved; a level begun again begins with them out.
6. The dream offers two souls for a puzzle or sequence and one for a trial, from 15 spirit, never under 10.
7. View is fixed with Menu, as half of the look.
8. The turn speed from half to twice today's.
9. Footprints: full from 0.7 of a cell, half from 0.45, a quarter under; pillars, arenas, her rooms, gargoyles' plinths and props in
   the open left as they are.

## 14. The trial merge

(Filled in at step 1 and step 9: each conflict with `claude/inspiring-archimedes-dbj5n6`, where, and how to keep both.)

- **After step 1** (7 November): this branch's changes from main (`git diff main`), applied with `git apply --3way` onto a scratch
  worktree of their branch at `d87d2dc` (*PLAN-combat: marks on the sprite sheet's groups*): `CLAUDE.md`, this plan and
  `src/scene_crawl.c` went in cleanly, none of the names added (`door_body`, `unstick`, `TRI_IN`) is used on their branch, and
  `make host` built it with no warnings. Nothing was committed there or pushed. Nothing for them to do yet.

## 15. Hand-off log

(Each step's entry: what was built, how it was tested, its commits, what is left.)

- **7 November. Step 1, the trial's door: built** (harness only).
  - *Built:* `TRI_IN` (1.5) and the body's test in `update_trial`; `door_body` in place of the centre test in `update_doors`
    (sealed, trialed and gated doors) and in `mk_door_busy`; `unstick` at the head of `human_move` (`EVENT unstuck`). The trials'
    comment and `CLAUDE.md` say a cell and a half now; `CLAUDE.md` has this plan's paragraph at its top and a bullet of its own
    at the end of *How CATACOMB works*.
  - *The fault, reproduced on 1.0.0b18* (seed 1, `CRAWL_LEVEL=3 CRAWL_TPTRIAL=1 CRAWL_TRIAL=1`, a scripted walk to the door, then
    0.46 s on): the Vigil began at 31.4 s with the caster's centre at x 35.84, a twelfth of a cell of its body over the door's cell
    (36,18). The door shut; S, A, D and W did nothing for the thirteen seconds tried, while the trial's dead struck. That room's
    first cell has a tomb on either side, so a person caught there could do nothing at all.
  - *Mended, the same script:* the peek begins nothing, and the caster backs out and walks on. Walking two steps in begins the
    trial at 31.8 s with the caster well inside; it moves about the room, and the shut door and the tombs stop it as they should.
  - *The safety net alone:* a scratch build (never committed) with the old trigger and the old door test put back: at the first
    press after the door shut, `EVENT unstuck from (35.84,18.50) in (36,18) kind 2 to (35.75,18.54)`, and W then walked it into
    the room.
  - *The boss arch:* `CRAWL_LEVEL=10 CRAWL_TPBOSS=1`, walked in: the fight began at x 13.6 (the arch is x 12), backing up stopped at
    the arch, strafing was free, nothing unstuck. The winch's gate goes through the same `door_body`, but wasn't tried in play.
  - *The dream:* 1.0.0b18 and this build on eight seeds (1-15 odd), 30 minutes from level 3, the same run ids: trials begun 27
    and 30, won 11 and 13, lost 5 and 0, died in 10 and 15; levels cleared 26 and 23, deaths 19 and 23. The runs part ways
    early (seed 9's level 5 came out as different rooms in the two), so then the dream started at each trial's door
    (`CRAWL_TPTRIAL=2`), sixteen seeds (1-31 odd) on levels 3-8, everything alike up to the trial: begun 271 and 269, won 36 and
    35, lost 36 and 32, died 172 and 169 (a cold deep start beside the door, tried again after each death); 81 of 96 first
    outcomes the same, the rest close fights going either way. The trigger leaves the dream's trials as they were; `unstuck`
    never fired in a dream (its walk is its own).
  - *Other checks:* `make host` and `make build/host_debug` without warnings; `make smoke` passes (1.91 ms a frame); ASan and UBSan
    clean on the peek, a walk-in with a fight, a ten-minute dream from level 3 and the safety net's scratch build; the trial merge
    (section 14) clean.
  - *Left:* nothing of step 1. Next, step 2 (the wheels in her rooms).
