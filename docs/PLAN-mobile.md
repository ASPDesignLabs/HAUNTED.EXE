# 1.0.0b21.4: touch, the mouse in menus, and asking how you play

**Status (10 November 2026): the touch controls and the plate's question are built and tried by the owner on their Pixel 10a,
who said they work well enough to publish. Releasing them as 1.0.0b21.4 is the next task, for another agent: *Start here*, below,
is all of it. Part A (the mouse in menus) is not built.** On `claude/funny-meitner-yn953c`, with main at 1.0.0b21.3 merged in; then
carried on to `claude/brave-albattani-sqvb5n` (a fast-forward), where the stones had their art pass at the owner's word (section 11's last
entry), so that branch is the one to release from now. The
owner asked for a plan to play the browser build on a phone or tablet, with a layout of their own, formal mouse support, and a
question on the browser's page of how a person is playing. They answered T1-T14 the same day (section 10, kept for the record):
the recommended answer to each but T10 (the mouse in menus in the browser only, not on Windows) and T13 (it goes out as **1.0.0b21.4**,
after their 1.0.0b21.3). It will be tested on **Android**. They added one thing: the new controls at the edges keep the game's
gravestone look, with enough contrast that their words and marks are easy to read (section 6.5). The art pass's rest stays 1.0.0b22 and
the lower storey 1.0.0b23.

## Start here: releasing 1.0.0b21.4 (the next agent)

The owner (10 November, after trying the touch build on their phone): *"This works well enough to publish."* So what is on
`claude/brave-albattani-sqvb5n` goes out as **1.0.0b21.4**: the touch controls, the plate's question, the probe, and the stones' art pass
(asked for after that word; ask the owner whether they want to try it on the phone first). Part A, the
mouse in menus, is not in it; say so in the notes, and ask the owner which number it takes when it is built (the plan's own order
had it first; the owner moved touch ahead of it). Nothing on the branch changes Windows' play: touch is behind `IN.touch`, which only
the browser's shell sets, and the prefs record grew from 24 bytes to 26 (`PL_BYTES_TOUCH`), which `win_main.c` reads (it takes up to
64, `pmn >= 23`) and 1.0.0b21.3 reads too (it stops at 24).

1. **Bring main in.** `git fetch origin main`; main was at `9873164` (1.0.0b21.3) when this was written and is merged into the branch.
   If it has moved, merge it into the branch and build (`make host`, `make web`) before anything else. Emscripten 6.0.13 isn't in the
   container: section 11's *Next: the first cut* says how to fetch it (about two minutes).
2. **Check.** `make smoke`; `make web`, then `tools/web_smoke.js` on a site folder with the build in `play/` (`sh tools/build_site.sh
   <dir>` and `cp -r build/web <dir>/play`; it fails on `build/web` itself, which has no `/play/`); `make scr` if the cross compiler is
   there (CI builds and smoke-tests Windows anyway). `node tools/touch_smoke.js build/web <shots dir>` walks the touch build as a
   Pixel 7 held sideways with a faked camera strip (fingers by CDP), prints its errors (none) and writes frames to look at: the stones
   clear of the picture (`t-a-room.png`), a turn (`t-b-turned.png`), Esc's rows (`t-e-raised.png`, TURNING at 110%). Nothing in the harness drives fingers yet (`HOST_STAGE` and `T` are still to do),
   so ASan can't reach the touch code; say so rather than claim it.
3. **Write it up** (CLAUDE.md's *Releasing*): `VERSION` to `1.0.0b21.4`. The README: what touch is (a phone or tablet held sideways;
   the plate asks Keyboard and mouse, Controller or Touch; a finger held three seconds takes over; the stones and what each does,
   section 6.1-6.2 and the controls card's lines; sliding on the picture's right half turns; Esc's TURNING and SMOOTHING), and under
   *What has been verified*: headless Chromium as a Pixel 7 held sideways, and the owner's Pixel 10a by hand (the probe's numbers in
   section 11: 119 frames a second in every phase; the owner's verdict), no iPhone or iPad, no tablet's stage, no mouse in menus.
   `site/index.html`: the BROWSER cell's words and the *Beta status* table, for the released build only. `.github/release-notes.md`
   rewritten for a player (no technicals or debug switches, as 1.0.0b14's notes set out). CLAUDE.md: the current release line, and
   the touch paragraph's state. A preview strip isn't needed; if one is wanted, a phone-shaped frame from Playwright shows the stones.
4. **Publish**, at the owner's word, given above: squash the branch onto main as one commit, as every beta has gone
   (`git checkout main && git merge --squash claude/brave-albattani-sqvb5n`, one commit titled *1.0.0b21.4: ...*; ask the owner for
   the release's name if none is given; *Touch* will do), push main, and let CI make the tag (pushed tags are dropped by the proxy).
   CI builds, smoke-tests and publishes the release when `VERSION` has none; Pages then serves `/play/` from the release's web zip,
   which is how touch reaches the public page.
5. **Check it went out:** the Build run on main, the release through the REST API (`gh api
   repos/ASPDesignLabs/HAUNTED.EXE/releases/tags/v1.0.0b21.4`; `gh release view` is blocked in cloud sessions), each asset, the live page
   and `/play/` on a phone-shaped Playwright page (the plate asks; Touch boots).
6. **Tell the owner** it is out, and what was and wasn't tried. The private test pages stay: the owner asked (10 November) to keep the
   first (https://claude.ai/artifact/V5ALUvTRsvhGq3eHey5C4Z, the touch branch's session's, which a session in another organisation can't
   update), and the stones' art pass is on https://claude.ai/artifact/L28ZMsohLh2N4vjdx9dnuB. A new touch build for their phone goes to
   whichever the session can write (`python3 tools/probe_page.py <dir> perf`, republished to the same address, the wasm as
   `application/wasm`); a read of the page says whether it can.

What isn't done, for the notes' honesty and for the work after the release, in order: THE BESTIARY and THE CODEX can't be reached by
touch (rows of Esc's question, `quit_rows`); the title's THE CONTROLS lists the keys, not touch; the harness's fingers (`HOST_STAGE`,
`T`); THE TOUCH CONTROLS' other settings (section 6.7: the stones'
strength, left-handed); the map panned by a drag; Part A (the mouse in menus, rows, caps and her rooms' prompt tapped); a tablet's
stage (a 4:3 screen takes a smaller picture so the stones fit; untried); HAUNT.EXE has no stage (its picture alone); the turning's 150
degrees and the smoothing's numbers are first guesses the owner hasn't tuned further. The owner may also want a flick to coast on
(momentum), which section 11's last entries leave open.

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

0. **The probe on the owner's Android phone** (**built**, 10 November; waiting for the phone's numbers). Today's page with `?perf`
   (a readout at the lower left) and `?take` (H held for three seconds once the dream has run 20 s, `?take=s` for another wait, since
   a phone can't take over yet): the dream (HAUNT.EXE), the title, and the crypt's dream after it. If the title is too slow there
   (8-14 ms natively might be 30-40 ms), it gets a cheaper way to draw first. The crypt in play isn't measured: the probe presses
   nothing but H, and a person's crypt draws what the dream's does with a status bar more. Pages serves only the release named in
   `VERSION`, so the probe went up as a private page on claude.ai (section 11). Failing that, Chrome's remote debugging over USB reads
   `window.HAUNTED.perf` from today's `/play/`, for the dream at least.
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
- **10 November, step 0 built.** In `web/haunted.js` only (the game is 1.0.0b21.3's, untouched): `?perf` or `#perf`, `?take[=s]` or
  `#take[-s]`, or `window.HAUNTED_PROBE` set before the script. The readout (`#probe` in `web/index.html`; `probeFrame`, `probeText`,
  `probeStart`) keeps each frame's own work (the game's step, the sound, the picture, and the picture split into the game drawing it,
  `web_frame`, and the page putting it up) in quarter-millisecond histograms by what is on the screen (HAUNT.EXE dreaming, the crypt
  dreaming, the title untouched, taken over once a person touches something), with the frames' gaps (the display's Hz from the median,
  late past one and a half times it, over 50 ms), the last five seconds, a mean a minute (for heat), and the device: its model where
  Chrome tells it (`userAgentData`), cores, memory, the GPU's name, the window, the picture's scale and the stage in the picture's pixels,
  the sound's rate. It starts folded to one line, opens on a tap; Copy (or, refused, the text selected), Share where allowed.
  `window.HAUNTED.probe()` gives the text. The take presses H and lets go after 3.3 s, trying again 8 s later if no takeover came; *CLICK TO
  TAKE THE MOUSE* is kept off. Three things for any page as well: a game that can't start (WebAssembly refused) says so on the plate in
  place of *Loading...*; a browser that refuses Web Locks boots anyway; and the plate's words wait on neither. `tools/probe_page.py` makes
  `build/web` into a page for a host that wraps the page and passes no query (no `<html>`, `<head>` or `<body>`, the probe on, the fonts
  inline, the link home absolute). Published privately on claude.ai as *HAUNTED.EXE Phone Probe*:
  https://claude.ai/artifact/V5ALUvTRsvhGq3eHey5C4Z (the owner's to open; the WebAssembly file served as `application/wasm`). Whether
  that host lets WebAssembly run is untried: if not, its plate says so, and the fallbacks are Chrome's remote debugging on today's
  `/play/`, or the probe's two switches put on the live page in a point release, the owner's call. Tried here only: headless Chromium
  as a Pixel 7 held sideways (tapped to boot, `?take=5` and `#take-4`, the title for 90 s and the dream back after it), under a strict
  content policy with WebAssembly allowed and refused, and `tools/web_smoke.js` (passes). This container's numbers (a server's CPU, no
  GPU; not a phone's): HAUNT.EXE 0.9 ms a frame, the title 15-16 ms (15 of it drawing the picture), the crypt's dream 4.4-4.6 ms, all at
  60 Hz. So even here the title costs three to four times the crypt.
- **10 November, the probe's numbers** (the owner's phone, inside claude.ai's private page). A Pixel 10a, Android 17, eight cores,
  8 GB, a Mali-G715; the window 866 x 259 held sideways (the page's own bars take the rest), so the picture ran at three to one
  (366 x 229) on a stage of 757 x 226. Ready 0.4 s after the page opened, booted 0.1 s after the tap. HAUNT.EXE: 3.1 ms a frame
  (p95 4.9), 60 Hz. The title: 6.9 ms a frame (p95 20.4; the 524 ms frame is the one that builds it), 5.9 of it the picture drawn,
  119 Hz, 7% of frames late; a minute's means 13.1, 11.8, then 5.7 from the third minute on. The first two minutes cost twice as
  much because the title's shading cache (`SCC`, `SCC_N` 48 in `crawl_title.c`) fills as its loop comes round (this container showed
  the same 13.7, 12.1); nothing slowed as the phone warmed. **Verdict: the phone is fast enough, and the title needs no cheaper way to
  draw.** The half-second hold as the title opens could be smoothed one day. The crypt wasn't measured, since the title never gave
  the screen back.
- **10 November, the title stuck.** The owner: it stuck on the title when it should go back to the dream. Not proven, but the
  likeliest: today's page asks for the mouse on any tap of the picture (`takeMouse` from the stage's `mousedown`); if Android's
  Chrome granted the lock, the next tap was MOUSE1, which `title_any` counts as a key, so the title's menu opened, and `TS_MENU`
  never times out, nor can a phone back out of it (Esc, Backspace or the right mouse). The probe counted only key presses and the pad
  as touching, so it still said *untouched*. The touch build answers it: no mouse taken in Touch, and its right stone is Esc
  everywhere, the title's menu included. To do as well: the probe should count every input that reaches the game.
- **10 November, the order changed.** The owner needs touch to test further on the phone, so the touch controls (Part C) come before
  the mouse in menus (Part A). Menus are worked first by the sticks and the stones, as the pad works them; tapping rows comes with
  Part A's hot list after. The owner also warned that the week's use is nearly spent: keep this log current enough to hand on.
- **10 November, built: the touch plumbing** (committed; harness and CI only; nothing calls it yet, so with touch off all is as
  before: `make smoke` passes, `make web` builds).
  - `src/core.h`: `Finger` (id, place in stage pixels, `down` the step it lands, `up` the step it lifts), `TOUCH_MAX` 5; `Input.touch`,
    `nfinger`, `finger[]`, `stage_w/h` (0 no stage), `pic_x/y` (kept by the app), `safe_l/t/r/b`; `Scene.stage(Frame *st, px, py)`, the
    controls drawn round the picture (`scene_haunt` NULL; `scene_crawl` NULL until step 5 below).
  - `src/app.c/.h`: a finger held anywhere takes over as H does (`A.hold_pad` 2; the plate *HOLD A FINGER DOWN TO TAKE CONTROL*, in
    HAUNT.EXE *... TO ENTER THE CRYPT*); after each step a lifted finger is dropped and the rest lose `down`; `app_stage_place` (the
    picture centred, the slow drift within the margins) fills `pic_x/y` each step a stage is set; `app_present_stage(dst, sw, sh, bgr)`
    lays the picture on a `Frame` the stage's size (`STAGE`, w x h at most 288,000, else it falls back to `app_present` and returns 0),
    calls the scene's `stage` hook while a person has it by touch, and puts the hold plate at the picture's place (`hold_overlay`
    takes a stride and an offset now).
  - `src/web_main.c`: `web_touch_mode(on)`, `web_stage(sw, sh, l, t, r, b)`, `web_touch(id, x, y, phase)` (0 landed, 1 moved, 2 lifted
    or cancelled; a landing and a lift in one step make a tap the game sees whole), `web_stage_frame()` (RGBA, or 0); `web_release`
    keeps touch and the stage.
- **Next: the first cut of touch for the owner's phone**, designed as follows. Do them in this order; each wants committing.
  1. `src/scene_crawl.c`: a `TCH` block beside `PADX` (about line 1296) so every part can see it: `fresh`, `used`, each finger
     tracked by id (`role`, `btn`, `moved`, `spent`, where it landed, the stick's base, its time), the walking finger's last lift (for
     the dodge), the menus' repeat timers (as `PADX.rep/on`), what to draw pressed, the rings. Set `TCH.fresh = 1` wherever
     `PADX.fresh = 1` is (the takeover in `crawl_control`, `title_open`, `title_to_play`): fingers down then count once lifted.
  2. `touch_geo`, from `IN.stage_w/h`, `pic_x/y`, `safe_*`: a column each side, `col` the black margin if 60 or more, else the larger
     of a fifth of the stage and 60, never over 112; in it from the top a corner stone (height clamp(sh/4, 38, 60)), the small stones
     (left WISPS and MAP side by side, right WARDROBE; clamp(sh/7, 24, 34)), then the strip to the foot; the ring's radius
     clamp(sh/9, 18, 30) stage pixels.
  3. `touch_keys`, called from `crawl_input` after `pad_keys`, and while `title_up()` too (the title skips `pad_keys`); by context:
     the title, a wheel (`WH.open`), a menu (`G.ui`), play; nothing while the console is down.
     - A finger landing: on a stone, that stone; in the left strip, pending (a tap if it lifts within 0.25 s and 6 pixels, else the
       walking stick from where it landed); elsewhere in the left half, the walking stick; in the right strip, the cast (MOUSE1 down
       from the touch, a hit on landing; dragged, it turns by `IN.mdx`); elsewhere in the right half, the turning stick.
     - Walking: `IN.lx/ly` from the offset over the ring (up the screen is forward), at most 1, a dead sixth; past 1.35 rings and
       mostly forward, `IN.down[KEY_SHIFT]` (the sprint); the base drawn along past 1.8 rings. Landing within 0.3 s of the walking
       finger's last lift arms Swift Dodge, done once the lean passes 0.6 (`PADX.dodge`, `PADX.dodge_a = G.ang + atan2f(lx, ly)`).
     - Turning: inside the ring the finger's travel, `IN.mdx` (8 counts a stage pixel times `PL.turn` / 10, to tune by hand); past it
       `IN.rx` = the side x min(1, (|dx| - ring) / ring), through `pad_turn`'s curve.
     - Play's stones: the top left tapped `PADX.rune_hit`, held 0.35 s Esc; the top right tapped MOUSE2 (`use_right`), held the
       spells' wheel, kept open (set `PADX.wheel_req` and a `TCH` flag, and have `wheel_input`'s `wheel_open(PADX.wheel_req, ...)` pass
       src 2, which no letting go closes); WISPS L's hit on landing and L down while held (the key's own tap and hold); MAP M, only
       with `RUN.has_map` (a worn blank stone without); WARDROBE Tab; a quick still tap on the picture's lowest 36 rows H.
     - A menu: the left strip Enter, the right Esc, the top left Back (Backspace), the top right Space, WARDROBE Tab; any drag a
       direction (W/S/A/D) with the pad's repeat (0.4 s, then 0.12).
     - A wheel: no walking or turning; a tap, in the picture's pixels (x - `pic_x`, y - `pic_y`), against the wheel's middle (160, 88;
       R 54 for the spells, 60 for the gifts): within 0.45 R the hub, turning between the spells and the runes and gifts
       (`wheel_close(0)`, `wheel_open(other, 2)`); within R + 24 the nearest slot by its angle (`WH.hi`, `wheel_close(1)`); beyond,
       or the right strip, puts it away.
     - The title: at its prompt any tap is Enter (`title_at_prompt()`, defined after `TI`, which comes after `crawl_input`); then the
       left stone Enter, the right Esc, drags W/S/A/D. In `crawl_title.c` `PADMODE` 2 is touch: `any_prompt` gives *TOUCH THE
       SCREEN*, and `PADMODE ? ...` at its key captions (about lines 1062, 1083, 1095, 1112) becomes `PADMODE == 1 ? ...`; `title_render`
       passes `v.pad = TCH.used ? 2 : TI.pad`.
     - `TCH.used` while a finger was the last thing used; a key, the mouse or the pad clears it, as with `PADX.used`.
  4. The words. In play (`G.ui == UI_NONE`), with `TCH.used` and not `PADX.used`, `pk` gives touch's: ENTER and E USE, ESC PAUSE,
     W/S, WASD and A/D WALK, TAB WARDROBE, SPACE and CLICK CAST, Q and 1-9 WHEELS, B and C PAUSE, L WISPS, M MAP, RIGHT-CLICK GIFT,
     BACK as it is. In menus the keys' names stand, since there the stones carry the keys they press (ENTER, ESC, BACK, SPACE, TAB; the
     stick W/S A/D). The controls card (`render_messages`, its `HINT` and `more[]`, about line 30068) gets touch's lines, with no lone
     letters, which `pk` would turn: *LEFT THUMB WALKS, RIGHT THUMB TURNS*, *THE RIGHT STONE CASTS; HELD, AGAIN*, *TAP THE LEFT STONE TO
     USE*, *HOLD TOP LEFT: PAUSE; TOP RIGHT: WHEELS*, *PUSH THE WALK PAST ITS RING: SPRINT*, *TOUCH TWICE AND PUSH: DODGE*.
  5. `crawl_stage(Frame *st, int px, int py)`, the scene's `stage` hook (in `scene_crawl`'s initialiser in place of the NULL). Each
     stone is drawn into a 320 x 200 scratch `Frame` filled with `C_CLEAR` (the kit's marble and fit checks are 320 x 200), at a place
     that differs stone to stone so the veins do, between `ui_on(scratch, 0, "touch")` and `ui_off()` (the run's stone, as the cards
     over the crypt), then copied onto the stage where not `C_CLEAR`. The strips `SLAB_HEAD | SLAB_CALM`, the rest `SLAB_TABLET |
     SLAB_CALM`; the words `ui_text8(..., FACE_INSC, R_BONE + 14, TX_INLAID)`, gold (`R_GOLD + 12`) and the stone a pixel lower while
     pressed. Marks: CAST the spell in hand (`ui_socket` with `UI_NODE_ICON[el][ND_OPEN]` and `ELEM_RAMP[el]`, as `wheel_render`
     draws it); the top left the rune readied (`ui_socket_mark` with `RUNE_MARK[k]`, `gear_color(GK_DEF, k)`); the top right the gift
     (`BA_MARK[b]` through `ui_mark`, as at about line 32280). A stick down: a roundel where it landed and a knob of bone. The words
     by context: play USE, CAST, RUNE (small: HOLD: PAUSE), GIFT (HOLD: WHEELS), MAP, WISPS, WARDROBE; a menu ENTER, ESC, BACK,
     SPACE, TAB; the title ENTER and ESC only; a wheel ESC only. The dream draws none (the app calls the hook only while a person has
     it). The contrast check (`build/uisheet touch`, section 6.5) comes with the sheet for the owner.
  6. `web/haunted.js`, in Touch: `web_touch_mode(1)`; `fit()` makes the stage (the scale as now, whole device pixels a picture pixel;
     `sw = floor(clientWidth * dpr / s)`, `sh` likewise, s raised till sw x sh is at most 288,000), the canvas sw x sh, and
     `web_stage(sw, sh, ...)` with the safe area in stage pixels (read from an element padded by `env(safe-area-inset-*)`); `draw()`
     takes `web_stage_frame()` (or `web_frame()` if 0) and drops the CSS drift; Pointer Events on `#stage` whose `pointerType` is touch
     or pen: `setPointerCapture`, the place by the canvas's rectangle in stage pixels, `web_touch(id, x, y, 0, 1 or 2)` (a cancel is 2);
     no `takeMouse` and no mouse buttons from a touch; the probe's readout to the top middle.
  7. The plate's question (Part B): three buttons, KEYBOARD AND MOUSE, CONTROLLER and TOUCH, the likely one marked; kept as
     `settings().controls`, and in the Settings panel; afterwards *PLAYING BY TOUCH: CHANGE*. Touch's tap asks for full screen and
     `screen.orientation.lock("landscape")`, each allowed to fail (claude.ai's frame refuses both).
  8. Try it in Playwright's Chromium as a Pixel 7 held sideways (`devices["Pixel 7 landscape"]`, `hasTouch`; two fingers at once
     want CDP's `Input.dispatchTouchEvent`), then publish: `make web && python3 tools/probe_page.py <dir> perf` (perf, not take: it
     should start in the dream and stay there till a finger is held), republished to the same private page,
     https://claude.ai/artifact/V5ALUvTRsvhGq3eHey5C4Z, with the same `files` map (the wasm as `application/wasm`).
  9. After the first cut: the probe counting every input that reaches the game but its own H (in `key()`, and the mouse and the
     wheel); THE BESTIARY and THE CODEX as rows of Esc's question by touch (`quit_rows`, its spacing at about line 30595); the
     harness's `HOST_STAGE` and `T` fingers; the contrast check and the sheet; THE TOUCH CONTROLS' settings; then Part A's hot list,
     so rows, caps and her rooms' prompt can be tapped.
  - Emscripten 6.0.13 isn't in the container's image: `git clone --depth 1 https://github.com/emscripten-core/emsdk.git`, `./emsdk
    install 6.0.13`, `./emsdk activate 6.0.13`, `. ./emsdk_env.sh` (about two minutes; CI does the same).
- **10 November, built: touch's first cut** (steps 1-7 of the list above, as designed there; harness and headless Chromium only,
  nothing on a real phone yet). In `scene_crawl.c`: `TCH` beside `PADX` (its finger roles are `TF_*`, since `TR_*` are the things'),
  `touch_geo`, `touch_ctx`, `touch_shown`, `touch_wheel_tap`, `touch_keys` (from `crawl_input`, after `pad_keys`), `title_at_prompt`;
  `wheel_input` opens a wheel asked by touch with src 2 (`TCH.wheel`); `TCH.fresh` beside each `PADX.fresh`; `pk`'s touch words
  (`PK_TOUCH`) in play; the controls card's touch lines (`HLINE`); the stones, `stone_put` on a scratch `TSCR` and `ring_put`, in
  `crawl_stage`, the scene's `stage` hook. In `crawl_title.c` `PADMODE` 2 says *TOUCH THE SCREEN*. In `web/haunted.js`: `cfg.controls`
  (asked on the plate by three buttons, `#how`, then *Playing with touch. Change*; a key on the plate answers *keyboard and mouse*, a
  pad's button *controller*, so the smoke test still boots with Enter); in Touch, `fitStage` (the canvas the stage's size, `web_stage`
  with the safe area from `#safe`), `draw` from `web_stage_frame`, `finger()` on Pointer Events, no mouse taken, the readout at the
  top middle, full screen and the sideways lock asked at the choice; `key()` marks the probe touched for anything but its own H. Tried:
  `make smoke` and `tools/web_smoke.js` pass; Playwright as a Pixel 7 held sideways with CDP fingers (the scratchpad's `touch.js`):
  Touch chosen, a finger held three seconds took over from HAUNT.EXE, the title said *TOUCH THE SCREEN*, a tap opened its menu, a drag
  moved down it, the left stone took BEGIN A NEW RUN, her question and her menu were worked by the stones, no errors. Not tried: the
  sticks' feel and numbers (the turning's 8 counts a pixel, the ring), the wheel by touch, the dodge, the sprint, the map's stone with a
  map, a tablet's stage (controls over the picture), HAUNT.EXE's stage (it shows the picture alone: its 720 x 400 wants a stage too
  big), and every real phone.
- **Still to do after this cut**, in order: the owner's report from the phone, and tuning; THE BESTIARY and THE CODEX in Esc's
  question by touch; the title's THE CONTROLS in touch's words (it shows the keys' list); the harness's `HOST_STAGE` and `T` fingers;
  the contrast check and a sheet; THE TOUCH CONTROLS' settings; the map dragged to pan; Part A's hot list (rows, caps and her rooms'
  prompt tapped); the README, the page and the notes for 1.0.0b21.4.
- **10 November, the first cut on the owner's phone's page.** Republished to the same private page
  (https://claude.ai/artifact/V5ALUvTRsvhGq3eHey5C4Z, its version 2) from `tools/probe_page.py <dir> perf`: the readout on, no
  takeover of its own, so it starts in the dream; the plate asks how you play. Inside claude.ai's frame the window is short (259
  pixels sideways on the owner's phone) and full screen is refused there, so the stones are larger and the picture smaller than on
  the real `/play/` in full screen. Waiting on the owner's report.
- **10 November, the owner's readout from the first cut** (the Pixel 10a, 479 s, no words with it yet). Full screen was granted this
  time: 923 x 411 CSS pixels at 2.625, so the crypt stood at 5 device pixels a game pixel on a stage of 484 x 215, the stones' columns
  82 game pixels a side (about 2.5 cm of glass). Touch was chosen and a held finger took over: the phases came HAUNT.EXE, then *Taken
  over* (20,724 frames, about three minutes), then the crypt dreaming again for four. Every phase kept 119 frames a second. HAUNT.EXE
  2.79 ms a frame (0.2% late); the crypt dreaming 5.05 (0.2%); taken over 5.74, p95 9.88, 3.6% late, the most 394 ms, which is the title
  built the first time as the finger takes over (the untouched title's first frame was 524 ms in step 0's readout). *Taken over* is
  one bucket, the title, play and menus together, so the stones' own cost isn't apart from the title's; the minute means stayed
  between 4.9 and 5.7. So the phone runs the touch build at full rate, and the one hitch is a frozen half second as the title comes.
  A mend if it matters: build the title's still layers while the finger is held. The readout's stage line was HAUNT.EXE's would-be
  stage (it has none); it now prints the last touch stage in use (`probe.stage`, *The stage by touch: ...*), tried in headless
  Chromium. Still waiting on how the controls felt.
- **10 November, the owner's first report by touch** (a screenshot from the phone in the crypt, full screen). Two faults:
  1. *The stones cover the picture* (a no-go). The phone keeps a strip at its left for the camera (about 63 CSS pixels, the safe
     area), and the picture was still placed in the middle of the whole stage, so the margin left of it was under 60 game pixels and
     `touch_geo`'s fallback made the columns `sw / 5` wide, over the picture's edges (the status bar's ends hidden). Some words don't
     fit their stones and aren't drawn: the half stones WISPS and MAP, the corners' *HOLD: PAUSE* and *HOLD: WHEELS*. The mend: the
     picture in the middle of the safe area (`app_stage_place` reads the insets), the columns never wider than the margin beside it,
     and the page's `fitStage` taking a smaller whole scale when a margin would be under about 80 CSS pixels; WISPS and MAP one above
     the other, full width; a shorter word where one doesn't fit.
  2. *Turning is off.* The right stick turned by the finger's travel only within its ring (`touch_ring`, about 0.7 cm), then at a
     rate while the finger stayed past it, however still. The owner's way: slide a finger on the play field and the view follows it.
     Built as: a finger landing in play on the right half (outside the stones), and the cast strip's finger, turn the view by how far
     they slide, a full picture's width 150 degrees at 100%; no ring and no rate. Smoothing that falls off as the finger's speed rises
     (a 1-euro filter: steady when slow or held, little lag when quick), and on lifting what the smoothing still owed is turned at
     once, so a flick lands where the finger went ("trick shots"). Two settings, by touch, as rows in Esc's question in place of THE
     CONTROLLER (D or a drag right raises, A or a drag left lowers, ENTER steps round): *TURNING* 30% to 300% (`PL.tturn`, tenths)
     and *SMOOTHING* 0 to 10 (`PL.tsmooth`), kept in the prefs record's bytes 24 and 25 (`PL_BYTES_TOUCH` 26; older readers stop at
     24). If the owner meant the view coasting on after a flick, that is a few lines more.
- **10 November, built: the turning and the stones kept off the picture** (as the entry above designed; commit `8f06774`). In
  `scene_crawl.c`: `touch_follow` (the 1-euro filter and the flush on lifting, into `IN.mdx`), used by `TF_TURN` and `TF_CAST`
  fingers in play; the ring and the rate gone from the turn (`TCH.turn` and its ring drawing gone; the walking stick keeps its ring);
  `PL.tturn`/`PL.tsmooth` in the prefs (`PL_BYTES_TOUCH`); Esc's question by touch (`quit_touch`, `Q_TTURN`, `Q_TSMOOTH`,
  `quit_tune`; seven rows at 13 pixels apart). `touch_geo`'s columns from the safe area and never over the picture (none drawn under
  20 pixels), WISPS over MAP, `stone_put`'s colon split and GEAR. In `app.c`: `app_stage_place` in the safe area's middle, still while
  the stones are up. In `web/haunted.js`: `fitStage` steps the scale down until each side has 80 CSS pixels. Tried in headless
  Chromium as a Pixel 7 held sideways with a 63-pixel camera strip put on by hand (the scratchpad's `touch2.js`): the picture clear of
  the stones, the status bar whole, a slide of 120 stage pixels to the left turned the view about 55 degrees, Esc's rows read and a
  slide right raised TURNING to 110%; `make smoke` and `tools/web_smoke.js` pass. The private page republished (its version 3).
  Not tried: how it feels on the phone (the 150 degrees and the filter's numbers are first guesses), the smoothing's effect by hand.
  Next, after the owner's word on it: the rest of *Still to do after this cut*.
- **10 November, the stones' art pass** (the owner: the touch assets *"are too plain. They need the same attention to detail as our
  other gravestones, with intentionally designed spaces for contextual UI information to appear on them. They should appear more
  weathered, cracked, and damaged with more detail overall while retaining our same look as the game"*; and keep the private page).
  On `claude/brave-albattani-sqvb5n`, fast-forwarded from `claude/funny-meitner-yn953c`. The stones are now built with the title's
  graves' sculpt kit (`sc_*`), at the end of `src/crawl_title.c` (*Touch's stones*), declared in `crawl_title.h`: `TStone` (its kind
  `TSK_*`, size, column, blank, pressed, the mark inlaid in its socket `TSM_*`, seed, word, the plinth's word and mark), `TsInfo` (what
  fills its places each frame), `tstone_draw`, `tstone_places`, `tstone_word_fits`, `tstone_ring` (the walking stick). Each a monument of
  its own: the left strip a round-headed stone with scrolled shoulders and a hand inlaid in a sunk roundel, crossed bones cut low when
  there is room; the right a gothic stele, pinnacles with blind lancets when the column is wide enough, a gable with crockets and a
  cross for its finial, a blind lancet under it all and an iron cramp across its big crack; the corners broad stones on plinths, a
  low arch (left) or a gable with acroteria (right), dentils under the head, paterae, the hold's word cut along the plinth after an
  hourglass (PAUSE) or a wheel (WHEELS); the small ones tablets hung by iron pins, dentils at their foot, a scroll, a wisp or a robe
  inlaid in a roundel. All of them broken at an outer shoulder or corner and at the plinth's far corner, cracked from the breaks, chipped
  along every edge, with lichen in clumps (gold at its heart, grey-green round it, thickest on the outer side and the top), pits, water's
  streaks, rust run down from the iron, damp and moss at the foot, grass in front of the plinths, ivy up the strips' outer sides. The
  places cut for what the game says (`ts_lay`): a socket (sunk, a moulded ring round it) for what is in hand; a ring groove round it
  for its time; a tablet with dovetail handles (a tabula ansata) for a word or a number; a groove for a fill. What fills them
  (`crawl_stage` in `scene_crawl.c`, by touch's context): USE, the word for what E would do now (`use_word`, human_use's order: OPEN,
  ENTER, PUSH, DRINK, KNEEL or RISE, KINDLE, LEAVE, PRAY, SEARCH, LIGHT, DROP, a thing's TAKE, PLACE, TURN, PULL, STRIKE, READ; in her
  rooms BUY, DRAW, LOOK, SPEAK, READ, OPEN) and the sprint's breath in its groove (gold while sprinting); CAST, the spell in hand in the
  socket (the element's icon; Restoration's ankh), its time round the ring, its price in souls (red when it can't be paid; FREE; the
  spear's bones, or its splinters), a charge filling the groove in gold; RUNE, the rune readied, its time round the ring, its quality's
  tag (C to EX) in its colour, struck through (`SK_CLOSED`) when the void or a status bars it, NONE when there is none; the top right,
  the gift (its mark, its wait round the ring, its rank in pips) or the rune if Q readied one last; WISPS the souls held (red under a
  wisp's price); MAP the level's secrets found of all (gold when all are), worn blank till it is bought; WARDROBE (GEAR where it won't
  fit) the bag's places used (red when full). In a menu, the title or a wheel the stones are the keys they press, with marks (a tick,
  a cross, an arrow back, a space bar) and ACCEPT, CHOOSE, BACK or AWAY in the strips' tablets. Pressed, a stone sinks a pixel and its
  words and marks go from bone to gold; blank, its socket is empty and cracked, its words worn nearly smooth and moss over its foot.
  Each is built once for its size and state and kept (`TSC`, forty stones, the least used going first; it doesn't touch the title's
  own cache, `SCC`), so a frame only copies them and fills their places: in headless Chromium as a Pixel 7 the frames ran 4.1-4.8 ms
  (5.1 before). The sheet is `build/titlemock <dir> touch` (`docs/touch/stones.png`: each run's stone in play, pressed, blank and full,
  and in a menu; `docs/touch/sizes.png`: a narrow, a wide and a short stage, where the narrow tablets drop their roundels and the
  plinths' marks give way before their words), and it prints each run's contrast: the words' and marks' darkest inlay (bone 13, gold
  10) against the lightest quarter of the stones as drawn, 5.5 to 7.7 to 1 across the four runs (the plan's bar is 4.5). The stone was
  darkened a little and the inlay raised to the top of its ramp to get there. `docs/touch/frames.png`: the build in Chromium (a room, a
  thumb on the walk and one on CAST facing the merchant, Esc's rows), and the first cut's frame for comparison. Tried: `make smoke`,
  `tools/web_smoke.js`, `tools/touch_smoke.js` (no errors) and a walk with two thumbs; ASan clean on the sheet (it found an overrun in
  the sheet's own short stage, mended). Not tried: on the phone; the harness still drives no fingers, so ASan hasn't run the game's
  side of it (`crawl_stage`, `use_word`). The private page couldn't take this build: from this session it reads as made outside the
  organisation, with no edit access, and the publish was refused (it was put up from the touch branch's session). It is kept as it was,
  untouched, at the owner's word; this build went up as a new private page, https://claude.ai/artifact/L28ZMsohLh2N4vjdx9dnuB (from
  `tools/probe_page.py <dir> perf`, the wasm as `application/wasm`), for the owner's phone.
