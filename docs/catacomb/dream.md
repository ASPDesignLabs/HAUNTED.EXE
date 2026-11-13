# The dream: the autopilot

Moved here word for word from `CLAUDE.md`'s *How CATACOMB works* when that file was slimmed (docs/PLAN-tidy.md, step 6); add to it as the game changes.
Where it names a function, search `src/crawl/` and `src/art/` for it: `scene_crawl.c` and `crawl_art.c` list their parts. Where it says
"above" or "below" it meant `CLAUDE.md` as it was: the save's format is `docs/SAVES.md`, the debug switches and the
harness `docs/HARNESS.md`, the known issues `docs/KNOWN-ISSUES.md`, and each release's account `docs/HISTORY.md`.

- **Autopilot** walks only lanes (`apass`), with quick 90-degree turns; `decide`, `pick_goal`,
  `key_goal`. It remembers treasure it has seen, leaves at 270 s (300 if above 60% health) or 330
  (360), or earlier once it has most of the treasure. It presses about a third of the secrets it walks
  past. Setting off for the exit (`G.leaving`), it casts an exit wisp with 2 spirit or more. It stops
  at a Black Altar within 24 steps (30 to anoint) from the lane beside it (`altar_spot`, `at_altar`,
  `PS_ALTAR`): anoints when it is gold, wakes it with 25 spirit until 180 s (`WAKE_TIME`), sends its
  wisp on when the price leaves it 15 spirit, until 240 s (`wants_send`), and blesses with 40 when the
  level has undead until 240 s (`ALTAR_TIME`); while the altar's wisp is out and there is time it walks
  to the secret it seeks (`G.altar_seek` as `G.seek_secret`), and calls it home once the wall has moved
  and it is out of the room and within four cells of it.

- **The autopilot in boss fights**: `find_target` takes a boss or its adds from any side within nine cells
  (before, only what was ahead or attacking, so a boss that never strikes, like the Deacon, was never
  aimed at); in a wave, `boss_goal` heads for the nearest add when none is within five cells.

- **The autopilot in chains** (`chain_goal`, ahead of `pick_goal`'s own; `use_spot`, `at_use`, `PS_USE`,
  `pocket_go`). In the level, an open chained room's doorway while it has two thirds of its health,
  before 180 s, and with time left of `CHAIN_TIME` (150 s a level in chains, `PK.auto_t`); each chain once
  (`PK.auto_done`). In a pocket: treasure it has seen within 16 steps, then the visportal or cooler at
  the end, else the way on; it turns back (`PK.auto_back`) under two thirds of its health, after
  `CHAIN_TIME`, or with the gift taken. It stands on the lane beside the cell before a doorway, turns and
  goes through. It now presses every secret it passes before 180 s, a third after. The debug build logs
  `EVENT chain_use`, `chain_back` (hurt, time, done) and, every 3 s in a pocket, `CHAIN ...`.

- **The autopilot's part in 1.0.0b11** (step 14). `auto_charge_pick` (from `aim_at` and `PS_HOLD`) sets `G.auto_chg`, and
  `PS_CAST` then charges (`G.chg_on`, as a person's) or holds Soul Tether (`G.auto_hold`); a blow breaks it as any wind-up.
  Her room: `hall_errand` (`bell_wanted`, `font_wanted`), `SH_HALL` with `G.hall_step` legs through the arch, `hall_act`;
  then the shrine.
