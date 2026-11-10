# 1.0.0b21.4: touch, the mouse in menus, and asking how you play

**Status: settled (10 November 2026), nothing built.** On `claude/funny-meitner-yn953c`, with main at 1.0.0b21.3 merged in. The
owner asked for a plan to play the browser build on a phone or tablet, with a layout of their own, formal mouse support, and a
question on the browser's page of how a person is playing. They answered T1-T14 the same day (section 10, kept for the record):
the recommended answer to each but T10 (the mouse in menus in the browser only, not on Windows) and T13 (it goes out as **1.0.0b21.4**,
after their 1.0.0b21.3). It will be tested on **Android**. They added one thing: the new controls at the edges keep the game's
gravestone look, with enough contrast that their words and marks are easy to read (section 6.5). The art pass's rest stays 1.0.0b22 and
the lower storey 1.0.0b23.

## Decided

1. **The strips and the sticks** (T1 a). The walking stick floats wherever the left thumb lands in the left half. A quick, still tap
   in the left strip is interact (E) or accept (Enter). The right strip casts from the moment it is touched; held, it casts again as
   the spell comes round, or charges; dragged, it turns as well, so a person aims as they cast. Inward of it is the turning stick.
2. **Turning** (T2 c). The finger's travel turns the view near the stick's middle; past its ring it keeps turning. The pace is a
   setting.
3. **The upper right's tap** (T3 a) is the right mouse's job (the pad's LT): what Q readied last, a boss's gift or a rune raised by
   hand. Held, it opens the wheels.
4. **A corner's tap fires as the finger lifts** (T4 a); the hold opens after 0.3 s and fires nothing.
5. **Homes for what touch lacks** (T5 a). WISPS is a small button beside MAP: a tap is L's tap, and held it opens the list. THE
   BESTIARY and THE CODEX are rows of Esc's question. The status bar comes up on a tap on the picture's lower edge. The sprint is the
   walking stick pushed forward past its ring. Swift Dodge is the walking stick touched twice quickly and pushed the way to go.
6. **The game draws the controls** (T6 a), in its own pixels, in the margins around the picture, and over its edges on a tablet.
7. **Sideways to play** (T7 a). Held upright, the dream can be watched, and taking over shows *TURN IT ON ITS SIDE*.
8. **The question on the plate the first time** (T8 a), then remembered, with *CHANGE* on the plate and the choice in Settings.
9. **The choice leads, it doesn't shut out** (T9 a). The hints, the page's ways and the touch controls follow it, but a pad or a key
   used still works.
10. **The mouse in menus in the browser only** (T10 b, the owner's answer). Windows keeps its menus, keys and pad as they are.
11. **A tap on a row** (T11 a): the first chooses it and shows what it is; a tap on the row already chosen takes it. The mouse's
    pointing chooses, so for the mouse a click takes at once. Screens that ask twice still ask.
12. **The game's own cursor under the lock** (T12 a), the lock kept through menus; the system's cursor only once the mouse has been
    freed.
13. **1.0.0b21.4** (T13, the owner's answer), a point release after 1.0.0b21.3.
14. **Android** (T14): the owner tests it on an Android phone. Step 0 and the checks by hand are theirs. iPhones and iPads get what is
    cheap to give them (section 6.8), untried, and the notes say so.

## 1. What was asked

- **Mobile support in the browser.** Today a phone or tablet can only watch (docs/PLAN-web.md, W6).
- **Two virtual sticks**: one walks, the other turns.
- **Taps at the screen's edges, each strip about a fifth of its width**: the left one interact and accept, the right one attack and
  back.
- **The upper left corner**: tapped, it fires the rune readied; held, it opens the pause menu.
- **The upper right corner**: tapped, it fires; held, it opens the spells' wheel and the runes' wheel, with something on screen to
  tap between the two.
- **A Map button** while the map is owned, and **an Inventory button** for the wardrobe.
- **Formal mouse support**: clicks register as interactions and the mouse can move freely over the screen.
- **The web app asks how a person is playing**: Keyboard, Controller or Mobile, which switches the controls. The screensaver doesn't
  ask. With Mobile, a finger held on the screen while the dream plays takes over, as H held does.
- **With the answers**: the edge controls in the gravestone theme, with decent contrast so their words and marks are easy to tell.

## 2. What there is today

- **The page** (`web/haunted.js`, `web/index.html`). Keys by `KeyboardEvent.code`, the mouse under Pointer Lock, the Gamepad API,
  the slots in `localStorage`. `#stage` already has `touch-action: none`, but nothing listens for touches. A device with only a
  coarse pointer gets *On this device you can watch* and *Tap to boot*. A finger held on the glass is nothing to the game, so a phone
  can't take over.
- **What reaches the game** (`Input` in `src/core.h`): keys held and hit, the mouse's movement in counts (relative only), the wheel,
  typed letters (for the console), the pad. There is no pointer position anywhere, and the game draws no cursor.
- **The takeover** (`app_update` in `src/app.c`): H, or any pad button, held for `HOLD_SECONDS`. `A.hold_pad` picks the plate's
  words.
- **The pad as keys.** `pad_keys` turns the pad into the keys each place reads, by context. In a menu the D-pad and stick are
  W/S/A/D with repeat, A is Enter, B is Esc, and so on; in play, each button's state goes to its job (`PL`). Walking reads
  `IN.lx`/`IN.ly`, turning `IN.mdx` and `pad_turn` (`IN.rx`), the sprint `PADX.run`, Swift Dodge `PADX.dodge`. Touch follows the same
  pattern.
- **Menus.** Twenty-five screens (`UI_*`, with 1.0.0b21.3's `UI_BUY`), the title's five states, the two wheels, the map, the cards (a
  level's, a death's, her claim's, a boss's) and the reckoning. All are worked with keys: W/S, A/D, Enter, Esc, Space, Backspace,
  PgUp/PgDn, Tab. Most keep their place in `G.ui_sel`; a few keep their own (`G.pt_sel`, `G.vow_sel`, `G.stone_sel`, `G.cs_sel`,
  `G.rst_sel`, `G.shop_buy`, `TI.sel`/`TI.gsel`, `PS`). They are drawn with the kit (`ui_row`, `ui_tree`, `ui_socket`, `ui_niche`,
  `ui_pane`, `card_box`, `hall_frame`, `tab_frame`), and their hints by `ui_keys` and `hint8`.
- **What 1.0.0b21.3 changed here.**
  - Facing anything in her rooms gives a person a slim prompt at the foot of the view (`render_shop_prompt`, `shop_prompt`): the
    key on a cap, what it is, a note. A ware's card is gone for a person.
  - E at a ware, or at the map, opens the ware's buying (`UI_BUY`, `update_buy`): E or Enter buys (twice where the old would be
    lost), Q, Space or Back holds, A/D go round her other wares and the map, Esc, Tab or the right mouse closes. On the pad: A buys,
    Y holds, the bumpers go round, B closes.
  - `pk` reads the word BACK as a key only where a group begins (*BACK UNBINDS*), so touch's words must follow the same rule.
  - Q tapped at a pedestal still holds the ware (`wheel_input`). On touch the hold is the buying screen's HOLD.
- **The wheels** already take a point from the mouse's movement (`WH.mx`, `WH.my`) as well as from a stick.
- **The hints** speak the keys, or the pad's buttons when it was touched last (`pk`, `PK_KEY`/`PK_PAD`, `PADX.used`). Touch is a third
  language.
- **The picture** is scaled by whole device pixels and letterboxed. On a phone held sideways that leaves black either side:

  | Device, held sideways | Its pixels | Picture | Black at each side | The stage in game pixels |
  |---|---|---|---|---|
  | Pixel 7, a typical Android (915 x 412, 2.625 to one) | 2402 x 1081 | 610 x 381 (five to one) | 153, 17% | 480 x 216 |
  | iPhone 14 (844 x 390 pt, three to one) | 2532 x 1170 | 533 x 333 pt | 155 pt, 18% | 506 x 234 |
  | iPad Air (1180 x 820, two to one) | 2360 x 1640 | 1120 x 700 (seven to one) | 30, 3% | 337 x 234 |
  | iPhone 14 upright | 1170 x 2532 | 320 x 200 (three to one) | 322 above and below | 390 x 844 |

  So on a phone the owner's fifth at each side is almost exactly the black either side of the picture, where the thumbs rest anyway.
  On a tablet there is no margin, and the controls lie over the picture.
- **Speed.** Under Node on a desktop the crypt took 4.5 ms a frame as wasm (PLAN-web's step 0); the title is 8-14 ms a frame natively.
  A phone has never been measured.

## 3. The shape of it

Three pieces, each built on the one before, all in the browser:

- **A. The mouse in menus.** A pointer in the game: every row, socket, node and hint can be pointed at and clicked. Touch needs it, as
  a tap on a menu is a click.
- **B. Asking how you play**, on the page's plate: Keyboard and mouse, Controller, or Touch.
- **C. Touch**: the owner's layout, the takeover by a finger, the controls drawn in the margins as headstones, and the hints in
  their words.

All three are compiled into every build and turned on by the shell (`AppConfig.input`, below): the browser's page turns them on,
the harness can for its checks (`HOST_MODE`), and Windows never does, so its menus, keys and pad are as they are (Decided 10).

Nothing in the simulation, the saves or the dream's numbers moves. The prefs record (`SLOT_PREFS`) grows, for the touch controls'
settings. This changes the game's code, not only its shell: the pointer and the touch controls belong to the game, so the harness
can see and test them.

## 4. Part A: the mouse in menus

- **The pointer in `Input`**: a place in the picture's pixels (`px`, `py`), and how it came: the mouse's counts gathered into a place,
  or a place given outright (the browser's cursor with the mouse free, or a finger). The buttons and the wheel are the ones there are
  now.
- **A cursor of the game's own** while a menu, card, wheel or the title is up and the mouse has moved: a small bony hand in the UI's
  stone, a sheet first for the owner. It moves by the mouse's counts, so it works under Pointer Lock and nothing is let go and taken
  back. In play there is no cursor: the mouse turns, as now. It hides after three seconds still, or when a key or the pad is used.
- **With the mouse free** (Esc freed it, or it was never taken), the page gives the game the system cursor's place over the canvas,
  and the game draws no cursor of its own. A click in a menu then counts as a click, not as taking the mouse back, as the next click
  after an Esc does today. *CLICK TO TAKE THE MOUSE* comes only once play resumes.
- **What can be pointed at: a hot list.** As a screen is drawn, each thing that can be chosen puts its rectangle on a list (`ui_hot`):
  a row (which selection it sets and to what), a socket, a node, a grave, a slab, a wheel's slot, her rooms' prompt, and each cap of
  a hint line (the key it presses). Before a menu's own update, `ptr_keys` (beside `pad_keys`) reads the pointer against the last
  frame's list. Pointing at a row chooses it, as W/S would, with the tick. A click on the row already chosen presses Enter; a click on
  a cap presses its key. Each menu's own update stays as it is: it sees its selection move and its keys pressed, as it does with the
  pad.
- **Most of it comes from the kit.** `ui_row`, `ui_socket`, `ui_tree`'s nodes, `ui_niche`, `card_box`, `ui_keys` and `shop_prompt`
  register themselves. A hint's caps (*A/D PAGE*, *SPACE UNBIND*, *BACK UNBINDS*) become buttons where they are drawn, so every key
  a screen names can be clicked without work for each screen. That takes in 1.0.0b21.3's buying screen whole: its BUY, HOLD, the
  arrows round the wares and CLOSE are its hint's caps. The rest is by hand: the wardrobe's figure, the Lampstand's tree, the map
  (dragged to pan), the title's slabs and graves (`title_draw` hands back their rectangles in `TitleView`), and lists that scroll (the
  wheel moves through them as W/S do, and a drag moves them).
- **Small rules, taken by default:** the right button backs out of a menu, as Esc does (the buying screen already reads it); the
  wheel moves the choice in any list. In play nothing changes.
- **Checks:** `tools/uishots.sh` with `MOUSE=1`, as `PAD=1` plays every recipe on the pad: each screen opened, a row pointed at and
  clicked, a cap clicked. ASan as ever.

## 5. Part B: asking how you play

- **On the page's plate**, before it boots (*C:\>HAUNTED.EXE*):

  ```
  C:\>HAUNTED.EXE_
  HOW WILL YOU PLAY?
    1  KEYBOARD AND MOUSE
    2  CONTROLLER
    3  TOUCH
  ```

  The likely one is marked: Touch where the only pointer is coarse, Controller where a pad has been seen, else the keyboard. The
  choice is the press that boots it, so sound starts with it. It is kept in the page's settings, the plate says *PLAYING BY TOUCH:
  CHANGE* afterwards, and the Settings panel has it.
- **What each one changes.**
  - *Keyboard and mouse*: today's, with Part A. The mouse is taken at the takeover.
  - *Controller*: the hints speak the pad's language from the start; the page never asks for the mouse and never shows *CLICK TO TAKE
    THE MOUSE*. Sound still waits for a key or a tap (a pad's press doesn't count to the browser).
  - *Touch*: Part C. No mouse is taken. On Android the choice's tap also asks for full screen and holds the screen sideways
    (section 6.8). The canvas becomes the stage (section 6.5), and the hold to take over is a finger.
- **It leads** (Decided 9): a pad picked up or a key pressed still works, as keys and the pad do between them today, but the touch
  controls show only in Touch.
- **The game is told** (`AppConfig.input`: keys, pad, touch, with the pointer on in all three; the Windows shell leaves it at none),
  since the game draws the touch controls, words the plate and speaks the hints.

## 6. Part C: touch

### 6.1 The layout

On a phone held sideways (the stage of a Pixel 7 in game pixels, the picture in the middle):

```
 0          80                                              400       480
 +-----------+---------------------------------------------+-----------+  0
 | (1) RUNE  |                                             | (2) GIFT  |
 | hold:     |                                             | hold:     |
 | PAUSE     |                                             | WHEELS    |
 +-----------+                                             +-----------+  54
 | WISPS MAP |                                             | WARDROBE  |
 |           |                                             |           |
 |  left     |                the picture                  |  right    |
 |  strip:   |                 320 x 200                   |  strip:   |
 |  USE /    |                                             |  CAST /   |
 |  ACCEPT   |                                             |  BACK     |
 |   (o)     |                                             |   (o)     |
 |  walking  |                                             |  turning  |
 |  stick    |                                             |  stick    |
 +-----------+---------------------------------------------+-----------+  216
```

Every box drawn here is a headstone or a tablet in the run's stone (section 6.5). The sticks float, so the rings are only where a
thumb last came down.

### 6.2 What each control does, by where you are

| Control | The dream | The title | In play | A wheel up | A menu or card |
|---|---|---|---|---|---|
| A finger held anywhere, three seconds | takes over (H) | | | | |
| Left strip, a tap | | the choice (Enter) | E: open, take, use, speak | | Enter: take what is chosen |
| Right strip | | back | cast; held, again as it comes round, or charged; dragged, it turns too | puts it away | Esc: back |
| Walking stick | | | walk and strafe by the lean; pushed forward past its ring, the sprint; touched twice and pushed, Swift Dodge | | |
| Turning stick | | | turn: by the finger's travel near its middle, on past its ring | | |
| Upper left, a tap | | | the rune readied (the pad's RB; the Mirror is raised this way) | | |
| Upper left, held | | | Esc's question | | |
| Upper right, a tap | | | the right mouse's: the gift or rune Q readied last | | |
| Upper right, held | | | the spells' wheel, a tab at its head for the runes' and gifts' | | |
| MAP (once bought) | | | the map, and again to close it | | |
| WISPS | | | L: a tap the exit wisp, held the list | | |
| WARDROBE | | | Tab | | Tab |
| A tap on the picture | | a slab, a grave | a hint's cap or her rooms' prompt; on its lower edge, the status bar | a slot, the tab | a row, a cap, a socket |
| A drag on the picture | | | | | a list scrolled; the map panned |

- **Taps, holds and drags** (first numbers, to be tuned by hand): a tap is down and up within a quarter second, moved less than 6
  game pixels (about 2 mm on a phone); a hold is 0.3 s still (`WHEEL_HOLD`, as the pad's). A stick floats: its base is where the thumb
  comes down in its half, its ring is 24 game pixels across (about 7 mm), its knob follows the thumb, and the lean is the distance over
  the ring past a dead sixth. Swift Dodge's two touches come within 0.3 s, as the keys' double tap does.
- **Who gets a finger**: the buttons first (the corners, MAP, WISPS, WARDROBE), then a cap, a row or the prompt under it, then the
  strip's tap, then the stick.
- **The finger that took over** counts only once it is lifted, as a pad's button does (`PADX.fresh`).
- **Mostly the pad's paths.** `touch_keys` runs after `pad_keys` and writes what the pad writes: the walking stick into `IN.lx`/`IN.ly`,
  the turning into `IN.mdx` (travel) and `IN.rx` (past the ring), the sprint into `PADX.run`, a dodge into `PADX.dodge`/`dodge_a`, and
  keys pressed by the buttons. Walking, turning, the sprint and the dodge need nothing new, and the pad's hints stay off.
- **The wheels stay up on touch.** On the pad a wheel is held open and let go to take. On glass the hold opens it, the finger lifts,
  and it stays (the world grey and slow, as now) until a slot is tapped, the tab swaps wheels, or the right strip or a tap outside
  puts it away (`WH.sticky`).
- **Her rooms** (1.0.0b21.3). The slim prompt names the left strip's mark in place of E, and is a cap itself: the left strip or a
  tap on the prompt opens what it names. The buying screen is worked as any menu: the left strip buys, the right strip closes, its
  caps hold and go round the wares.

### 6.3 The takeover and the title

- **A finger held anywhere** on the stage for three seconds is H held: `app.c` counts it with H and the pad's buttons, in the crypt
  and in HAUNT.EXE (cutting to the crypt, as H does). A quick tap does nothing, as a pad bumped on a desk does nothing. The plate reads
  *HOLD A FINGER DOWN TO TAKE CONTROL*, and in HAUNT.EXE *HOLD A FINGER DOWN TO ENTER THE CRYPT* (37 letters: it fits HAUNT.EXE's 720
  pixels at its doubled size, where *ON THE SCREEN* would run off).
- **The title** says *TOUCH THE SCREEN* where it says *PRESS ANY KEY*; a tap on a slab or a grave chooses it, a second takes it, and
  the right strip goes back. THE CONTROLS shows the touch layout as the pad's screen shows the pad: a phone's outline, each control on
  a leader (`ui_touch`, beside `ui_pad`).

### 6.4 What the answers settle

- **The left strip and the walking stick share the left thumb.** A tap is decided as the finger lifts (E's jobs aren't timed to the
  moment); a drag is the stick.
- **The right strip is a button at once**, dragged to turn as well, so casting answers on the touch and can be held, and turning never
  casts by accident.
- **The corners fire as the finger lifts**, so pausing never spends the rune; a parry rises 0.1-0.2 s later, against its 0.6 s of true
  parry.
- **Keys touch has no button for**: B and C through Esc's question, L the WISPS button, H the picture's lower edge, the sprint and Swift
  Dodge on the walking stick, 1-9 on the wheel. The console needs a keyboard, so it isn't on touch.

### 6.5 Drawing the controls: headstones in the margins

**The stage.** Phones' margins lie outside the picture, so the page tells the game the stage: its size in game pixels (the window's
device pixels over the scale), where the picture lies in it, and the safe area (the camera's cut-out, the gesture bar). In Touch the
canvas is the stage, shown at the picture's whole-pixel scale: 480 x 216 on a Pixel 7 held sideways, the picture at (80, 8). The game
draws the picture there (the slow drift moves it within the stage) and the controls in the margins.

**Gravestones** (the owner's word with the answers). Every control is cut in the run's stone, as the cards over the live crypt are, so
it changes as the runs do and matches her hall in her rooms:

- **The strips** are tall headstones, one in each margin, weathered as the title's graves are (chips at the edges, a crack, lichen
  low down). Each has its mark cut and inlaid near its head (a hand for USE, a flame for CAST) and its word below it. In a menu the
  same stones read ACCEPT and BACK.
- **The corners** are small headstones with their marks: a rune for the rune readied, the gift's own mark, or a rune if a rune was
  readied last. The hold's words are cut smaller under the mark (PAUSE, WHEELS).
- **MAP, WISPS and WARDROBE** are small tablets set between the corner and its strip: a scroll, a wisp, a robe.
- **The sticks' rings** are roundels cut into the strip's stone where the thumb came down (the wheel-cross's ring), the knob a boss of
  bone. The walking stick's ring lights gold past its rim while the sprint holds.
- **Pressed**, a stone sinks a pixel and its mark goes from bone to gold, with a tick. A button with nothing to do (MAP before the
  map is bought, the corner with nothing readied) is a weathered blank stone, its mark worn away.
- **On a tablet**, where there are no margins, each control is a small headstone of its own over the picture's edge, and nothing else
  of the stone is drawn.

**Contrast.** Each mark and word is inlaid in bone, or in gold while pressed or chosen, at the top of its ramp, on the stone's darker
half, with a cut shadow on its lower edge so it reads on any of the runs' stones. That gets a bar of its own: `build/uisheet touch`
works out the contrast of every mark and word against the stone behind it from the palette (as the browser's accessibility rules
reckon luminance), on each of the four runs' stones and her hall's, and fails under 4.5 to 1 for words and 3 to 1 for marks, as `uisheet
fit` fails on an overflow. The settings' strength (6.7) fades only the stone outside a control's plate, never the plate behind its mark
and word, so the bar holds at every strength.

**The sheet comes first** (step 5): each control idle, pressed, blank and in a menu, on all five stones, a Pixel 7's stage and an iPad's,
the margins on plain black and with a dim earth at the headstones' feet, for the owner to choose.

Two things to know for the build. A stage of a phone held sideways fits inside a `Frame` (720 x 400 pixels) at the scales phones use;
upright it doesn't, which is one more reason for Decided 7. And the kit's marble and its fit checks are 320 x 200 (`MW`, `MH` in
`crawl_ui.c`), so the margin's stones are drawn small and placed, not laid on the kit's whole-screen marble.

### 6.6 The hints in touch's words

`pk` gets a third language. Each control's mark is drawn in a hint where its key would be, as the pad's faces are drawn in their
colours: *E TO OPEN* with the hand's mark, *ESC BACK* with the right strip's, *A/D PAGE* with two arrows. Every cap can be tapped
(Part A). W/S drops out of the hints, since a row is tapped. BACK is a key only where a group begins, as 1.0.0b21.3 made it.

### 6.7 The touch controls' own settings

THE TOUCH CONTROLS, from Esc's question while playing by touch (as THE CONTROLLER is while a pad is in): the turning's pace,
left-handed (the layout mirrored), the stone's strength over the picture (for a tablet), sticks floating or fixed, buzzing on or off.
Kept in the prefs record's next version, beside the pad's layout.

### 6.8 A phone's own trouble

**Android** (the owner's, so tried by hand):

- **Full screen and sideways.** Chrome lets the page go full screen from a tap, and then hold the screen sideways
  (`screen.orientation.lock`). The plate's TOUCH is that tap; after it, *FULL SCREEN* on the page's bar asks again if it was left.
- **Installed as an app.** A web app manifest (`display: fullscreen`, held sideways) lets Chrome put it on the home screen without the
  browser's bars. On Android it shares its storage with Chrome, so the ledger is the same.
- **Edge gestures.** Android's gesture navigation takes a swipe in from either side as Back. Taps are safe; a stick begun at the very
  edge may be taken by the system. The live ground keeps a little in from the edges, and a finger the system takes (`pointercancel`)
  is let go.
- **Long presses.** The context menu, text selection and the callout are turned off over the stage.
- **Buzzing.** `navigator.vibrate` stands in for the rumble (Decided by default; the touch settings' buzzing turns it off).
- **The wake lock** works in Chrome; the page already asks for it.
- **Speed and heat**: unknown until measured, step 0.

**iPhone and iPad** (not tried; what is known):

- Safari lets a page go full screen on an iPad but not on an iPhone, so the bars stay unless the page is added to the Home Screen. The
  same manifest serves, but there a Home Screen app keeps its storage apart from Safari's (as far as is known), so its ledger starts
  fresh. The page says so; the ledger's file carries it across.
- Safari can't hold the screen sideways, and its swipe from the left edge goes back a page.
- The silent switch mutes the Web Audio the game's sound uses, unless the page asks for playback (Safari's audio session, where it has
  it).

## 7. What doesn't move

The simulation, the saves (but for the prefs record), the dream and its numbers, the harness's balance runs, HAUNT.EXE's art, and
everything on Windows. In the browser with the keyboard and mouse nothing changes in play; menus gain the cursor. The pad's layout and
screen are as they are.

## 8. Steps

0. **The probe on the owner's Android phone.** Today's `/play/` with `?perf` (a corner readout of ms a frame from
   `window.HAUNTED.perf`) and `?take` (H held for three seconds as it boots, since a phone can't take over yet): the dream, the crypt in
   play, the title. If the title is too slow there (8-14 ms natively might be 30-40 ms), it gets a cheaper way to draw first.
1. **The input's shape.** `Input` gains the pointer, up to five fingers (id, place in stage pixels, down, moved, up) and the stage
   (size, the picture's place, the safe area); `AppConfig.input`. `web_main.c` exports them; `tools/host.c` reads `p` (a pointer's
   place), `T` (a finger: `T0d40,150`, `T0m60,150`, `T0u`), `HOST_STAGE` and `HOST_MODE`. Nothing behaves differently yet.
2. **The mouse in menus** (Part A): the hot list in the kit, `ptr_keys`, the cursor (its sheet first), the title's rectangles, her
   rooms' prompt, the wheel by pointing, lists by the wheel and by dragging, the right button as back. Every screen walked with
   `MOUSE=1`.
3. **The plate's question** (Part B): the three choices, kept, changed in Settings; what each changes in the page; the game told.
4. **Touch, the fingers' rules** (6.2, 6.3): `touch_keys`, the strips, the sticks with the sprint and the dodge, the corners and
   buttons, the takeover by a finger in `app.c`, the title's words and taps, the sticky wheel, her rooms. In the harness with scripted
   fingers, nothing drawn yet but a debug outline.
5. **Touch, drawn** (6.5): the sheet first (`build/uisheet touch`, with its contrast check), then the stage in the game and the page,
   the safe area, the strength setting.
6. **Touch's words and the gaps** (6.4, 6.6, 6.7): the hints, THE BESTIARY and THE CODEX in Esc's question, THE TOUCH CONTROLS and its
   prefs, the title's touch layout.
7. **The browser's side for phones** (6.8): Pointer Events with capture, full screen and sideways on Android, the manifest, the
   callouts, the edges, buzzing, the plate's words for a phone; the iPhone's silent switch.
8. **The checks.** The harness: every screen by scripted fingers on a Pixel 7's stage and on an iPad's, and with the mouse; the
   contrast check on every stone; ASan. Playwright in CI: a phone's shape with touch (a held finger taking over, a stick walked, a menu
   tapped) and a mouse through the menus. By hand: the owner on their Android phone, and with a mouse in a desktop browser. The README,
   the page's *Beta status* and the release notes say what was tried and how, and that no iPhone or iPad was.

## 9. Size

As a guess: in the game, the pointer and its hot list some 500 lines, touch's rules 600-900, the stones and marks 500-800, the hints and
the gaps 300; the page some 300 lines of script; the harness, the contrast check and the smoke test 400. Some 3,000 lines in all, about
half of it touch's.

## 10. Questions (answered 10 November; kept for the record)

**T1. The edge strips and the sticks.** How do the strips share the ground with the sticks?
(a) The walking stick floats wherever the left thumb lands in the left half; a quick, still tap in the left strip is interact or
accept. The right strip is the cast from the moment it is touched, held to cast again or charge, and dragged it turns too; inward of it
is the turning stick alone *(recommended)*; (b) the strips are buttons only, and the sticks float between them and the middle; (c) both
strips shared, decided on the lift. **Answer: (a).**

**T2. The turning stick.** (a) A stick: the further it leans, the quicker; (b) a swipe: the finger's travel turns the view; (c) both:
travel turns it near the middle, and past the ring it keeps turning *(recommended)*. **Answer: (c).**

**T3. The upper right corner's tap.** (a) What Q readied last, the right mouse's job *(recommended)*; (b) a cast. **Answer: (a).**

**T4. A corner tapped or held.** (a) The tap fires as the finger lifts, the hold opens after 0.3 s *(recommended)*; (b) the tap fires as
it is touched. **Answer: (a).**

**T5. Homes for what touch lacks.** (a) WISPS beside MAP; THE BESTIARY and THE CODEX in Esc's question; the status bar on the picture's
lower edge; the sprint past the walking stick's ring; Swift Dodge by two touches and a push *(recommended)*; (b) more buttons; (c) other
homes. **Answer: (a).**

**T6. Who draws the controls.** (a) The game, in the margins *(recommended)*; (b) the page in HTML; (c) the game over the picture only.
**Answer: (a)**, in the gravestone theme with good contrast (section 6.5).

**T7. A phone upright.** (a) Sideways to play, upright to watch *(recommended)*; (b) an upright layout too. **Answer: (a).**

**T8. When the question is asked.** (a) The first time, remembered, changeable *(recommended)*; (b) every visit; (c) never. **Answer:
(a).**

**T9. Does the choice shut out the others?** (a) It leads *(recommended)*; (b) strict. **Answer: (a).**

**T10. The mouse in menus on Windows too.** (a) Yes *(recommended)*; (b) the browser only. **Answer: (b).**

**T11. A tap on a row.** (a) The first chooses, a second takes *(recommended)*; (b) a tap takes at once. **Answer: (a).**

**T12. The mouse free in the browser's menus.** (a) The game's own cursor under the lock *(recommended)*; (b) the lock let go in menus.
**Answer: (a).**

**T13. The release.** (a) 1.0.0b22, the others moving on; (b) after them; (c) the mouse first as 1.0.0b21.3. **Answer: 1.0.0b21.4**, as the
owner released 1.0.0b21.3 ("Her Wares, Weighed") while this was being planned.

**T14. Phones to test on.** **Answer: Android.**

## 11. Hand-off log

- **10 November.** The draft: what the browser, the input, the takeover, the title, the menus and the wheels do today, and the plan.
  Nothing built.
- **10 November, the answers.** The owner took every recommended answer but T10 (the browser only) and T13 (1.0.0b21.4), will test on
  Android, and asked for the edge controls in the gravestone theme with good contrast. Main at 1.0.0b21.3 merged into the branch, and the
  plan read against it: the slim prompt in her rooms, the buying screen (`UI_BUY`), `pk`'s BACK, Q's hold at a pedestal. Settled;
  nothing built. Next: step 0, the probe on the owner's phone.
