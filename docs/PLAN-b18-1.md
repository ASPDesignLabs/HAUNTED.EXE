# 1.0.0b18.1: the trial's door, and the crypt made kinder

**Settled; steps 1-8 built** (harness only; 7-8 November 2026), the sheets of steps 4, 7 and 8 (`docs/b18-1/bells.png`,
`lamps.png`, `pad.png`, `footprints.png`) approved by the owner. **At the owner's word (8 November) it goes out with 1.0.0b19, not alone**: merged into
`claude/inspiring-archimedes-dbj5n6`, its step 9 folded into PLAN-combat's J19 (run over both together), its step 10 that
release's (section 15's last entry). Written on `claude/funny-lamport-onhkrp` from main at 1.0.0b18, while the two
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
- Found in the frames (built so): her cards (a ware's, hers, the Repository's ...) and the pedestals' price tags step aside while
  a wheel is up or fading (`render_shop_card`, `render_shop_tags`); they had been drawn straight through it.
- The dream never opens a wheel.

## 4. The trial's reward

On a win (R1 a, R2 b), two things appear on free floor in the middle of the room, in sight of its door, with a chime and a flare
of light. Lost (R3 a): nothing, as now; the curse stands.

- **The trial's hoard.** The run's own treasure (run I's chest, II's reliquary, III's strongbox, IV's idol), worth three times
  the level's chest: `3 x treasure_value(IT_CHEST)`, 750 gold on level 1, 3,600 on level 20, with Treasure Hunter and the
  caster's Treasure on top as on any pickup. It **glows**: a gold light of its own, slowly pulsing; a gold edge (`DR_EDGE`); the
  twinkle (`DR_GLINT`). It is kept out of the level's treasure count, as a secret's loot is (`Item.secret`), so the tally's share
  and the dream's sums for leaving don't change; the tally names it on the trial's line. No new sprite.
  *As built:* drawn full bright (`DR_BRIGHT`) with a shimmering gold edge (`DR_EDGE` with a ramp in `Draw.glint`: one line in the
  sprite drawer, the edge's colour, which the blessing and a waiting boss keep white), the treasure's twinkle quicker, and its own
  light flaring as it comes (`build_lightmap`). Not `DR_GLINT`, which is the Keymaker's and shows through walls; the shrine's glow
  as a halo was tried and dropped (it is a sigil, and hung over the chest like a mark). The tally's note is under TREASURE (*THE
  TRIAL'S HOARD, TAKEN*, or *LEFT IN ITS ROOM* in red), and the reckoning at the exit says *THE TRIAL'S HOARD STILL LIES IN ITS
  ROOM*. A caption 2.4 s after the win, once the boon's has had its time: *ITS HOARD AND A DRAUGHT LIE IN THE ROOM*. Both are
  marked by `Item.color` (`TRI_REWARD`), which only keys and scrip read; `Item` itself is left alone, since the combat branch
  changed the line above it.
- **The trial's draught.** A potion, which replaces the win's restoring of health (R2 b): health no longer comes back the moment
  the trial is won. Defaults: it restores in full (not a potion's usual share), and it isn't taken at full health, so it waits
  in the room until it is wanted (a caption says so the first time). The win still fills bones and spirit at once, as now.
- **The dream** takes both: the hoard as treasure it has seen, the draught when it is hurt.
- Debug: `EVENT trial_reward <gold>`, `EVENT draught`.
- *As built:* standing on the draught while whole gives *THE DRAUGHT WAITS: YOU ARE WHOLE* once. It cures poison too, as the win
  did. The Codex's trial page says so (and *A STEP AND A HALF IN*, which step 1 should have changed). Debug: `CRAWL_TRIWIN=s` (a
  trial won s seconds after it begins), `EVENT trial_reward`, `trial_hoard`, `draught`.

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

*As drawn* (step 4; `spr_pbell`, `bell_body`, `bell_r`, `cut_numeral`, `cv_swing`, `spr_bell_dead`; `SP_PBELL0..3`, `SP_PBELL0R..3R`,
`SP_BELL_DEAD`; `make sheets`' `b181bells`, and `docs/b18-1/bells.png` in run I's light at level 6 over run II's at 16; the
sheet tool draws each sprite at one size, so the knell's sizes and the drowned bells' depths are left to step 5's frames):
- Silver is the iron ramp's brightest steps, not the ghost ramp, which came out icy blue. So the four darken in order, I to IV,
  with bronze between: another way to tell them, besides shape, bands and numeral.
- The numerals have serifs; without them II and III read as slots in the metal. IV is inlaid pale in the black iron.
- Rung, a bell leans from its beam, its clapper lags and strikes the lip, and the ring goes out from where it struck.
- Today's bell (the art pass's, P4) is already a little broken: a crack up from a chip in its lip, its rope cut. The dead bell
  goes much further, so the two can't be confused: a wedge gone from the lip, cracked through, no clapper, one strap left, askew
  from a beam broken through, with grey threads of web. The Toll's four small bells keep today's bell, as the plan says.

*As built* (step 5): the knell and the drowned bells stand as `SP_PBELL0 + k` (I the least or the shallowest), at today's sizes
and depths; rung (a thing's clock under 0.8 s) a bell is drawn as its `SP_PBELL0R` frame, leaning, and from 0.42 s mirrored,
swinging back, then still. Their names are *THE LEAST BELL (I)* ... *THE DEEPEST BELL (IV)*, and the banner and the carving
read *LEAST (I), GREAT (IV), ...*, as does the answer kept in the documents. Run II's hanging bell is `SP_BELL_DEAD` and no longer
sways (the same draw of the dice, so the levels come out as before). The Toll's bells and `things_test` keep `SP_BELL`.

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
- *As drawn* (step 4; `spr_wlamp`, `lamp_flame`, `lamp_pips`; `SP_WLAMP0 + 3 x run + state`, 0 out, 1 and 2 lit; `make sheets`'
  `b181lamps`, and `docs/b18-1/lamps.png` with each run's three in its own run's light, levels 5, 15, 25 and 35): run I an iron
  cage under a plate pierced with the Order's cross; run II a bronze bowl on three chains from a bracket, dripping; run III a
  miner's lantern on a hook from a timber bracket, its horn panes glowing; run IV an obsidian vessel glinting violet, the flame
  seen through it. The soul's flame is the full-bright magic ramp, which is blue (the plan said pale violet), and so plainly not
  a torch's. The sprites show all five pips lit when the lamp is lit; in play the pips not yet banked will be drawn out (step 6).

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
that, the fix is a line in `find_target`, which is track D's, agreed with them first. Debug: `CRAWL_LAMPS=0` (no lamps), `EVENT
ward_offer`, `ward_lit`, `ward_out`, `ward_held` (the Wraith at an edge), `ward_driven`.

### As built (step 6)

- **Where it lives.** Outside `G`, in a static `WD` (the lamps `WLamp`, a byte of bits a cell, the fields), so nothing is added
  to `G`'s end, where the combat tracks add theirs, and a pocket leaves it as it was (its clock stops there, as the plan says).
  `ward_make` runs at the end of `start_level`'s placements (after `place_fights`), so a level begun again begins with the lamps
  out. It draws on no dice (the lamps' decorations are written with a fixed phase), so a level's creatures and treasure come out
  as before.
- **Only where the Wraith comes** (a default): no lamps on levels 1-8, a boss's level or her rooms, where a soul offered would
  buy nothing.
- **The areas**, as section 6 has them; the ring is a cell all round, corners too (through a corner a person at the edge of the
  ground could have been touched from a cell outside the ring), through floor and doorways. A puzzle's things are its pieces
  (those on the level as it is made, not a trial's); a sequence is a tablet, plinth, reliquary, ward-stone or relay and whatever
  shares its group (the relay's coil). A puzzle's ground is large (70-90 cells with its wing, on the levels looked at), and
  where it lies across a hall the Wraith goes round by another way.
- **The lamp** hangs on bare stone within four steps (no torch, no feature on that face, not a secret's wall, not on another
  decoration), the trial's two steps from the hall cell before its door if it can be, and never in the room or wing it guards
  (a first try hung a trial's inside its own room, reached through the door). E within 1.7 cells, looking at it within a third
  of a right angle, offers a soul (a puzzle's piece looked at takes the press first); the press goes no further. Its words once a level, when first seen near: *A SOUL-LAMP   E:
  OFFER A SOUL* (the pad's words through `pk`). Lit: *THE WARD HOLDS: 4 MINUTES*, or before the Wraith has come *4 MINUTES,
  FROM THE WRAITH'S COMING*; *THE LAMP WANTS A WHOLE SOUL*, *THE LAMP HOLDS NO MORE* (a sixth would pass ten minutes), *A
  SOUL-LAMP GOES OUT*, and *THE WRAITH CANNOT PASS THE WARD* the first time it is held at an edge. The approved sprites are used
  as drawn (their pips all lit); in play the pips not banked are drawn out over them, and the one burning flickers in its last
  fifteen seconds (`ward_pips`, from the end of `render_marks`). A lit lamp gives a small light of its own (`ward_lights`).
- **The Wraith**: `wraith_barred` in its step's test; `wraith_h` the field it reads (the ward's while one burns, `ward_field`
  made every half second and at once when a lamp is lit or goes out, through the hunt field's doors); `ward_step_out` drives it
  out the shortest way if a lamp is lit with it inside (one recoil, kept driven a step at a time, through a door by opening it);
  it never comes into being inside a lit ward. Held at an edge: `SFX_HISS` every three seconds and the line flaring there.
- **Seen**: `FM_WARD` on the floor's cells at the ward's edge (set inside `hz_marks`, under every other mark), drawn in
  `fm_pixel` as a faint violet dash that drifts, brighter near the Wraith while it is held; on the map the ward washed violet
  over cells seen and each lamp a mark on its wall, blue while lit.
- **The dream** looks twice a second. It offers when the Wraith has come or is due within the minute and it has 15 spirit, and
  then only from inside a puzzle's or sequence's ground, or within 2.5 cells of a trial's lamp before the trial (a default: it
  doesn't walk to the lamp; it lights it from where it works), and only while the challenge waits: two souls, one for a trial,
  never going under ten. Inside a ground whose lamp has under a minute left and the Wraith on the level, one more, five at most.
- Debug as section 6 says, and: `CRAWL_WARDLIT=n` (every lamp lit with n souls, none paid), `CRAWL_TPWARD=k` (before lamp k,
  facing it, once you take control), `CRAWL_WARDMAP=1` (each level's map with the ward's cells by lamp), `CRAWL_WARDTRACE=1` (the
  Wraith, you and the ward once a second), `WARD level n:` lines, `EVENT ward_breach` (should never come: the Wraith into a lit
  ward from outside).

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

*As drawn* (step 7's sheet, `docs/b18-1/pad.png`; `build/uisheet pad <png>`, also in `make sheets`, failing on any overflow):
a kit piece the game will draw too, `ui_pad` (`UiPad`, `UI_PAD_NAME`) at the end of `src/crawl_ui.c`, so the sheet is the screen.
- The pad is cut from the wells' stone, lit from above, in the Xbox layout with no mark on it: the bumpers a band along each
  shoulder's curve, the triggers above them, the left stick high and the D-pad low on the left, the face buttons high and the
  right stick low on the right, View and Menu small in the middle. Face buttons in their colours (A green, B red, X blue, Y yellow).
- Each job is written beside its button's name, the left hand's seven on the left, the right hand's seven on the right, with a
  leader line to the button; a job with a hold has *HELD: ...* under it. The D-pad's buttons are named by arrows. View and Menu,
  fixed, are written under the pad (*VIEW PAGES*, *MENU PAUSE*). So the colour is never the only way to tell a button: its
  name, its place and its line all say it.
- The chosen button is lit in gold on the drawing, its line gold, its row pressed with gold notches; its whole job in words at
  the foot (*X: THE NEXT RUNE OR GIFT READIED. HELD: THE WHEEL OF RUNES AND GIFTS.*).
- The options in the well's foot, two a side: *STICKS: SAME* or *SWAP*, *TURNING: 1.0* (0.5 to 2.0), *RUMBLE: ON*, *THE
  DEFAULTS* (asked twice: the foot's words turn gold, *AGAIN, AND EVERY BUTTON GOES BACK TO ITS FIRST JOB*).
- A on a button opens the jobs on a tablet over the other hand's side, each with the button that has it now; the chosen
  button's own job in gold. The foot and the hint move into the half the tablet leaves.
- The look is the same drawing with nothing chosen and no options, titled *THE CONTROLLER, AS IT IS*: *TO CHANGE THEM: MENU,
  THEN THE CONTROLLER.* The sheet shows it in run I's marble and run IV's.
- The jobs' short names (a default, to fit twelve letters with the button's name): USE, DODGE, NEXT RUNE, WISPS, THE GIFT, THE
  RUNE, READIED (the right hand: the rune or gift readied with X), CAST, SPRINT, the right stick's click nothing (a dash), THE
  MAP, THE BAR, BESTIARY, CODEX; in the list SWIFT DODGE and NOTHING are written out.

*The owner's word while it was built* (8 November): "make sure the user can bind the stick they use to move around the menus
we have ... Holding a d-pad button should cause the right stick to move through options in the ring menus we use." So:
- **A wheel is pointed with the free hand's stick**: held open by a button under the left thumb (the D-pad, the left stick's
  click, View), the right stick; under the right thumb (the face buttons, the right stick's click, Menu), the left stick; by a
  shoulder, whichever stick is pushed further. It goes by the button the job is on, so a moved job takes its hand with it. The
  wheel's hint says which: *R-STICK AIMS*, *L-STICK AIMS*, *A STICK AIMS*. Before, every wheel took the left stick, which the
  left thumb could not reach while it held the D-pad.
- **X held opens the runes' wheel**, as the screen and this plan's table had said; before, the pad pressed Q for one frame
  only, so X held only readied the next.
- **THE MENUS** (a fifth option, *MENUS: LEFT*, *RIGHT* or *BOTH*): the stick that moves through the list menus with the
  D-pad. Left at first, as before. Saved in the flags' byte (bits 2-3). The options are now three down the left (sticks,
  turning, menus) and two down the right (rumble, the defaults), and the sheet was drawn again so (`docs/b18-1/pad.png`).

*As built* (step 7, in the game):
- **The layout** (`PL`): each button's job, a job named by the button it began on, so today's layout is each button its own; the
  sticks, the turning (5 to 20 tenths), the menus' stick, the rumble. Saved at every change as section 7's record (`PL_BYTES`, 23
  bytes); 1.0.0b15's single byte reads as today's layout with the sprint traded onto that button (a byte naming View or Menu is
  let be).
- **In play** `pad_keys` hands each button's state to its job after the map's own D-pad and B, so the rest of it is unchanged and
  a job's tap and hold go with it whole. Menus keep the pad as it was but for their stick. The sticks are swapped in play only, so
  the stick that walks also leans a dodge. The turning multiplies the turning stick's turn; rumble off stops every motor a
  person's play starts.
- **View and Menu** act as they are let go; the second of the two within 0.15 s of the first opens the look (in play, in her
  rooms, over any menu, the map, the tally; the crypt waits under it, as under the console), and B, Menu alone, the pair again or
  Esc on the keys puts it away.
- **The screen** comes from Esc's question, its last row, *THE CONTROLLER* (*EACH BUTTON'S JOB, AND THE STICKS*), there while a
  pad is in, as the sprint's row was. The D-pad or the menus' stick moves over the drawing as it is laid out (two columns and the
  options under them; on TURNING and MENUS left and right change it), A opens the jobs on the button chosen and A trades, B goes
  back (from the screen to the question). View and Menu chosen only knock. The about line names the button and what its job does.
- **The hints** in play name the button a job is on now (*E*, *Q*, *L*, *CLICK*, *M*, *1-9*, *RIGHT-CLICK* through `pk`), and so do the
  controls card's pad lines (the walking and turning sticks swapped with them; a moved bestiary or codex by its button).
- **Windows**: `PadMap` (binary) beside `PadSprint`, which is written from the layout; the settings dialog's sprint choice, made
  there, is traded in as the layout is loaded. **The browser**: its page's sprint setting reads and trades within the same record.
- Defaults taken: the about line's names (*D-PAD UP*, *THE L-STICK*); the question's row only while a pad is in; a View or Menu
  chosen in the dialog for the sprint is let be (they keep their own jobs); the menus' stick left at first.

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

*The sheet as drawn* (8 November; `docs/b18-1/footprints.png`, `make sheets`' `footprints`). The measure is `crawl_foot[NSPR]`
with `crawl_prop_foot` (the widest frame or look) and `crawl_foot_part` in `crawl_art.c`, taken at the end of
`crawl_art_level`, not `crawl_art_init`: the props are drawn there, a level at a time (`props_level`, then the run's own), so a
redrawn prop brings its own. All four runs give the same widths today. What the art decided, of the props `dress_level` lays:

| Whole (0.70 and up) | Half, by its wall (0.45-0.70) | A quarter, in a corner (under 0.45) |
|---|---|---|
| the broken stone (0.72, its other look 0.78), the fire pit (0.72), the mine cart (0.81), the draped altar (0.77), the magma vent (0.83) | the round stone (0.56), the cross (0.50, its other look 0.62), the obelisk (0.53), the brazier (0.55), the crystals (0.61), the obsidian (0.56), a statue's plinth (0.48) | the candlestand (0.39), the armour stand (0.38) |

One default of mine, added to section 13's list (item 9) and shown on the sheet: **things with a use** keep their whole cell, as
pillars, the arenas and her rooms do: the Black Altar, the Black Shrine and the water cooler, and the puzzles', trials' and
hazards' things that make their cells solid. Each is faced to be used, has its flames, eyes, wisp or glint set at its cell's
middle, and is what the dream walks up to. A plinth with a gargoyle on it stays whole (section 8's own rule); the desks are whole
by their art (0.94).

*As built* (8 November; the sheet approved by the owner, who said to build it). One change from the text above, for safety's
sake: **the cell keeps its kind** (`K_PROP`) and carries the mask (`G.foot`: 1 the -x -y quarter, 2 +x -y, 4 -x +y, 8 +x +y),
rather than becoming floor. What walks reads the mask, and nothing else needs to: `blocks_move` and `hunt_pass` let the dead and
their fields through such a cell, and `body_blocked` tests the caster's box against its quarters (`foot_of`, `foot_hit`; the box,
`BODY` 0.24 each way, is no wider than a quarter, so its corners are an exact test, as against walls). The hundred-odd tests of a
floor cell elsewhere (placements, hazards, puzzles, trials, the dream's planner, `passable`) keep their old caution, as if the
whole cell were still taken, unless they have learnt otherwise.
- *Laying.* The rule lives in `solid_prop`, live only while `dress_level` lays the level (`g_foot`; `dress_level` is now that
  flag and a count around `dress_level_props`, the old body), so the arenas, her rooms, the pockets, the Black Altar and Shrine
  and the puzzles' and trials' things are never touched. Which wall, and which corner, go by a hash of the cell and the level,
  not the generator's dice, so a level is laid as it was. A quarter's corner takes a second wall if there is one, else the side
  away from its block's middle. The lit props' lights (`fire_pit`, `candlestand`, `brazier`, `crystals`, `magma`, the altars)
  stand where the prop now stands (`prop_lx`, `prop_ly`). A prop moved into its part keeps the look its cell gave it
  (`deco_look` hashes the cell's middle for it).
- *The dead.* `cell_pass` gives the middle of what a prop leaves free: the other half, or across from a quarter (0.3 from that
  corner's far sides). The walk in `update_mob` and its step aside from a spell, `update_ally` and the ordinary Wraith's
  (`update_wraith`) aim there. A gargoyle placed on a statue's plinth makes it whole again (`place_gargoyles`), and its waking
  makes it floor as before. A creature born in such a cell (raised from remains there, a sac laid, spiderlings) is set into its
  free part by `foot_settle` each frame, since `spawn_mob`, which puts it at the cell's middle, is track D's.
- *Placement.* The rooms' treasure and the potion may lie in what a prop leaves free (`part_spot`: beside a lane, by floor
  reached without a key, no door beside; `part_lay` puts it at `cell_pass`). **Not taught** (a second change from the text):
  `mk_free` and `pz_free`. Things that want a spell and the puzzles' pieces are faced and struck at their cell's middle, and the
  dream's spots for them stand before a cell; half a cell's offset would put them off those lines, for little gain.
- *Seen.* The map draws a part-filled cell's mark in its part; the debug map prints it `f`.
- *Debug.* `CRAWL_FOOT=0` (every prop whole, as before), `FOOT level n: ...` (whole, of them in the open, half, a quarter),
  `CRAWL_FOOTLIST=1` (each part-filled cell, its mask, sprite and foot), `EVENT foot_pass` (a creature into such a cell, by kind)
  and `foot_brush` (one standing in what the prop takes: never a walker, in the checks), `EVENT part_item` and `part_taken`.

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
| `collect`, `update_items`, `reckon_lines`, the tally's notes, the Codex's trial page | neither | the reward picked up, waiting, noted |
| the dynamic lights in `build_lightmap` (after the motes'), the items' part of `render_sprites` | shared (both unchanged on their branch) | the hoard's light and look |
| the sprite drawer's edge colour (one line) | shared; track C's `DR_ELEMX` is about 25 lines below | a ramp in `Draw.glint` colours `DR_EDGE` |
| `update_doors`, `mk_door_busy` | neither | a body, not a centre |
| `human_move`, `body_blocked` | neither (C has `update_human` and `human_walk`) | the safety net; footprints |
| the frame's wheel gate, `wheel_input`, `shop_human`, `update_shop`, `render_shop_card`, `render_shop_tags`, `ready_next` (a debug line) | neither | the wheels in her rooms |
| `pad_keys`, `PADX`, `quit_rows`, `pk`, `pad_sprint*` | neither | the layout |
| `solid_prop`, `dress_level`, the placement helpers | neither | footprints |
| the walk in `update_mob` (`gx = m->tx + 0.5f`, line 7782) | shared | `cell_pass`, two lines |
| the Wraith's step and field in `update_wraith` | shared; D changed the lines below the step's loop | `wraith_barred` in the loop's test, the field it reads, `ward_step_out` above it |
| the Wraith's coming in `crawl_update`, the frame's call before the player moves, `start_level`'s placements | neither | never inside a lit ward; `ward_tick`; `ward_make` |
| `hz_marks`, the `FM_*` enum, `fm_pixel`, `render_marks`' end, `render_map`, `build_lightmap` (a call) | neither | the ward's edge, pips, wash, light |
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
5. The lamps aren't saved; a level begun again begins with them out. They are laid only on levels the Wraith comes to.
6. The dream offers two souls for a puzzle or sequence and one for a trial, from 15 spirit, never under 10, from inside the
   ground (or by a trial's lamp), not walking to the lamp.
7. View is fixed with Menu, as half of the look.
8. The turn speed from half to twice today's.
9. Footprints: full from 0.7 of a cell, half from 0.45, a quarter under; pillars, arenas, her rooms, gargoyles' plinths and props in
   the open left as they are; and things with a use (the Black Altar, the Black Shrine, the cooler, the puzzles', trials' and
   hazards' things), added when the sheet was drawn.

## 14. The trial merge

(Filled in at step 1 and step 9: each conflict with `claude/inspiring-archimedes-dbj5n6`, where, and how to keep both.)

- **After step 1** (7 November): this branch's changes from main (`git diff main`), applied with `git apply --3way` onto a scratch
  worktree of their branch at `d87d2dc` (*PLAN-combat: marks on the sprite sheet's groups*): `CLAUDE.md`, this plan and
  `src/scene_crawl.c` went in cleanly, none of the names added (`door_body`, `unstick`, `TRI_IN`) is used on their branch, and
  `make host` built it with no warnings. Nothing was committed there or pushed. Nothing for them to do yet.
- **After step 2** (7 November, their branch still at `d87d2dc`): the first try conflicted in `crawl_render`, where step 2 had
  put the wheel's test on the call of `render_shop_tags`, the line above the one track C changed for C2 (`render_af_view` added
  to the `render_wards` line). Mended on this side: both call sites are back as on main, and the test sits inside
  `render_shop_tags` and `render_shop_card`, which neither track has touched. Applied again: clean, and built with no warnings.
  For whoever merges: keep `crawl_render`'s lines there as theirs; nothing of this update is on them.
- **After step 3** (7 November, their branch still at `d87d2dc`): applied clean, built with no warnings, none of the new names
  used there. Two things kept out of their way on purpose: `Item` isn't touched (their `Bone` change is the line above it, so a
  new field would have conflicted), and no draw flag was added (their C3 added `DR_ELEMX`); the hoard's gold edge is a ramp in
  `Draw.glint` read by the edge's one line, about 25 lines above their `DR_ELEMX` block in the same drawer.
- **After step 4** (7 November, their branch at `5559111`, two commits on: C4's sheet and its notes, which added sprites and
  sheet groups above their marks): applied clean, no conflict markers, `make host` and `make build/sheet` built there. The new
  sprites are in the middle of the `SP_*` enum (after the things that want a spell) and of `crawl_art_init` (after the drum),
  and the sheet's groups before `b6`, all away from their marks at the ends; the sprite numbers after them shift, which nothing
  saves.
- **After step 6** (7 November, their branch still at `5559111`): applied clean (with `--binary` for the sheets), `make host`
  and `make build/host_debug` built there with no warnings, and the held-at-the-edge test ran there as here. For whoever merges:
  their `wraith_cornered` lands the Wraith with `pick_cell_at_depth`; add `&& !wraith_barred(x, y)` to that test, as this
  update's coming of the Wraith has, so it never comes out of the stone inside a lit ward. Their `if (best < 0) {
  wraith_cornered(m, dt); return; }` after the step's loop is right as it is: held at an edge the Wraith is coming, not driven,
  so it only waits there.
- **After step 5** (7 November, their branch at `d36e0c4`, one commit on: track C's log): applied clean, built with no warnings.
  The bells touch only the knell's lines in `pz_make_run1`, one line of `dress_level` and the things' part of `render_sprites`,
  none of which they have changed.
- **After step 7's sheet** (8 November, their branch still at `d36e0c4`): applied clean, built, and both their `status` sheet
  and the `pad` one ran there. They had added to the kit too (after `ui_mark` in `crawl_ui.c`, after `ui_socket` in the header,
  a `status` mode in `uisheet`'s main, `combat` in the Makefile); this update's pieces sit at the ends of the kit's files,
  before `sheet()` in the sheet tool, its mode before main's last line, and after `uisheet all` in `make sheets`.
- **After step 7** (8 November, their branch still at `d36e0c4`): applied clean and built there, the `pad` sheet drawn there. The
  controller touches `pad_keys`, `PADX`, the old sprint byte's functions, `pad_turn`, `rumble`, `pk`, the controls card's pad lines,
  the wheel's pointing and hint lines, Esc's question, a UI state at the end of the list, a line after `con_update` and before each
  `con_render`, `win_main.c`'s slots, `tools/host.c`'s print and `web/haunted.js`'s sprint setting; none of it theirs.
- **After step 8's sheet** (8 November, their branch still at `d36e0c4`): applied clean, built there with the sheet tool. The
  measure sits before `crawl_art_level` in `crawl_art.c` and is called at its end; its externs after `crawl_glint` in `crawl.h`;
  the page before the sheet tool's `main`, its mode before `hazards`. None of it theirs.
- **After step 8** (8 November, their branch still at `d36e0c4`): applied without a conflict and built there; the merged harness ran.
  Footprints touch `solid_prop` and the lit props' helpers, `dress_level` (split around `dress_level_props`), `blocks_move`,
  `hunt_pass`, `body_blocked`, `deco_look`, `place_gargoyles`' one line, `place_items` (two lines, and `part_spot`/`part_lay`
  beside `room_cell`), `collect`'s debug line, the map's prop mark, the debug map's letter, a debug watch at the head of
  `crawl_update`, and the walk's target in four places: `update_mob`'s walk and its dodge (shared), `update_ally` and
  `update_wraith`'s movement (neither's, by PLAN-combat's own note on the Wraith), and `foot_settle` beside the debug watch at the
  head of `crawl_update`. None is in 17.2's lists (`spawn_mob`, track D's, is left alone for that reason). If either track
  changes the line `float gx = m->tx + 0.5f, gy = m->ty + 0.5f, ...` in `update_mob`, keep `cell_pass` there.

- **The merge** (8 November, at the owner's word; their branch at `2b56cdd`, this one at `a92756d`): `git merge --no-ff` into their
  branch, no conflict, as the trial merges said; built with no warnings, `make smoke` passing. The one line section 14 asked for after
  step 6 went in after it, in a commit of its own: `wraith_cornered` refuses a landing that `wraith_barred` bars.

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
  - *CI:* run 42404300163 on the branch (started by hand): the Windows build compiled, the smoke test passed.
- **7 November. Step 2, the wheels in her rooms: built** (harness only).
  - *Built:* the frame's wheel gate takes `!G.ui` in place of `!G.shop` (her rooms, never over one of her menus, which in the
    crypt return before it); the ware's hold is `shop_hold`, called from `wheel_input`'s tap branch on Q's release when no wheel
    is up and a ware is looked at (otherwise the next rune or gift, as in the crypt); `update_shop` calls `shop_human` only with
    no wheel up or fading (`WH.k`), so the press that closes one buys nothing (buying there reads the press itself, not
    `G.click_spent`); her cards and tags step aside under a wheel. Debug: `CRAWL_TPPED=k` (before her pedestal k, facing it,
    once you take control), `EVENT shop_hold`, `EVENT ready_next`.
  - *Tested* (seed 1, `CRAWL_TOTEM=1111 CRAWL_RUNES=2`, before level 1 and level 5): Q tapped at a ware holds it on letting go
    (a quarter of 1,020 taken; tapped again, nothing more); Q held opens the gifts' wheel and letting go takes from it, no ware
    held; E and a click under a wheel, and E just after letting go, buy nothing, E two seconds later buys; the pad's X holds, the
    D-pad's down and up held open the spells' and the gifts' wheels, A under one buys nothing, B puts one away; Q held over her
    menu and over the wardrobe opens nothing, and after Esc it opens the wheel. Frames before, under and after each wheel.
  - *Not changed, seen on the way:* in the crypt as on 1.0.0b18, the gifts' wheel shortens the status bar's HEALTH to HEAL...,
    and a toast or a caption can be drawn over the wheel. Older than this work; left for the owner.
  - *The dream:* untouched (every change is behind `G.player`; it never opens a wheel), so no runs.
  - *Other checks:* both builds without warnings; `make smoke` passes (1.72 ms a frame); ASan and UBSan clean on three sessions
    in her rooms (keys, pad, menus); the trial merge clean after the mend above.
  - *Left:* nothing of step 2. Next, step 3 (the trial's reward).
  - *CI:* run 42406057780 on the branch (started by hand): the Windows build compiled, the smoke test passed.
- **7 November. Step 3, the trial's reward: built** (harness only).
  - *Built:* as section 4 says, with its *as built* notes: `tri_reward`, `tri_reward_spot`, `tri_reward_at`, `TRI_REWARD`; the
    win's health restore gone (bones and spirit as before); the draught in `collect` and waiting in `update_items`; the hoard's
    look and light; the captions; the tally's note, the reckoning's line, the Codex's page; `CRAWL_TRIWIN`.
  - *A person* (seed 1, level 3, the Vigil, `CRAWL_TPTRIAL=1 CRAWL_TRIWIN=3`): on the win the hoard (1,050, three of level 3's
    chests at 350) lay at (33,17) and the draught at (34,16), health stayed at 96 and the treasure count at 0 of 31; walked onto,
    the hoard gave 1,050 gold and the draught made 96 into 100. Won at full health (`CRAWL_TRIWIN=1`), standing on the draught
    left it lying with *THE DRAUGHT WAITS: YOU ARE WHOLE*. Frames: the hoard full bright and edged in shimmering gold beside
    the draught; *ITS HOARD AND A DRAUGHT LIE IN THE ROOM* on time; the Codex's page on eight lines with room below it; and,
    after a sleepwalk to the level's end (`dump`, since a `shot` run had parted from the `apl` one by then), the tally's
    *THE TRIAL'S HOARD, TAKEN* under TREASURE. The reckoning's line for a hoard left lying was read, not seen.
  - *The dream from each trial's door* (sixteen seeds, levels 3-8, 240 s, the same run ids; step 1's build, which the dream
    can't tell from step 2's, against this): begun 298 and 300, won 37 and 38, lost 35 and 36, died in 193 and 194; 38 rewards,
    37 hoards taken, 36 draughts drunk. Deaths later on the level after a win, 11 and 15: every one in the same life came after
    the draught, which the dream drank one or two seconds after the win; the rest were later lives (the level begun again).
  - *The dream from level 3* (sixteen seeds, 30 minutes, the same builds): levels cleared 46 and 48, deaths 38 and 31, trials
    begun 53 and 46, won 20 and 20, lost 6 and 6, died in 25 and 18; 20 rewards, 18 hoards taken, 15 draughts (it leaves a
    draught above four fifths of its health, as it does any potion). No harm in the noise.
  - *Other checks:* both builds without warnings; `make smoke` passes (1.89 ms a frame); ASan and UBSan clean on a person's win
    with both taken and the Codex opened, a win at full health with the draught left, and a ten-minute dream from level 3 with
    `CRAWL_TRIWIN=8`; the trial merge clean.
  - *For the owner's eye:* whether the gold edge reads as "worth more" on a real screen, in motion. It can be made stronger.
  - *Left:* nothing of step 3. Next, step 4: the sheets of the bells and the soul-lamps, to the owner first.
  - *CI:* run 42409578815 on the branch (started by hand): the Windows build compiled, the smoke test passed.
- **7 November. Step 4, the sheets of the bells and the soul-lamps: drawn, and sent to the owner.**
  - *Drawn:* sections 5 and 6's *as drawn* notes. The sprites are made at start-up but nothing places them yet (step 5 puts the
    bells in the puzzles and the dead bell in run II's halls, step 6 the lamps).
  - *Changed on the way, from my own look at the first drawings:* silver from the ghost ramp (icy blue) to the iron ramp's
    brightest steps; serifs on the numerals; stronger bands; the dead bell's missing piece cut down from a third of the bell to a
    wedge out of the lip; its cobwebs from gold dots to grey threads; the lamps' page put together with each run's lamps in that
    run's light.
  - *Checks:* both builds without warnings; `make smoke` passes (1.81 ms a frame); the trial merge clean (section 14).
  - *Waiting on:* the owner's word on both sheets. While they wait, step 6's logic (the ward's areas, the Wraith's way round,
    the offering) and step 7's controller sheet can go on; neither hangs on these drawings.
  - *CI:* run 42411396362 on the branch (started by hand): the Windows build compiled, the smoke test passed.
  - *Approved* by the owner the same day, who then asked for step 6's logic before step 5.
- **7 November. Step 6, the ward: built** (harness only).
  - *Built:* as section 6 says, with its *as built* notes: `WD`, `WLamp`, `wraith_barred`, `wraith_h`, `ward_field`,
    `ward_step_out`, `ward_make`, `ward_hang`, `ward_spread`, `ward_ring`, `ward_offer`, `ward_tick`, `ward_marks` (`FM_WARD`),
    `ward_pips`, `ward_lights`, the map's wash; debug `CRAWL_LAMPS=0`, `CRAWL_WARDLIT`, `CRAWL_TPWARD`, `CRAWL_WARDMAP`,
    `CRAWL_WARDTRACE`. The names `warded`, `ward_cracked` and `ward_give` in the event log and the code are still the warded
    creatures' (1.0.0b16), and `ward_shards` the Bone Ward's: only `ward_offer`, `ward_lit`, `ward_out`, `ward_held`,
    `ward_driven` and `ward_breach` are the lamps'.
  - *Laid* (seed 3, levels 9-34): a lamp by each trial, puzzle and sequence from level 9, none on a boss's level; grounds of 16-48
    cells for a trial, 69-91 for a puzzle and its wing, 17-23 for a sequence. Two faults found by the map and mended: a trial's
    lamp hung inside its own room, reached through the door (now never in the room or wing it guards); and the Wraith's way out
    counted walls as outside, so a lamp lit with it inside sent it at the stone and it wandered (now through the hunt field's
    floor and doors only).
  - *A person* (seed 3, level 12, three cisterns, `CRAWL_TPWARD=2 CRAWL_WRAITH=12 CRAWL_SOULS=20`): two souls at the puzzle's
    lamp (*4 MINUTES, FROM THE WRAITH'S COMING*); the Wraith came round to the ground's nearest edge, 6.3 cells off, hissed, and
    stood there for the rest of the run, following along the edge as the person walked about inside; with no soul offered it
    reached them and killed them in 30 s. Lit with the Wraith three cells inside: one recoil, out in three steps, held at the
    edge. One soul: out 120 s after the Wraith came (the clock waited for it), touched ten seconds later. E while turned away
    offered nothing; three Es before the trial's lamp banked six minutes, one at a time, its pips going on one by one. Frames:
    the lamp lit and its pips, the ward's faint line on the floor, the wash on the map, *THE WRAITH CANNOT PASS THE WARD*.
  - *The dream* (lamps lit from the start, `CRAWL_WARDLIT=5`, the Wraith at 20 s, nine level starts): no `ward_breach`, holds
    at the edge in four. On its own (`CRAWL_SOULS=40 CRAWL_WRAITH=90`, sixteen level starts, levels 12-22) it lit lamps on
    fifteen, and the Wraith was held at an edge on eight. *Sixteen seeds (1-31 odd), 30 minutes from level 9* (gear 3, `CRAWL_XPBANK=9450`, the same run ids;
    step 4's build against this): final levels a median 12 and 12, levels cleared 43 and 39, deaths 4 and 3 (all on level 10),
    trials won 42 and 43 of 44, puzzles 9 and 9. It lit two lamps in all (its spirit is seldom 15 when the Wraith is due) and the
    Wraith was never held from it. The Wraith touched it 2 and 26 times: 25 in one run, a stall on level 11 with no lamp lit,
    where the dream stood firing fire bolts at the Wraith 1.1 cells off and each struck a bell puzzle's bell between them
    (*THREE BELLS wants FROST got FIRE*), so it was never driven back. That is on main too (the bells block shots unless rung,
    and the dream's aim is track D's `find_target`); left for the owner. Each build had other stalls of its own (1 and 2).
  - *Other checks:* both builds without warnings; `make smoke` passes (1.75 ms a frame); ASan and UBSan clean on three sessions
    (a person driving the Wraith out, walking inside, the map open, the lamp out; the dream from level 9 with the Wraith at 30 s
    and souls to spend; the dream with lamps lit from the start through two pockets); the trial merge clean (section 14).
  - *For the owner's eye:* whether the line on the floor is too faint to find, and whether the lamps' words come at the right
    moment. Both are a number each.
  - *Left:* step 5 (the bells in the game), then step 7.
  - *Mended after* (8 November): the lamps' switch was first `CRAWL_WARD=0`, which is already the warded creatures' (1.0.0b16
    step 7), so it took both away at once; it is `CRAWL_LAMPS=0` now. No numbers above used it.
  - *CI:* run 42415459761 on the branch (started by hand): the Windows build compiled and its smoke test passed; the browser's
    build compiled and its smoke test in Chromium passed.
- **7 November. Step 5, the bells in the game: built** (harness only).
  - *Built:* as section 5's *as built* says. Debug: `CRAWL_PZTHINGS=1` (each of a puzzle's things and where, on its `PUZZLE`
    line's level), `CRAWL_TPAT=x,y,deg` (stand there facing that way once you take control, for frames).
  - *Frames* (seed 3 level 6, the knell; seed 1 level 14, the drowned bells; seeds 1 and 3 level 19, the dead bell): the four
    told apart side by side in run I's light (III wide-mouthed iron and I the silver handbell on its yoke; II tall bronze and IV
    black iron with its pale numeral) and in run II's, the drowned four at their depths with IV's numeral clear; bell II rung,
    leaning, swinging back mirrored, still; the dead bell askew from its broken beam behind a reliquary. In an unlit stretch IV's
    black iron is hard to see but its numeral shows; a person comes close to ring it anyway.
  - *The dream:* the puzzles' logic is unchanged (the bells' values, groups and sounds), and so is the hanging bell's draw of
    the dice. Sixteen starts (seeds 1-15 odd, the knell on level 6 and the drowned bells on 14, 240 s, the same run ids), step 6's
    build against this: 13 solved in each, run for run.
  - *Other checks:* both builds without warnings; `make smoke` passes (1.74 ms a frame); ASan and UBSan clean on a person ringing
    the knell, the dream at the drowned bells, and a level 19 with dead bells; the trial merge clean (section 14).
  - *Committed* as `6867e7f` (the code; these notes followed in the next commit, the first try at them having failed).
  - *Left:* step 7, the controller's sheet to the owner first.
  - *CI:* run 42417307380 on the branch (started by hand): the Windows build and its smoke test, the browser's build and its
    smoke test in Chromium, all passed.
- **8 November. Step 7's sheet, the controller's screen and the look: drawn, and sent to the owner.**
  - *Drawn:* section 7's *as drawn* note; `ui_pad` in the kit, its declarations at the end of `crawl_ui.h`, the sheet's `pad`
    mode in `tools/uisheet.c` and a line in `make sheets`. Six panels: X chosen; A's list, the map chosen to trade; an option;
    the defaults asked twice; the look; the look in the Deep's marble.
  - *Changed on the way, from my own look at the first drawings:* the fit check found the left hand's hold lines off the stone,
    the hint below the slab's moulding and the trade list over the words under it (the foot now moves into the free half); the
    shoulders were hats on a box and now follow the shoulder's curve; the leaders were too faint; View and Menu cut in the
    stone read badly and are inlaid.
  - *Checks:* both fit checks pass (the pad's and the Alignment's); both builds without warnings; `make smoke` passes (1.81 ms
    a frame; the game links the new piece but doesn't draw it yet); the trial merge clean (section 14).
  - *Waiting on:* the owner's word on the sheet. Nothing of the screen is in the game yet.
  - *CI:* run 42433030953 on the branch (started by hand): the Windows and browser builds and their smoke tests passed.
  - *Approved* by the owner, who asked for it to be built.
- **8 November. Step 7, the controller: built** (harness only; a scripted pad, no real controller).
  - *Built:* as section 7's *as built* note says, with the owner's word given while it was built (the wheels by the free hand,
    X held, the menus' stick).
  - *Tested with a scripted pad* (seed 3, level 1, `HOST_SLOTS` for the prefs): taken over with A held; Menu let go opened the
    question; its last row opened the screen; X chosen, the jobs opened on NEXT RUNE, THE MAP chosen and traded (*X TAKES THE
    MAP; D-PAD UP TAKES NEXT RUNE*), saved as 23 bytes and drawn so; back with B twice; X then did the map's job in her room
    (*NOTHING HERE TO MAP*) and in the crypt (the map up, the D-pad still panning it, B closing it); the controls card read *X
    TAPPED: THE MAP*. View and Menu together opened the look in the crypt, over the wardrobe in her room, and the pair again (in
    either order) put it away, as did B; View alone opened the wardrobe as it was let go, Menu alone the question. Rumble off,
    sticks swapped and turning 1.5 saved; the defaults asked once (in gold), restored on the second press. An old prefs byte of 3
    read as Y sprinting and the left stick's click taking the wisps. In play: the sticks as they came walk on the left and turn on
    the right (3.0 radians in a second); swapped, the other way about; turning 2.0 turned 6.0 radians in a second, 0.5 turned 1.5.
    A fall with rumble on started the motors (two `RUMBLE` lines), with it off none.
  - *The owner's additions, tested:* the spells' wheel held open on the D-pad's down took the slot the right stick pointed at,
    and with the left stick pushed instead took only the slot already lit; X held opened the runes' wheel and the left stick
    pointed it, the right being let be; X tapped still readied the next (*ready_next gift*). The question moved by the left stick
    only (menus LEFT), the right only (RIGHT), either (BOTH).
  - *Faults found on the way and mended:* the look closed by B had marked Menu spent, so the next Menu alone did nothing; with the
    look up, View then Menu didn't close it; the question's note was wider than its headstone (the debug build's fit check). And one
    older than this work: the wheel's hint lines touched the words in its middle (*IT RISES BY ITSELF*) by a row; they sit six
    pixels lower now.
  - *Other checks:* both builds without warnings; every about line's wrapping checked against the kit's rule for every button and
    job (two lines, and five in the list's half); `make smoke` passes (1.76 ms a frame); ASan and UBSan clean on two sessions (the
    screen, a trade, the options, the look and View and Menu alone, the sticks swapped; then the options' grid, the wheels by the
    D-pad and by X, and a menu on the right stick); the sheets' fit checks pass; the trial merge clean (section 14). The Windows
    build (`PadMap`) and the browser's page only compile in CI here: no MinGW in this container.
  - *Untried:* a real controller (none here), the Windows registry by hand, the browser's settings panel by hand.
  - *CI:* run 42436508177 on the branch (started by hand): the Windows build (with `PadMap`) and the browser's compiled, and both
    smoke tests passed.
- **8 November. Step 8's sheet, footprints: drawn** (for the owner's eye; nothing of it in the game yet).
  - *Drawn:* `docs/b18-1/footprints.png` (`build/sheet footprints`, and in `make sheets`): every prop `dress_level` lays, each with
    its foot on a ruler marked at 0.45 and 0.70, the share, the verdict and the cell from above with the part it takes; then
    those left as they are, with their feet for interest. The measure (`crawl_foot`, `crawl_prop_foot`, `crawl_foot_part`) is in
    `crawl_art.c`, at the end of `crawl_art_level` (section 8's *sheet as drawn*). Five props come out whole, seven half, two a
    quarter.
  - *A default of mine, to be confirmed with the sheet:* things with a use keep their cells (section 8's note).
  - *Other checks:* the sheet and the harness build without warnings; `make smoke` passes (1.75 ms a frame); the trial merge clean
    (section 14).
- **8 November. Step 8, footprints: built** (harness only), the sheet approved by the owner.
  - *Built:* as section 8's *as built* note says. Two changes from section 8's text, both for safety: the cell keeps its kind
    (`K_PROP`) and only what walks reads the mask; and `mk_free` and `pz_free` were not taught, only the rooms' treasure and the
    potion. The frames (`docs/b18-1/footprint-frames.png`: a candlestand and a cross on level 1, an obelisk on 15, a brazier on 25,
    obsidian on 35, each with every prop whole on the left and its footprint on the right, from the same place) were taken
    standing beside each prop.
  - *Laid* (seed 3): level 1, 15 whole (6 of them in the open), 11 half, 1 a quarter; level 15, 10, 29 and none; level 25, 7, 8
    and 4; level 35, 11, 65 and none (its halls' obsidian).
  - *Tested by hand in the harness* (seed 3, level 1, a scripted pad): walking east into a cell whose east half a cross takes, the
    caster stopped at x 35.23, its box against the prop's half (34.76 with `CRAWL_FOOT=0`, against the whole cell); beside a
    candlestand's quarter it stopped at 39.23, and a step lower walked past it to the wall (39.74). A person walked to a coin lying
    in a stone's free half (seed 5) and took it. Standing in a prop's cell and handing back to the dream (Esc, sleepwalk), the
    dream stepped onto its lane at once and went on.
  - *The dead:* two dream runs from level 1 (500 s) and two from level 31 (700 s): creatures went into part-filled cells 4, 16,
    118 and 79 times (skeletons, bats, hounds, hunting spiders, ghouls, knights, imps, deacons, the Sexton and the Lightless among
    them); none that walks stood in a prop's part. The few `foot_brush` lines were a spectre (through walls), web spiders on
    their cells' edges and a hunting spider overhead. Neither run from 31 died.
  - *A fault found on the way and mended:* a prop moved into its part could come out in its other look, since a look is chosen by
    a hash of where it stands; it now keeps the look its cell gave it.
  - *Dream runs* (the same build, footprints on against `CRAWL_FOOT=0`, seeds 1-15 odd, 50 minutes each; from 31 with gear 8 and
    `CRAWL_XPBANK` 77658): from level 1, 53 levels cleared and 6 deaths (47 and 11), a level's median time 357 s (361); from 31,
    50 cleared and no deaths (49, none), 372 s (377). One `STALL` line from 1 either way, two from 31 (four). Inside the noise of
    eight seeds, and no worse. Creatures went into part-filled cells 348 times from level 1 and 2,597 from 31. The rooms' treasure
    and the potion lay by a prop 35 times from each start, and the dream took 18 and 17 of them, against three in four of all
    treasure: it heads for them by their lane as for any other, so the gap is where it chooses to walk (and each run's last level
    is never finished); a person walked to one and took it.
  - *A second fault found and mended:* the runs' `foot_brush` lines were mostly spectres and web spiders, with bats, a gargoyle
    and hunting spiders flying and ghouls just after a leap, all as they should be; but one was a skeleton a deacon had raised
    from remains lying in a prop's cell, stood at the cell's middle, since `spawn_mob` puts every newborn there. `spawn_mob` is
    on track D's list (PLAN-combat 17.2), so it is left alone: `foot_settle`, at the head of `crawl_update`, sets anything
    standing exactly at a part-filled cell's middle (only the newly born can) into its free part. The same run again, with the
    same run id, passed that moment with the skeleton in the free part.
  - *Other checks:* both builds without warnings; the trial merge clean and built there (section 14); ASan and UBSan clean on three
    sessions (a person walking into and along a half and a quarter and handing back to the dream, on level 1; the dream from level
    31, 400 s; a person roaming level 35's obsidian halls); CI run 42440663924 on the branch (started by hand): the Windows build
    and the browser's compiled and both smoke tests passed (before `foot_settle`). With `foot_settle`: `make smoke` passes (1.91 ms
    a frame) and a fourth ASan session (the dream from level 31, 300 s, 104 passes) is clean; CI run 42441922401 on the
    final state passed (both builds and both smoke tests).
- **8 November. Merged into PLAN-combat's branch, to go out with 1.0.0b19** (at the owner's word; by track D's agent). Section 14's
  last entry has the merge. Step 9's checks are J19's now: the dream's runs over both together against 1.0.0b18 on the same seeds
  (PLAN-combat's section 19), and ASan on this plan's three sessions over the merged build (a trial peeked at and walked into, the
  ward with the Wraith at 12 s, a scripted pad opening the look): clean, as was a dream from level 21 (600 s, the Wraith come); the runs over
  both are under way as this is written. The heads and treasure on a level
  (six seeds, levels 1-39) were not counted again. Step 10 is 1.0.0b19's release: the README, the page and the notes carry this
  plan's parts already. Anything more built on `claude/funny-lamport-onhkrp` needs merging again.
