# Touch, the mouse in menus, and asking how you play

**Status: a draft (10 November 2026), nothing built.** On `claude/funny-meitner-yn953c` from main at 1.0.0b21.2. The
owner asked for a plan to play the browser build on a phone or tablet, with a layout of their own, and with it formal mouse
support and a question on the browser's page of how a person is playing. They asked for a plan only, no code. The questions
(section 10) are for the owner. Where their words can be read two ways the plan says how it read them, and asks.

## 1. What was asked

- **Mobile support in the browser.** Today a phone or tablet can only watch (docs/PLAN-web.md, W6).
- **Two virtual sticks**: one walks, the other turns.
- **Taps at the screen's edges, each strip about a fifth of its width**: the left one interact and accept, the right one
  attack and back. (Read here as two strips, one at each side. T1 checks that reading and how the strips share their
  ground with the sticks.)
- **The upper left corner**: tapped, it fires the rune readied; held, it opens the pause menu.
- **The upper right corner**: tapped, it fires; held, it opens the spells' wheel and the runes' wheel, with something
  on screen to tap between the two. (What a tap fires is T3.)
- **A Map button** while the map is owned, and **an Inventory button** for the wardrobe.
- **Formal mouse support**: clicks register as interactions and the mouse can move freely over the screen.
- **The web app asks how a person is playing**: Keyboard, Controller or Mobile, which switches the controls. The
  screensaver doesn't ask. With Mobile, a finger held on the screen while the dream plays takes over, as H held does.

## 2. What there is today

- **The page** (`web/haunted.js`, `web/index.html`). Keys by `KeyboardEvent.code`, the mouse under Pointer Lock, the
  Gamepad API, the slots in `localStorage`. `#stage` already has `touch-action: none`, but nothing listens for touches.
  A device with only a coarse pointer gets *On this device you can watch* and *Tap to boot*. A finger held on the glass
  is nothing to the game, so a phone can't take over.
- **What reaches the game** (`Input` in `src/core.h`): keys held and hit, the mouse's movement in counts (relative only),
  the wheel, typed letters (for the console), the pad. There is no pointer position anywhere, and the game draws no
  cursor.
- **The takeover** (`app_update` in `src/app.c`): H, or any pad button, held for `HOLD_SECONDS`. `A.hold_pad` picks the
  plate's words.
- **The pad as keys.** `pad_keys` turns the pad into the keys each place reads, by context. In a menu the D-pad and stick
  are W/S/A/D with repeat, A is Enter, B is Esc, and so on; in play, each button's state goes to its job (`PL`). Walking
  reads `IN.lx`/`IN.ly`, turning `IN.mdx` and `pad_turn` (`IN.rx`), the sprint `PADX.run`, Swift Dodge `PADX.dodge`.
  Touch can follow the same pattern.
- **Menus.** Twenty-four screens (`UI_*`), the title's five states, the two wheels, the map, the cards (a level's, a
  death's, her claim's, a boss's) and the reckoning. All are worked with keys: W/S, A/D, Enter, Esc, Space, Backspace,
  PgUp/PgDn, Tab. Most keep their place in `G.ui_sel` (182 uses); a few keep their own (`G.pt_sel`, `G.vow_sel`,
  `G.stone_sel`, `G.cs_sel`, `G.rst_sel`, `TI.sel`/`TI.gsel`, `PS`). They are drawn with the kit (`ui_row` 20 times,
  `ui_tree`, `ui_socket`, `ui_niche`, `ui_pane`, `card_box`, `hall_frame`, `tab_frame`), and their hints by `ui_keys`
  and `hint8`.
- **The wheels** already take a point from the mouse's movement (`WH.mx`, `WH.my`) as well as from a stick.
- **The hints** speak the keys, or the pad's buttons when it was touched last (`pk`, `PK_KEY`/`PK_PAD`, `PADX.used`).
  Touch would be a third language.
- **The picture** is scaled by whole device pixels and letterboxed. On a phone held sideways that leaves black either
  side:

  | Device, held sideways | Its pixels | Picture | Black at each side | The stage in game pixels |
  |---|---|---|---|---|
  | iPhone 14 (844 x 390 pt, three to one) | 2532 x 1170 | 533 x 333 pt (five to one) | 155 pt, 18% | 506 x 234 |
  | Pixel 7 (915 x 412, 2.625 to one) | 2402 x 1081 | 610 x 381 | 153, 17% | 480 x 216 |
  | iPad Air (1180 x 820, two to one) | 2360 x 1640 | 1120 x 700 (seven to one) | 30, 3% | 337 x 234 |
  | iPhone 14 upright | 1170 x 2532 | 320 x 200 (three to one) | 322 above and below | 390 x 844 |

  So on a phone the owner's fifth at each side is almost exactly the black either side of the picture, where the thumbs
  rest anyway. On a tablet there is no margin, and the controls would have to lie over the picture.
- **Speed.** Under Node on a desktop the crypt took 4.5 ms a frame as wasm (PLAN-web's step 0); the title is 8-14 ms a
  frame natively. A phone has never been measured.

## 3. The shape of it

Three pieces, each built on the one before:

- **A. The mouse in menus.** A pointer in the game: every row, socket, node and hint can be pointed at and clicked. It is
  in the game's code, so Windows gets it too (T10). Touch needs it, because a tap on a menu is a click.
- **B. Asking how you play**, on the browser's page: Keyboard and mouse, Controller, or Touch.
- **C. Touch**: the owner's layout, the takeover by a finger, the controls drawn in the margins, and the hints in their
  words.

Nothing in the simulation, the saves or the dream's numbers moves. The prefs record (`SLOT_PREFS`) grows, for the touch
controls' settings. Unlike the browser's first build, this changes the game's code, not only its shell: the pointer and
the touch controls belong to the game, so the harness can see and test them.

## 4. Part A: the mouse in menus

- **The pointer in `Input`**: a place in the picture's pixels (`px`, `py`), and how it came: the mouse's counts gathered
  into a place, or a place given outright (the browser's cursor with the mouse free, or a finger). The buttons and the
  wheel are the ones there are now.
- **A cursor of the game's own** while a menu, card, wheel or the title is up and the mouse has moved: a small bony hand,
  drawn in the UI's stone, with a sheet first for the owner. It moves by the mouse's counts, so it works under Pointer
  Lock in the browser and with Windows' raw mouse alike. Nothing is let go and taken back. In play there is no cursor:
  the mouse turns, as now. It hides after three seconds still, or when a key or the pad is used.
- **In the browser with the mouse free** (Esc freed it, or it was never taken), the page gives the game the system
  cursor's place over the canvas, and the game draws no cursor of its own. A click in a menu then counts as a click, not
  as taking the mouse. Today the next click after an Esc only takes the mouse back. *CLICK TO TAKE THE MOUSE* comes only
  once play resumes.
- **What can be pointed at: a hot list.** As a screen is drawn, each thing that can be chosen puts its rectangle on a
  list (`ui_hot`): a row (which selection it sets and to what), a socket, a node, a grave, a slab, a wheel's slot, and each
  cap of a hint line (the key it presses). Before a menu's own update, `ptr_keys` (beside `pad_keys`) reads the pointer
  against the last frame's list. Pointing at a row chooses it, as W/S would, with the tick. A click on the row already
  chosen presses Enter; a click on a cap presses its key. Each menu's own update stays as it is: it sees its selection
  move and its keys pressed, as it does with the pad.
- **Most of it comes from the kit.** `ui_row`, `ui_socket`, `ui_tree`'s nodes, `ui_niche`, `card_box` and `ui_keys`
  register themselves. A hint's caps (*A/D PAGE*, *SPACE UNBIND*, *BACK*) become buttons where they are drawn, so every
  key a screen names can be clicked without work for each screen. The rest is by hand: the wardrobe's figure, the
  Lampstand's tree, the map (dragged to pan), the title's slabs and graves (`title_draw` hands back their rectangles in
  `TitleView`), and lists that scroll (the wheel moves through them as W/S do, and a drag moves them).
- **Small rules, taken by default:** the right button backs out of a menu, as Esc does; the wheel moves the choice in
  any list. In play nothing changes.
- **Checks:** `tools/uishots.sh` with `MOUSE=1`, as `PAD=1` plays every recipe on the pad: each screen opened, a row
  pointed at and clicked, a cap clicked. ASan as ever. By hand on Windows, the owner's.

## 5. Part B: asking how you play

- **On the page's plate**, before it boots (*C:\>HAUNTED.EXE*):

  ```
  C:\>HAUNTED.EXE_
  HOW WILL YOU PLAY?
    1  KEYBOARD AND MOUSE
    2  CONTROLLER
    3  TOUCH
  ```

  The likely one is marked: Touch where the only pointer is coarse, Controller where a pad has been seen, else the
  keyboard. The choice is the press that boots it, so sound starts with it. It is kept in the page's settings, the plate
  says *PLAYING BY TOUCH: CHANGE* afterwards, and the Settings panel has it (T8).
- **What each one changes.**
  - *Keyboard and mouse*: today's, with Part A. The mouse is taken at the takeover.
  - *Controller*: the hints speak the pad's language from the start; the page never asks for the mouse and never shows
    *CLICK TO TAKE THE MOUSE*. Sound still waits for a key or a tap (a pad's press doesn't count to the browser).
  - *Touch*: Part C. No mouse is taken. The canvas becomes the stage (section 6.5), and the hold to take over is a finger.
- **Whether it shuts out the others** is T9. The recommendation is that it leads and doesn't shut out: a pad picked up or
  a key pressed still works, as keys and the pad do between them today, but the touch controls show only in Touch.
- **The game is told** (`AppConfig.input`: keys, pad, touch; Windows always the keys, with the pad as it is now), since the
  game draws the touch controls, words the plate and speaks the hints.

## 6. Part C: touch

### 6.1 The layout, as read from the owner's words

On a phone held sideways (the stage of an iPhone 14 in game pixels, the picture in the middle):

```
 0          93                                             413        506
 +-----------+---------------------------------------------+-----------+  0
 | (1) RUNE  |                                             | (2) ?     |
 | hold:     |                                             | hold:     |
 | PAUSE     |                                             | WHEELS    |
 +-----------+                                             +-----------+  58
 | WISPS MAP |                                             | WARDROBE  |
 |           |                                             |           |
 |  left     |                the picture                  |  right    |
 |  strip:   |                 320 x 200                   |  strip:   |
 |  USE /    |                                             |  CAST /   |
 |  ACCEPT   |                                             |  BACK     |
 |   (o)     |                                             |   (o)     |
 |  walking  |                                             |  turning  |
 |  stick    |                                             |  stick    |
 +-----------+---------------------------------------------+-----------+  234
```

The WISPS button isn't in the owner's list; it is proposed in T5, with homes for the other keys touch would lack.

### 6.2 What each control does, by where you are

| Control | The dream | The title | In play | A wheel up | A menu or card |
|---|---|---|---|---|---|
| A finger held anywhere, three seconds | takes over (H) | | | | |
| Left strip, a tap | | the choice (Enter) | E: open, take, use, speak | | Enter: take what is chosen |
| Right strip | | back | cast; held, again as it comes round, or charged | puts it away | Esc: back |
| Walking stick | | | walk and strafe by the lean; pushed forward to its rim, the sprint | | |
| Turning stick | | | turn (T2) | | |
| Upper left, a tap | | | the rune readied (the pad's RB; the Mirror is raised this way) | | |
| Upper left, held | | | Esc's question | | |
| Upper right, a tap | | | T3 | | |
| Upper right, held | | | the spells' wheel, a tab at its head for the runes' and gifts' | | |
| MAP (once bought) | | | the map, and again to close it | | |
| WARDROBE | | | Tab | | Tab |
| A tap on the picture | | a slab, a grave | a hint's cap (a prompt's *Q HOLD IT*) | a slot, the tab | a row, a cap, a socket |
| A drag on the picture | | | | | a list scrolled; the map panned |

- **Taps, holds and drags** (first numbers, to be tuned by hand): a tap is down and up within a quarter second, moved
  less than 6 game pixels (about 2 mm on a phone); a hold is 0.3 s still (`WHEEL_HOLD`, as the pad's). A stick floats:
  its base is where the thumb comes down in its half, its ring is 24 game pixels across (about 7 mm), its knob follows
  the thumb, and the lean is the distance over the ring past a dead sixth.
- **Who gets a finger**: the buttons first (the corners, MAP, WARDROBE, WISPS), then a cap or a row under it, then the
  strip's tap, then the stick.
- **The finger that took over** counts only once it is lifted, as a pad's button does (`PADX.fresh`).
- **Mostly the pad's paths.** `touch_keys` runs after `pad_keys` and writes what the pad writes: the sticks into
  `IN.lx`/`IN.ly` and the turning, the sprint into `PADX.run`, a dodge into `PADX.dodge`/`dodge_a`, and keys pressed by
  the buttons. Walking, turning, the sprint and the dodge need nothing new, and the pad's hints stay off.
- **The wheels stay up on touch.** On the pad a wheel is held open and let go to take. On glass, the hold opens it, the
  finger lifts, and it stays (the world grey and slow, as now) until a slot is tapped, the tab swaps wheels, or the right
  strip or a tap outside puts it away (`WH.sticky`).

### 6.3 The takeover and the title

- **A finger held anywhere** on the stage for three seconds is H held: `app.c` counts it with H and the pad's buttons, in
  the crypt and in HAUNT.EXE (cutting to the crypt, as H does). A quick tap does nothing, as a pad bumped on a desk does
  nothing. The plate reads *HOLD A FINGER DOWN TO TAKE CONTROL*, and in HAUNT.EXE *HOLD A FINGER DOWN TO ENTER THE CRYPT*
  (37 letters: it fits HAUNT.EXE's 720 pixels at its doubled size, where *ON THE SCREEN* would run off).
- **The title** says *TOUCH THE SCREEN* where it says *PRESS ANY KEY*; a tap on a slab or a grave chooses it, a second
  takes it (T11), and the right strip goes back. THE CONTROLS shows the touch layout as the pad's screen shows the pad: a
  phone's outline, each control on a leader (`ui_touch`, beside `ui_pad`).

### 6.4 Where the layout needs deciding

The owner's layout raises a few questions of its own. Each is in section 10:

- **The left strip and the walking stick share the left thumb.** A tap there means stop and press, which suits E (open,
  take): nothing it does is timed to the moment. So a tap is decided on the lift. A drag is the stick.
- **The right strip casts, and the right thumb also turns.** Casting is the most frequent thing and wants to answer at
  once, and to be held (again as it comes round, or charging an ultimate). So the recommendation (T1 a) makes the right
  strip a button from the moment it is touched, turning as it is dragged so a person can aim while casting, with the
  turning stick inward of it.
- **A tap or a hold on one corner.** If a tap fires on the touch, every hold for the pause fires the rune first. If it
  fires on the lift, a parry with the Mirror comes a tap's length late (0.1-0.2 s of its 0.6 s). T4.
- **What the upper right's tap fires**, since the right strip already casts. T3.
- **Keys touch would lack**: B (the bestiary), C (the Codex), L (the wisps: the exit wisp, every level), H (the status
  bar), Swift Dodge (a direction tapped twice), the sprint, 1-9 (the wheel covers those), the console (no keyboard: it
  isn't on touch). T5 proposes homes.
- **Upright.** The layout is for a phone held sideways. T7.

### 6.5 Drawing the controls: the stage

The recommendation (T6 a) is that **the game draws the controls in its own pixels**. Phones' margins are outside the
picture, so the page tells the game the stage: its size in game pixels (the window's device pixels over the scale), where
the picture lies in it, and the safe area (the notch, the home bar). In Touch the canvas is the stage, shown at the
picture's whole-pixel scale: 506 x 234 on an iPhone held sideways, the picture at (93, 17). The game draws the picture there
(the slow drift moves it within the stage), the controls in the margins in the run's carved stone with a mark each, and on
a tablet, where there are no margins, over the picture's edges and fainter.

So the controls are VGA pixels like everything else, and the harness can draw a phone's stage (`HOST_STAGE=506x234`)
and play it with scripted fingers. The other ways (the page drawing them in HTML, or the game drawing them over the
picture only) are in T6.

Two things to know for the build. A stage of a phone held sideways fits inside a `Frame` (720 x 400 pixels) at the scales
phones use; upright it doesn't, which is one reason for T7 (a). And the kit's marble and its fit checks are
320 x 200 (`MW`, `MH` in `crawl_ui.c`), so the margin's pieces are drawn small and placed, not laid on the kit's
whole-screen marble.

### 6.6 The hints in touch's words

`pk` gets a third language. Each control has a mark (a hand for the left strip, a flame for the right, the rune, the
scroll, the satchel, the wisp), drawn in a hint where its key would be, as the pad's faces are drawn in their colours.
*E TO OPEN* reads with the hand's mark, *ESC BACK* with the right strip's, *A/D PAGE* with two arrows. Every cap can be
tapped (Part A). W/S drops out of the hints, since a row is tapped.

### 6.7 The touch controls' own settings

THE TOUCH CONTROLS, from Esc's question while playing by touch (as THE CONTROLLER is while a pad is in): the turning's
pace, left-handed (the layout mirrored), the controls' strength (faint to solid), sticks floating or fixed, buzzing on or
off. Kept in the prefs record's next version, beside the pad's layout.

### 6.8 A phone's own trouble

These are the browser's rules and the phones' habits. Each has a remedy or a warning, and none can be tried in this
container:

- **No full screen for a page on an iPhone.** Safari lets a page go full screen on an iPad but not on an iPhone, so its
  bars stay unless the page is added to the Home Screen. A web app manifest (`display: fullscreen`, held sideways) makes
  that work. A Home Screen app keeps its storage apart from Safari's (as far as is known; to be checked), so its ledger
  starts fresh. The page would say so, and the ledger's file carries it across.
- **Turning the phone.** Android's Chrome can hold the screen sideways once full screen (`screen.orientation.lock`);
  Safari can't.
- **Edge gestures.** Safari's swipe from the left edge goes back a page, and Android's gesture navigation takes swipes
  from either side. Taps are safe; a stick begun at the very edge may be taken by the system. The live ground keeps a
  little in from the edges, and a finger taken by the system (`pointercancel`) is let go.
- **Long presses.** The callout, the magnifier and text selection are turned off over the stage
  (`-webkit-touch-callout`, `user-select`), the context menu stopped.
- **The iPhone's silent switch** mutes the Web Audio that the game's sound uses, unless the page asks for playback
  (Safari's audio session, where it has it).
- **The wake lock** works in Safari since 16.4 and in Chrome; the page already asks for it.
- **Buzzing.** Android's `navigator.vibrate` can stand in for the rumble; iPhones have nothing a page can reach.
- **Speed and heat.** Unknown until measured: step 0.

## 7. What doesn't move

The simulation, the saves (but for the prefs record), the dream and its numbers, the harness's balance runs, HAUNT.EXE's
art. In play with the keyboard and mouse nothing changes but a cursor in menus. The pad's layout and screen are as they are.

## 8. Steps

0. **The probe on a phone.** Today's `/play/` with `?perf` (a corner readout of ms a frame from `window.HAUNTED.perf`)
   and `?take` (H held for three seconds as it boots, since a phone can't take over yet). The owner opens it on their
   phone or phones: the dream, the crypt in play, the title. If the title is too slow on a phone (8-14 ms natively might
   be 30-40 ms there), it gets a cheaper way to draw before anything else.
1. **The input's shape.** `Input` gains the pointer, up to five fingers (id, place in stage pixels, down, moved, up) and
   the stage (size, the picture's place, the safe area); `AppConfig.input`. `web_main.c` exports them; `tools/host.c`
   reads `p` (a pointer's place), `T` (a finger: `T0d40,150`, `T0m60,150`, `T0u`), `HOST_STAGE` and `HOST_MODE`. Nothing
   behaves differently yet.
2. **The mouse in menus** (Part A): the hot list in the kit, `ptr_keys`, the cursor (its sheet first), the title's
   rectangles, the wheel by pointing, lists by the wheel and by dragging, the right button as back. Every screen walked
   with `MOUSE=1`. The Windows build is CI's, and the owner's hand.
3. **The plate's question** (Part B): the three choices, kept, changed in Settings; what each changes in the page; the
   game told.
4. **Touch, the fingers' rules** (6.2, 6.3): `touch_keys`, the strips, the sticks, the corners and buttons, the takeover by
   a finger in `app.c`, the title's words and taps, the sticky wheel. In the harness with scripted fingers, nothing drawn
   yet but a debug outline.
5. **Touch, drawn** (6.5): a sheet first for the owner (each control idle, pressed and in a menu, the marks, a phone's
   stage, a tablet's), then the stage in the game and the page, the safe area, the strength setting.
6. **Touch's words and the gaps** (6.4, 6.6, 6.7): the hints, the homes T5 settles, THE TOUCH CONTROLS and its prefs, the
   title's touch layout.
7. **The browser's side for phones** (6.8): Pointer Events with capture, the manifest, the orientation, the callouts, the
   silent switch, buzzing on Android, the edges, the plate's words for a phone.
8. **The checks.** The harness: every screen by scripted fingers on a phone's stage and on a tablet's, and with the
   mouse; ASan. Playwright in CI: a phone's shape with touch (a held finger taking over, a stick walked, a menu tapped) and
   a mouse through the menus. By hand: the owner, on their phone and on Windows. What wasn't tried is said in the README
   and the page's *Beta status*, as ever.

## 9. Size

As a guess: in the game, the pointer and its hot list some 500 lines, touch's rules 600-900, the controls' drawing and
marks 500-800, the hints and the gaps 300; the page some 300 lines of script; the harness and the smoke test 300. Some
3,000 lines in all, about half of it touch's.

## 10. Questions

**T1. The edge strips and the sticks.** How do the strips share the ground with the sticks?
(a) The walking stick floats wherever the left thumb lands in the left half; a quick, still tap in the left strip is
interact or accept. The right strip is the cast from the moment it is touched, held to cast again or charge, and dragged
it turns too, so a person aims as they cast; inward of it is the turning stick alone *(recommended: casting answers at
once and can be held, and turning never casts by accident)*; (b) the strips are buttons only, and the sticks float in the
ground between them and the middle, over the picture's lower corners; (c) both strips shared, decided on the lift: a tap
is the button, a drag the stick, so a cast comes a tap's length late and a still hold holds it.

**T2. The turning stick.** (a) A stick, as asked: the further it leans, the quicker it turns, as the pad's right stick
does (`PL.turn`); (b) a swipe: the finger's travel turns the view, as the mouse does, which most phone shooters use for
precision; (c) both: travel turns the view near its middle, and past its ring it keeps turning *(recommended: a rate stick
alone is hard to aim on glass, which has no spring to find the middle by, and a swipe alone runs out of glass on a long
turn)*. Which is the default, and the other a setting?

**T3. The upper right corner's tap.** (a) What Q readied last, the right mouse's job (the pad's LT): a boss's gift, or a
rune raised by hand *(recommended: the right strip already casts, and this gives the gifts a home)*; (b) a cast, beside
the right strip's.

**T4. A corner tapped or held.** (a) The tap fires as the finger lifts, and the hold opens after 0.3 s, firing nothing
*(recommended: pausing never spends the rune; a parry rises 0.1-0.2 s later, against its 0.6 s of true parry)*; (b) the tap fires
as it is touched, and the hold opens after.

**T5. Homes for what touch lacks.** (a) WISPS a small button beside MAP (L's tap, and held, the list), as the exit wisp is
cast every level; THE BESTIARY and THE CODEX as rows of Esc's question; the status bar a tap on the picture's lower edge;
the sprint the walking stick pushed forward past its ring; Swift Dodge the walking stick touched twice quickly and pushed
the way to go, as the keys' double tap *(recommended)*; (b) the bestiary, the Codex and the wisps as further buttons in the
margins; (c) other homes the owner names.

**T6. Who draws the controls.** (a) The game, in its own pixels, in the margins around the picture, and over its edges on
a tablet (section 6.5) *(recommended: they look like the rest, and the harness can draw and test them)*; (b) the page, in
HTML and CSS, crisp at the phone's own resolution, with the game told only which buttons to show; (c) the game, over the
picture only, leaving the margins black.

**T7. A phone upright.** (a) To play, sideways; upright, the dream can be watched, and taking over shows *TURN IT ON ITS
SIDE* *(recommended)*; (b) an upright layout as well, the controls under the picture like a handheld's.

**T8. When the question is asked.** (a) On the plate the first time, then remembered, with *CHANGE* on the plate and the
choice in Settings *(recommended)*; (b) every visit; (c) never asked: guessed from the device, changed in Settings.

**T9. Does the choice shut out the others?** (a) It leads: the hints, the page's ways and the touch controls follow it,
but a pad or a key used still works, as today *(recommended: a phone with a pad, or a tablet with a keyboard, does what
the person does)*; (b) it is strict: only the chosen way is heard.

**T10. The mouse in menus on Windows too.** (a) Yes: it is the game's code, a cursor of the game's own moved by the raw
mouse *(recommended: one way for both, and the owner can try it by hand where they play)*; (b) the browser only.

**T11. A tap on a row.** (a) The first tap chooses it and shows what it is; a tap on the row already chosen takes it. The
mouse's pointing chooses, so for the mouse a click takes at once *(recommended: the panes beside the lists are read before
anything is taken, and the screens that ask twice still ask)*; (b) a tap takes at once.

**T12. The mouse free in the browser's menus.** (a) The game's own cursor under the lock, the lock kept through menus,
and the system's cursor only once the mouse has been freed *(recommended: nothing is let go and taken back, and Chrome's
second of refusal after an Esc never bites)*; (b) the lock let go whenever a menu opens and the system's cursor used.

**T13. The release.** (a) Its own beta, next: 1.0.0b22, with the art pass's rest moving to 1.0.0b23 and the lower storey to
1.0.0b24; (b) after those two, as 1.0.0b24; (c) the mouse in menus first as a point release, 1.0.0b21.3, and touch as
1.0.0b22.

**T14. Phones to test on.** Which phones and tablets does the owner have (an iPhone, an Android, an iPad)? Step 0 and the
checks by hand are theirs, and Safari and Chrome differ the most here.

## 11. Hand-off log

- **10 November.** The draft: what the browser, the input, the takeover, the title, the menus and the wheels do today, and
  the plan above. Nothing built. Waiting on the owner's answers to T1-T14.
