# 1.0.0b8: a game controller

**Status: settled (27 October 2026).** The owner asked for XInput support: holding any button on a
controller takes the dweller, or enters the crypt from HAUNT.EXE, in the screensaver and in the program.
They read the plan below and said to go ahead; the questions it asked take the answers it recommended.
All four steps are built and went out as 1.0.0b8, tested in the harness with a scripted pad only.

## What was asked

- XInput, for Xbox and other controllers.
- Holding any button takes control of the dweller, or cuts from HAUNT.EXE into the crypt, as holding H
  does.
- In the `.scr` if it can be done, else in the `.exe`. It can be done in both: once Windows has started
  the screensaver it is an ordinary program, and Windows pays no attention to a controller at all, so
  what a pad does is the game's to decide.

## Decided

1. **A quick press in the screensaver does nothing.** A pad bumped on a desk doesn't end it; keys and
   the mouse still do.
2. **Any button held three seconds takes over**, the triggers and the stick clicks included; the sticks
   themselves don't count. The plate reads *HOLD ANY BUTTON TO TAKE CONTROL* (or *TO ENTER THE CRYPT*)
   while a button is the thing held, *HOLD H* while H is.
3. **Full play with the pad**, by the layout below.
4. **The hints speak the pad's language**: when the pad was the last thing touched, every menu's hint
   line and every card's prompt names its buttons (A in green, B in red, X in blue, Y in gold), and the
   controls card is the pad's. Touch a key or the mouse and they go back.
5. **Rumble**: when you are hurt (by how much), a rock landing, a boss's fall, a secret wall grinding,
   the BFS9000.
6. **PlayStation and Switch pads later.** XInput covers Xbox 360, One and Series pads and most
   third-party PC pads in their Xbox mode; the others work today through a translator such as DS4Windows.
7. **A setting**: *Game controller* in the settings dialog, on by default (`Controller` in the registry).
8. **Its own beta, 1.0.0b8.**

## The layout

| Pad | In the crypt and the merchant's room | In a menu |
|---|---|---|
| Left stick | Walk and strafe, as far as it leans; click to run until it is let go | Up, down, left, right (repeating) |
| Right stick | Turn, faster the further it leans | |
| D-pad | Left the bestiary, right the Codex, down the status bar, up the wisps | Up, down, left, right (repeating) |
| RT | Cast (held, it casts again as the spell comes round) | |
| LT | The boss ability readied | |
| RB / LB | Next / previous spell | Next / previous page (as D and A) |
| A | E: open, use, buy, the cards' prompts | Enter |
| B | Swift Dodge, the way the left stick leans, else back | Esc: back, close |
| X | Q: ready the next ability; at a ware, hold it | |
| Y | L: the wisps (held, the list when L casts at a tap) | Space |
| View | Tab: the wardrobe | Tab |
| Menu | Esc: the question | Esc |

A button already held when the pad takes over counts only once it has been let go, so the trigger held
to take over doesn't cast.

## Windows

- **XInput** is loaded at run time (`xinput1_4.dll`, which every Windows 10 and 11 has, then
  `xinput1_3.dll`, then `xinput9_1_0.dll`); without it there is no pad and nothing else changes. No new
  dependency.
- All four pad slots are read every frame while connected; an empty slot is looked at every two
  seconds (asking an empty slot is slow).
- Dead zones: the sticks' own (XInput's suggested values), radial, the rest rescaled to 0..1; triggers
  past a tenth.
- The program window reads the pad only while it has the focus; the screensaver always.
- **Keeping the display awake.** Windows counts no controller as activity, so while a person is
  playing with the pad (touched in the last two minutes) the game tells Windows the display is needed
  (`SetThreadExecutionState`), and the program window refuses the screensaver and the monitor's sleep
  (`SC_SCREENSAVE`, `SC_MONITORPOWER`). An administrator's lock after inactivity may still lock.
- Rumble through `XInputSetState`, faded out, never left on: the motors are set to nothing on
  quitting, on losing focus and when the pad is put down.

## Building it

1. **Input** (built): the pad in `Input` (`core.h`), XInput in `win_main.c`, the hold in `app.c` and its
   plate, the setting, the harness's pad (`HOST_INPUT` `+PAD_A`, `aLX0.8`).
2. **Play** (built): the layout in `scene_crawl.c`, by context; walking and turning by the sticks.
3. **Hints and rumble** (built): the hint lines, the cards, the controls card and the ability pages in
   the pad's words; the rumble.
4. **Checks** (built): every menu and the crypt by a scripted pad in the harness; ASan; the Windows build; the
   README, the page and the release notes; the release.

Nothing of it can be tried with a real pad here (no controller, no Wine in this container): the harness
fakes a pad, so the layout and the menus are tested, how it feels is not.
