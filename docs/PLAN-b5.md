# 1.0.0b5: a status bar that steps aside, and wisps that walk the crypt

**Status: agreed (23 October 2026); all seven steps built (the status bar and the mini bar; wisps in the world; the L menu; the Black Altar's menu and blessing; the autopilot's wisps and altars; the console; the runs, previews and release text), tested in the harness only. Released as 1.0.0b5 at the owner's word (24 October 2026).** The owner asked for four things after 1.0.0b4,
answered the questions below, and left the smaller choices to defaults, marked *default* here so
they can be changed.

## What the owner asked for

1. The status bar should go away after a while, so it isn't over the crypt all the time. It comes back
   when you lose health, when you change spells (the wheel or 1 to 9), and when you tap H.
2. The will-o'-wisp leaves the hand. Wisps become things in the world that fly along the way to where
   they lead, in their target's colour, lighting the crypt, with a tail you can follow round corners
   when they are out of sight or you are busy fighting. A wisp costs a soul to cast; the Black Altar's
   are free.
3. Tapping L opens a menu of the wisps you can summon and what each costs, with a toggle to make L
   summon that one straight away. With the toggle on, holding L opens the menu again.
4. At a Black Altar, 25 souls bless your exit wisps for the rest of the level: as one goes, it turns
   the undead within about four cells.
5. *(Asked with step 2.)* A console on the key left of 1 (~), flying out from the top: a greeting, the
   date and time, the program's name and version, five rows of text and a row to type in, scrolling up
   as you go. Every debug switch becomes a cheat code beginning CATA, in the crypt's manner, like the
   codes of Dark Forces. While it is open the game and its sounds stop, and the music plays on. Using
   any code stops achievements and saving until the program is started again. The owner reviews the
   codes before any of them work.
6. *(Asked after step 2.)* The hand shows only when casting, to open up the view. Built at once: it
   comes up for a cast (or to reach for a ware), stays up 0.9 s ready for the next, then sinks out of
   sight over 0.3 s (`G.hand_base`, `G.ready_t`). The autopilot already lowered its hand between casts.

## Decided (23 October 2026)

- **The status bar** slides down out of sight after 5 s with nothing to show, over about a third of a
  second, and the crypt fills the screen; it slides back up when called. It always stays up in the
  merchant's room.
- **The mini bar.** Gathering treasure, taking a key, and gaining or spending souls don't bring the
  full bar back: they show the small readout the autopilot uses (health, gold, spirit, keys) at the
  bottom left for about 3.5 s.
- **Wisps run to their target** at their own pace and hover there until you arrive. You follow the
  tail.
- **Key wisps need Keymaker.** They are in the L menu only while you wear a Keymaker piece. Anyone's
  exit wisp still fetches the key the exit needs first, as now.
- **Waking an altar still costs 10 souls**; its violet wisps to each secret then cost nothing.
- **Turning the undead:** within about four cells of a blessed exit wisp they flee, as from the Dread
  rune, while it is near and for a few seconds after. The Wraith is driven back too.
- **The autopilot** summons an exit wisp when it sets off for the exit, and wakes altars and buys the
  blessing when it can afford them.

## What the game does today (1.0.0b4)

- The bar is drawn over the bottom 44 rows whenever a person plays (`render_statusbar`); the crypt is
  drawn full height under it, so hiding the bar shows more of the crypt with no change to the view.
  The autopilot's readout already comes and goes (`render_readout`, `pop_readout`).
- H matters only while the autopilot plays: holding it three seconds takes control (`app.c`). While a
  person plays, H does nothing yet.
- The wisp kindles on the back of the hand after ten souls on a level (`WISP_SOULS`, `update_wisp`,
  `render_wisp`, its tail `wtx`/`wty`). It leans along a BFS route (`wisp_goal`, `build_route`,
  `wisp_heading`) to the exit, or first to the key the exit's vault needs (`past_vaults`); Keymaker's
  L (`wisp_choose`) turns it to each key; an awake altar turns it violet towards the nearest secret.
  Its light pools four cells round (`light_dynamic_ex`), shows shades, and drives the Wraith back
  close by. The autopilot raises the hand to carry it.
- Keymaker's keys glint through the stone only while the wisp is lit.

## The design

### The status bar

- `G.bar` slides between shown and hidden (0 to 1 over 0.3 s); `G.bar_t` counts down from 5 s and is
  reset by: losing health, a spell changed (1 to 9, the wheel), a tap of H, opening Tab or B, and
  anything that already shakes the face (a hit, a grin). In the merchant's room it never counts down.
- Hidden, the bar's 44 rows show the crypt. The hand, captions, the merchant's card and the toasts keep
  their places *(default: nothing moves with the bar, so nothing on screen jumps)*.
- **The mini bar** is `render_readout`, now also in player mode, popped (`pop_readout`) by treasure,
  a key, and souls gained or spent, and drawn only while the full bar is away.
- **Tapping H** while you play shows the bar (and tapping H while the autopilot plays pops its
  readout, on top of the hold bar it already starts) *(default)*. A tap is a press shorter than the
  hold; holding H while you play does nothing more.

As built in step 1: the bar is up when a level or the merchant's room opens, and while you are dead.
Losing health in any way (poison too) brings it back, as do a spell chosen, a tap of H and Tab or B.
The mini bar comes up for souls gained or spent and for whatever already showed the autopilot's
readout (treasure, a key used, Mend, Soul Drain's healing, a shade's touch); it shows only once the
bar is fully away. While H is held during the screensaver, the app sends the scene `CTRL_PEEK`: the
readout comes up and rises above the HOLD H plate, and settles back when H is let go. A readout that
is up already now stays up when something pops it again, instead of sliding in afresh.

### Wisps in the world

- **A wisp is a thing in the world** (`Wisp`: position, target, kind, colour, pace, a trail), drawn as
  a glowing sprite with a flickering core, lighting about four cells round as the hand's did. Kinds:
  **exit** (the exit runes' blue), **key** (the key's own gold, silver or crimson) and **altar**
  (violet). A blessed exit wisp wears a pale gold halo *(default)*.
- **It flies** from the caster's hand along the BFS route to its target, at about one and a half times
  walking pace *(default)*, through open doors and round corners, rising and falling a little as it
  goes. It **hovers at the target** (the exit's doorstep, the key, the cracked wall) until you come
  within about a cell and a half, then flares and goes out. An exit wisp that meets a locked vault on
  the way goes to the key first, as now.
- **Its tail** is a line of motes it leaves along its path, which hang in the air and fade over about
  ten seconds *(default)*, so a turn it took out of sight still shows. The motes are drawn as small
  glowing points in the crypt, nearer ones larger.
- **One summoned wisp at a time**: casting another sends the first out *(default)*. The altar's violet
  wisp is apart from it and can be out at the same time.
- **What it does to the crypt**: its light shows shades, and the Wraith shrinks back from it within
  three cells, as from the hand's wisp. Creatures otherwise ignore it.
- **Gone:** the hand's wisp, its tail on the hand, and the ten-souls rule (`WISP_SOULS`). The tally's
  and the readout's wisp mentions go with them.
- **Keymaker** now makes keys within its reach glint through the stone all the time, not only while a
  wisp is lit *(default)*, and puts key wisps in the L menu.

As built in step 2: a wisp is a `Wisp` in `G.wisps` ([0] the one you summon, [1] the altar's), with its
own field of steps to its goal (`wisp_route`: floor and doors, but no door of a vault you have no key
for). It flies at 4.5 cells a second, looking up to three cells down the way where the line is clear
(`wisp_line`), slows as it arrives, and hovers; the motes it leaves every fifth of a cell hang for 12 s
and thin out as they go, drawn as small glowing points, larger near you, hidden behind walls and left
out when they are overhead. Its body is the hand wisp's flame drawn in the world, licking back the way
it came. An exit wisp still fetches the key the exit needs first, and goes on to the exit once the key
is taken; it goes out when you come within a cell and a half of the doorstep. A key wisp goes out when
the key is taken or you reach it. The altar's wisp rises from the altar when you wake it (still 10
souls, in step 2 by E as before) and goes from secret to secret, out when the last is open. Until the L
menu (step 3), a tap of L casts an exit wisp for a soul. Without the hand's wisp the caster no longer
carries a light of its own past ten souls, which leaves the autopilot's crypt darker and its shades
harder to see until it casts wisps in step 5; the runs in step 7 will say what that costs. In the
harness a test player that only follows the wisp cleared 18 of 18 levels (six seeds, levels 2, 6 and 9),
fetching up to three keys on the way, and followed the altar's wisp to every secret on three seeds.

### The console (~)

*The codes are approved (23 October 2026); built in step 6.*

- **Opening it.** The key left of 1, wherever ~ is printed on it, as in Quake, while you play CATACOMB.
  It flies down from the top over a quarter of a second and goes back the same way; Esc or the same key
  closes it. A black plate with an iron rim, in the palette's own stone, bone and gold.
- **What it shows.** On opening, a greeting from the crypt (one of a few), the date and the time of day
  (*FRIDAY 23 OCTOBER 2026, A QUARTER PAST TEN AT NIGHT*), and the program's name and version
  (*HAUNTED.EXE 1.0.0B5, CATACOMB*). Five rows of text, then the row you type in with a blinking cursor;
  old lines scroll up and out. Letters and numbers, Backspace, Enter to try a code; Up brings back the
  last one *(default)*.
- **While it is open** the crypt stops, as it does under Tab, and so do its sounds, footsteps, fires and
  the rest; the music plays on.
- **The price.** The first code that works says so (*THE DEAD TAKE NOTE. NO HONOURS AND NO GRAVES WHILE
  THIS LASTS.*). From then until the program is closed, no achievement is earned, fleeing doesn't save
  your progress, and no grave can be carved. A word it doesn't know: *THE CRYPT DOES NOT KNOW THAT WORD.*
- **Typing in the harness.** `HOST_INPUT` gains a way to type a word, so the console can be tested.

**The codes.** Every debug switch that matters in play has a code beginning CATA, in the crypt's manner,
with a number or a creature's name on the end where it needs one, and one of them lists the rest in
the console. The owner approved the list on 23 October 2026 and wants it kept secret: it is not written in
the repo, the README or the page, and the source keeps each code only as a hash. The README and the
page say no more than that secrets were added. The list is with the owner. Left out as tools for the
harness: printing the map, holding the hand up for pictures, the BFS9000 on anything, HAUNT.EXE's acts
and the Windows shell's test switches.

As built in step 6:

- **The key** is found by where it is, not what it prints (scan code 0x29, `KEY_CONSOLE`), so it is the
  key left of 1 on any layout. Typed letters and digits come from Windows as characters (`WM_CHAR`, into
  `Input.text`); Backspace, Page Up and Page Down are new keys too. It opens only while a person plays,
  in the crypt or the merchant's room, and not over another menu.
- **The plate** is 72 pixels tall, black, with a stone line and two of iron below; it slides down in a
  quarter of a second. Lines wrap at 37 letters; it keeps the last 48. Page Up, Page Down and the wheel
  scroll back, with small arrows at the right when there is more above or below. A typed line left
  unfinished is still there when it opens again, as in Quake.
- **The pause.** The crypt stops (`con_update` returns before anything moves). The sound engine's hold
  (`snd_hold`) fades the game's effects and the torches' crackle out over a fiftieth of a second and
  keeps them where they were, starts no new ones, and lets the music play on with its drums, which
  share the effects' voices. In the harness, the sound with the console down kept the music's level and
  lost the effects; the first version held the drums as well and let 28 of them loose at once on
  closing, which the fade and the split put right.
- **The price** is `CON.used`, kept until the program closes: no toasts or achievements (`ach_earn`),
  the save is empty, so fleeing leaves the stored progress as it was (`crawl_save` returns 0, which the
  Windows shell and the harness both skip), and carving a grave says *NO GRAVES WHILE THE DEAD TAKE
  NOTE*. The Esc menu's notes for fleeing and carving say so too. The list of codes is free and costs
  nothing.
- **The codes** sit in `CON_CODES` as an FNV-1a hash of the word and its length, with what follows it
  (nothing, a number, an optional number or a creature's name); the list one of them prints is in
  `CON_HELP`, scrambled. The debug switches they mirror now share their work with them (`give_gear`,
  `give_bag`, `give_ability`, `tp_exit`, `tp_secret`, `tp_altar`, `meet_creature`, `pilgrim_step`), and
  the debug build prints every console line to the log (`CONSOLE ...`). A word that is known but given
  a bad number, or used where it can't work (in the merchant's room, on a level without what it
  wants), says why and costs nothing.
- **Found on the way:** the Wraith came only once a session, not once a level as meant (since 1.0.0b3:
  `G.wraith_done` was never cleared); it is cleared with each level now, and on a retry after a death,
  which will make the Deep harder for the autopilot. And wisps from a level could leave their motes
  drawn in the merchant's room; they are cleared there too.
- Tested in the harness only: every code, its refusals, the scrolling, the sound, the price on fleeing
  and on graves; the Windows build compiles without warnings and its binary holds none of the words.


### The L menu

- **A tap of L** opens the menu (the game pauses, as with Tab). It lists what can be summoned now, each
  with its cost: *Exit wisp, 1 soul*; with Keymaker, *a wisp to the gold / silver / crimson key, 1 soul*
  for each key still lying; with an altar awake and secrets left, *the altar's wisp, free*. W and S
  choose, Enter casts, Esc closes. Too few souls: the line shows in red and Enter says so.
- **The toggle** at the bottom, *L ALWAYS CASTS THIS* (Space): with it on, a tap of L
  casts the chosen wisp at once (if it can't be cast now, the menu opens instead), and **holding L**
  for about 0.6 s *(default)* opens the menu.
- The choice and the toggle are kept with the run, in saves and graves *(default)*: save version 4,
  which still loads versions 1 to 3.

As built in step 3: `UI_WISPS` (`wisp_choices`, `update_wispmenu`, `render_wispmenu`), a plate at the
top of the view over the dimmed crypt, each line with a small wisp of its colour and its price, gold if
you can pay it and red if not; the spirit you have at the upper right. It opens on the wisp last cast.
Enter, E or a click casts and closes it (a click that closes a menu no longer casts a spell as well,
`G.click_spent`, and that goes for every menu); L or Esc closes it. Space turns the toggle and takes
the line under the cursor as the one L casts; casting from the menu makes that line the one too.
Holding L is timed from the press (`G.l_t`, `G.l_down`) and the menu opens while it is still held.
The hint shown on taking control gains a fourth line, *L WISPS  H THE STATUS BAR*.
Save version 4 is 943 bytes: four bytes after the record (the kind, the key's colour, the toggle and a
spare). In the harness: taps, a hold, a click, too few souls, a Keymaker run with three keys lying, a
version 4 save fled and loaded, and a version 3 save loaded; smoke and an ASan round were clean.

### Black Altars

- **E at an altar** now opens a small menu, like the question at Esc: *Wake its wisp, 10 souls*
  (until woken), *Bless your exit wisps, 25 souls* (once a level), and *Anoint* (when it burns gold).
- **Waking** sends the violet wisp out from the altar to the nearest secret still shut, then on to the
  next as each is opened, free; the L menu can call it back for nothing if it is lost.
- **Blessing** lasts until you leave the level, for every exit wisp you cast on it. As a blessed wisp
  goes (and while it hovers), the undead within about four cells flee, as from Dread, and for three
  seconds after it passes *(default)*; the Wraith is driven back. The flame takes a white edge while the
  blessing lasts *(default)*.
- **Anointing** is as in 1.0.0b4.

As built in step 4: E at the altar opens *THE BLACK ALTAR* (`UI_ALTAR`), three lines, each with a small
flame of its colour and its price, and under them a note on the line chosen: why it can't be had yet
(*FIND EVERY SECRET: 1 OF 3 SO FAR*), or what it does. Lines that can't be had are grey and Enter on
them only knocks; Enter on the others takes the gift and closes the menu, and the caption says what
happened. The blessing can be bought in any state of the altar, woken or not, and ends when the level
does (a death rebuilds the level, so it goes then too). Turned undead flee from where the wisp was,
not from the caster, and only those in its sight are turned. The white edge is drawn about the
flame, outside it, so the violet stays violet. In the harness, a test player that only follows an
exit wisp through level 9 took 2 blows with the blessing against 11 without (four seeds, one level
each). The debug build has `CRAWL_BLESS=1` to bless every level. ASan was clean.

### The autopilot

- **Exit wisps:** when it sets off for the exit, it casts one if it has a soul to spare *(default: at
  least 2 spirit)*.
- **Altars:** when it passes an altar with 25 spirit or more it wakes it *(default: keeping 15 back for
  spells, as its spending already does)*, and while it has time on the level it goes to the secrets the
  violet wisp leads to. It anoints when the altar burns gold, and buys the blessing with 40 spirit or
  more if the level has undead *(default)*.

As built in step 5: the exit wisp goes out as it sets off for the exit, unless one is already lit. It
asks at the altar from the lane cell beside it, turning to it and laying a hand on it for a moment
(`PS_ALTAR`), when the altar is within 24 steps (30 for an anointing, which is worth the walk). It
wakes an altar only until 180 s on the level, so there is time to follow the wisp (in the first runs
it woke one at 234 s and left at 242), and blesses until 240 s, since the blessing serves the exit
wisp it casts on leaving. While the altar is awake and before 240 s, the secret the violet wisp seeks
is its goal. From a level 6 save with 60 spirit, on four seeds, it woke three altars, blessed with
each, found every secret on two of those levels, and anointed once. How this weighs on its deaths and
times is for the step 7 runs. ASan was clean.

## Building it

1. **The status bar**: the slide, the five seconds, what brings it back, the mini bar in player mode,
   a tap of H. Always up in the merchant's room.
2. **Wisps in the world**: the `Wisp` and its flight, light and tail; exit and key wisps; the hand's
   wisp and the ten-souls rule removed; the Wraith, shades and Keymaker's glint moved over.
3. **The L menu**: tap and hold, the toggle, costs, save version 4.
4. **Black Altars**: the menu, waking with free violet wisps, the blessing and turning the undead.
5. **The autopilot**: exit wisps, waking altars, the secrets, anointing, the blessing.
6. **The console**, once the owner has approved its codes: the key, the plate flying down, the greeting,
   typing and scrolling, the codes, the pause with the music playing on, and the session's price.
7. **Checks and the beta**: frames of each in the harness, eight-seed runs against 1.0.0b4 from levels
   1, 11 and 16, ASan, the README, the page, previews and release notes; 1.0.0b5 when the owner says.

As built in step 7: eight runs on each of the usual starts, on the same seeds as 1.0.0b4 (whose numbers
the released build reproduced line for line). From level 1, 4 deaths against 3, levels 7 to 9 (median 8.5
against 8), 77% of the treasure (75%), 43 secrets (34). From level 11 at tier 1, 11 deaths against 12. From
level 16 at tier 2, 30 minutes, 28 against 25, most of the difference late in a level, now that the Wraith
comes once a level (33 times in those runs against 8; 56 against 8 from level 11). Treasure taken fell in
the Hollows (67% from 71%) and the Deep (63% from 68%). The autopilot cast 146 exit wisps, woke 39 altars,
bought 54 blessings (72 undead turned) and was anointed once. Losing the hand wisp's light cost it nothing
measurable: no more of its deaths came early in a level. New previews: the wisps strip, and the crypt, play
and items strips re-shot without the hand at rest or the wisp on it. README and release notes rewritten;
the page and VERSION wait for the release.

Risks worth naming:

- The autopilot going to altars and secrets takes time from the rest of the level; the runs will show
  whether it costs treasure or lives.
- A wisp that flies ahead can leave a person behind in a fight; the tail is meant to cover that, and
  playing it by hand is the real test.
