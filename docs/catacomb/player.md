# A person at the controls

Moved here word for word from `CLAUDE.md`'s *How CATACOMB works* when that file was slimmed (docs/PLAN-tidy.md, step 6); add to it as the game changes.
Where it names a function, search `src/crawl/` and `src/art/` for it: `scene_crawl.c` and `crawl_art.c` list their parts. Where it says
"above" or "below" it meant `CLAUDE.md` as it was: the save's format is `docs/SAVES.md`, the debug switches and the
harness `docs/HARNESS.md`, the known issues `docs/KNOWN-ISSUES.md`, and each release's account `docs/HISTORY.md`.

- **The game controller** (docs/PLAN-pad.md). `Input` carries the pad (`pad`, `pad_down/pad_hit[PAD_*]`,
  `lx ly rx ry` past the dead zones, y up, `lt rt`); the platform fills it (`pad_poll`: every connected
  slot each frame, an empty one every 2 s; the program only while in front, the saver always; no pad
  press ever ends the saver; while a person plays with it, `SetThreadExecutionState` every 20 s and
  `SC_SCREENSAVE`/`SC_MONITORPOWER` refused). `app.c` takes any button held as it takes H (`A.hold_pad`
  picks the plate's words), keeps the rumble (`pad_rumble`, `app_pad_motors`) and `AppConfig.pad`
  (registry `Controller`). In the game, `pad_keys` (from `crawl_input`) turns the pad into the keys each
  place reads, by context: in a menu the D-pad and stick are W/S/A/D with repeat, A Enter, B and Menu
  Esc, Y Space, the bumpers A/D, View Tab; in play A is E, X Q, Y L, D-pad up M (the map, 1.0.0b9), View Tab, Menu Esc,
  D-pad left/right/down B/C/H, RT and LT the mouse buttons, the bumpers the wheel, B Swift Dodge by the
  stick's lean (`PADX.dodge`, done in `update_human`), the sprint's button toggles the sprint (`PADX.run`; the left
  stick's click at first: since 1.0.0b18.1 a person lays out every job but View's and Menu's, the 1.0.0b18.1 bullet below); `human_walk`
  adds the sticks (`pad_turn`, `PAD_TURN`). What was held at the takeover counts once let go
  (`PADX.held`), and a direction that opened a menu isn't its first move (`PADX.menu`). `PADX.used` (the
  pad touched last, a key or the mouse turning it back) makes `pk` turn a hint's keys into buttons
  (`PK_KEY`/`PK_PAD`, face buttons marked `\1`), drawn by `hint8` in their colours (`hint_width`; `pkp`
  plain); only hint lines and prompts go through it, since a lone C or E elsewhere is prose. The
  controls card has a pad version, and `ab_text` the gifts' wording. `rumble()` (a person with the pad):
  hurts by share, death, a rock within 4 cells, a secret's slide, a boss's fall, the BFS9000.

- **Player mode** (`update_human`, `human_walk`, `human_use`, `human_cast`) after holding H. The hand
  is out of sight but for a cast or a ware reached for, and stays up 0.9 s after (`G.hand_base`,
  `G.ready_t`). It moves (1.0.0b14, at the owner's word): `hand_frame` picks one of `NHF` frames (`HF_*`: an idle of
  three while it is up, a cast of four by the cast's phase, the claw while a charge is held), each bent from the
  dressed traced hand by `crawl_hand_frame` (`HAND_AXIS`, `HAND_POSE` in `crawl_art.c`) when first wanted; the spell
  forms at the frame's fingertips (`hand_tip`). `CRAWL_HANDFRAME=k` holds one. Esc opens
  the question (flee, stay, sleepwalk, open the ledger, begin a new run: `Q_NEW`, asked twice by `G.quit_arm`,
  `run_new`: a fresh `RUN` as a person's, out of any pocket, page 0, so her question comes at once; `EVENT new_run`); Tab the wardrobe (the
  seven worn slots and the bag, Enter moves a piece between them; A/D turns to the caster's totals and
  the spellbook); B the
  bestiary (`render_bestiary`: a page a kind, A/D through them, W/S through its ranks, then the
  achievements).

- **The sprint, PHYSIC and the lantern** (1.0.0b15 step 0, docs/PLAN-ai.md sections 24-26). The sprint: Shift held, or
  the pad's sprint button as a toggle that ends when the stick stops leaning forward (`PADX.run`); forward only, half
  again the pace, `SPRINT_BASE` 2.5 s and SPRINT's (`sprint_max`); `sprint_tick` drains it while it carries you, then
  waits and fills the whole meter (`sprint_recovery`: 1.5 s and 3.5 s, a quarter off each for each whole hundred of
  health past 200, to 0.25 and 0.75; the owner's SP2 b); run dry, `G.spr_dry` until Shift is let go, and
  `SFX_BREATH`; the arrow keys' quicker turn with Shift stays free; `render_sprint` (a slim bar over the status bar's
  edge while it isn't full). Esc's question has `Q_PAD` while a pad is in (`quit_rows`): since 1.0.0b18.1 it opens the
  controller's screen, where the sprint is one job among the rest (the 1.0.0b18.1 bullet below).
  Two new stats at the end, `ST_SPRINT` (seconds, on boots, cap 7.5 over the base) and `ST_PHYSIC` (on robes and
  amulets, cap 400: `potion_heal`, 25 times one and it), and two lines at the end of `LINES` (`LN_SPRINT`, `LN_PHYSIC`, a
  point a step, no overlevels: `line_overs`); the ascend screen shows fourteen lines and scrolls; Tab's caster page
  has twelve rows down the left. The Lantern of the Dead (`lantern_held`, `lantern_lit`, `RUN.lantern_off`,
  `lantern_toggle`, `lantern_mark` on the portrait's corner): with it, L held 0.6 s shutters or lights it, a tap opens
  the list (on letting go), or with *L ALWAYS CASTS THIS* casts after `G.l_tap` and a double tap opens the list; the
  list has a lantern row. Debug: `EVENT sprint_on`, `sprint_dry`, `sprint_full`, `pad_sprint`, `lantern`.

- **The wheels and the Mirror by hand** (1.0.0b16 step 3, docs/PLAN-play.md sections 7 and 7a). The pad: D-pad up tapped the
  map, held (`WHEEL_HOLD`) the runes and gifts' wheel; down tapped the bar, held the spells' wheel; RB the rune readied
  (`rune_hand`, `rune_readied`), LB the gift readied; LT the right mouse (`use_right`); the bumpers no longer turn the spells.
  The keys: Q tapped readies the next (`ready_next`: the hand-raised runes, then the gifts; `RUN.rune_sel`, `RUN.mouse_rune`),
  held opens the wheel and the mouse points. `WH` (`wheel_input`, `wheel_fill`, `wheel_open`, `wheel_close`, `wheel_render`,
  `wheel_grey`): the world at `WHEEL_SLOW` and grey under it, `update_human` waiting. A person's Mirror rises only by hand
  (`rune_by_hand`), its first `MIRROR_TRUE` 0.6 s a parry (`G.mirror_true`: home, twice as hard); the dream's runes as before.
  The shrine's first curse gives a Common Mirror if none is held. Debug: `EVENT wheel_open`, `wheel_take`, `parries`,
  `shrine_mirror`, `CRAWL_PARRYT=s`.

- **The console** (`CON`, `con_update`, `con_render`, `con_run`, `con_do`; docs/PLAN-b5.md, step 6). The
  key left of 1 (`KEY_CONSOLE`, by scan code) drops a plate from the top while a person plays; the crypt
  waits and `snd_hold` holds the game's sounds while the music plays on. Its codes are secret: they are
  in the source only as hashes (`CON_CODES`) and a scrambled list (`CON_HELP`). Never write them in the
  repo, the README, the page, a commit message or a comment; the owner keeps the list. A code that
  works sets `CON.used` until the program closes: no achievements, and nothing signed or written. `build/version.h` gives it the version (any `make` of the harness
  makes it). The debug build logs each console line as `CONSOLE ...`.

- **The target's health** (section 2; `look_at`, `look_update`, `G.look_mob/look_t`, `render_target`). The
  creature nearest the view's middle within twelve cells and in sight, for 1.5 s after; anything a person hits
  for 3 s. At the top (under a boss's bar): name and rank, a bar and `small_digits` figures, the spell it takes
  worst on the left and best on the right (`weak_word`, from `Record.spell_kills` at three kills; `?` some
  spell not learnt and none says more, `--` all learnt and none differs; ties take turns). Spirits say nothing
  touches them; bosses keep their own bar; messages move below it.
