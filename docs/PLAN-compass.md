# The compass: a needle for what you have asked to find

**Planned, nothing built** (16 November 2026, on `claude/stoic-ramanujan-3iqb3m`, from main after 1.0.0b21.7). The owner asked for
"a compass item, that directs the player north and to one 'objective' they wish to track, should their skills or gear allow for it.
Keymaker pointing to keys, a wisp called for an exit pointing to it", art that "feels like it belongs in the UI", bought from the
merchant once the map is, for 2500g, and not kept through death. Settled by the owner on the first round of questions: see section 1.

*In a line: a wisp shows the way and costs a soul; the compass says only which way and how far, costs nothing once bought, and is
lost when you wake in the merchant's room after a death.*

## 1. Settled

| | |
|---|---|
| Bought from | the merchant, **only once the map is owned** (`RUN.has_map`), **2500g** (`COMPASS_PRICE`, beside `MAP_PRICE`) |
| Lost | **on a death, which wakes you in her room** (`death_wake`: it always runs, where `death_fall` returns early when a death costs nothing). Not lost by finishing a run, leaving a level or a replay |
| Chooses its objective in | **the L menu**; no new key, wheel or stone |
| The dial | **rotates**: north is always true, so the needle is read against your bearing |
| Autopilot | does not use it (as with Keymaker, `crawl_gear.c` 508): the dream must log as the build before, line for line |

## 2. What it does

- **A dial on the HUD** beside the MAP stone, blank until bought as the map's stone is. It turns as you turn so that its **N** tick is
  true north; a needle in the objective's colour leans toward the thing tracked. Under it, a short readout with the distance in
  cells and the quarter: `EXIT 34 NNE`. It is a bearing, never a route: it may point through a wall, and the way round is yours to
  find. That is what keeps it from replacing a wisp.
- **The objectives are the wisps' own.** `wisp_choices()` already lists them and `wisp_target()` already says where each is, past
  the keys an exit's vault needs first. The compass reads the same list: the exit always; each key lying, with Keymaker; the altar's
  secret while its wisp is out. A new wisp kind gets a needle with no more work, and gear and skills gate it for free.
- **Choosing.** In the L menu each row gains a second action, *TRACK*, beside casting. Tracking is free and costs no soul; the choice is
  kept as `RUN.wisp_kind` is, but separately (`RUN.compass_kind`, `RUN.compass_color`) so that "L always casts this" is untouched.
  With nothing tracked it tracks the exit. If the thing tracked is gone (the key taken, the secret found) it falls back to the exit
  and says so once.
- **Nothing to point at.** Inside a pocket the exit is the way back (as the exit wisp's). In her room, in the Backrooms and in a boss's
  hall where `wisp_target()` finds nothing, the needle rests and the readout says `NOTHING TO FIND`.

## 3. For whoever plays it (the owner dogfoods this)

- The needle is not read by colour alone: each objective has its own tip (the exit a diamond, a key a notch, the altar a ring) as well as
  its ramp, and the readout says the same in words.
- The dial turns smoothly but **never jitters or spins for show**: it eases to the bearing, and when the bearing is a wall's length off
  it does not hunt. With the small-life movement setting on *reduced* or *off* (docs/PLAN-ambience.md) the dial still turns, since it
  is information, but the needle does not swing past and settle.
- It is on the HUD where the eye already goes; it is not drawn on the map overlay, which stays as calm as it is.
- The sound of buying it is the map's; the compass makes none of its own in play.

## 4. Art

New art goes to the owner as a sheet before it goes in the game (`make sheets`). Three pieces, in the carved stone of docs/PLAN-ui.md,
cut with the `sc_*` kit:

1. **The dial** for the HUD's stone: a ring with a sun-worn N tick, four cardinal points, a needle with the three tips above, in the
   stone's own sockets. A mark `TSM_COMPASS` beside `TSM_SCROLL` in the stones' list (`NTSM`).
2. **The prop** in her room, floating at her right as the map floats at her left (`draw/sprites.c` 1226; `draw/hud.c` 47 for its tag).
3. **The shop pane** card in `draw/tab.c` beside the map's: the price in gold or "YOU ARE $n SHORT", and "BUY THE MAP FIRST" while
   there is no map.

The sheet comes first, on its own, and nothing else is built until it is approved.

## 5. Steps

| | Step | Proof |
|---|---|---|
| 0 | This plan; the sheet (section 4). **Stop for the owner** | the sheet |
| 1 | `RUN.has_compass`, `compass_kind`, `compass_color`; saved in bit 2 of the byte at `save.c` 321 with the map (bit 0) and the lantern (bit 1), with `compass_kind/color` and a ledger note in `docs/SAVES.md`. Every older save loads as "no compass" | an old save loads; the save round-trips |
| 2 | Buying: `BUY_COMPASS` in `her_rooms.c` (live only with a map and no compass), price, say-lines, the prop, the pane, `EVENT compass_bought left n` | harness frames; `HOST_INPUT` buys it |
| 3 | The bearing: `compass_bearing()` from `wisp_target()`; the readout's words | a test level, each objective |
| 4 | The L menu's TRACK; falling back to the exit | scripted input; every run's keys |
| 5 | The HUD stone and dial (`draw/frame.c` `TB_MAP` neighbourhood; touch stones in `crawl_title.c` `tstone_draw`; the pad's layout) | frames in the three input modes |
| 6 | Death: cleared in `death_wake`; the death card lists `A COMPASS` among what is gone only if the owner wants it (an open question, section 6) | `death_wake` log; compass gone, map kept |
| 7 | Proof: `sh tools/tidy_check.sh` for the parts that should move nothing; the dream's logs match the build before on 8 seeds, line for line; `make asan` and a long run | logs, sanitizers |

Touch has no L menu of its own beyond the wisps' stone, so TRACK goes where the wisps' choices already are on touch; to be confirmed
against `controls.c` when step 4 is reached.

## 6. Open, for the owner

- Should the death card (`G.dlost`) list the compass among what is gone? Today it lists gold, pieces, lines, the BFS9000, souls, coins
  and atonement; the map is not on it because it stays.
- 2500g is as asked. Income per level has not been measured against it; step 7 should put the number in `docs/KNOWN-ISSUES.md`
  (how many levels of a typical run it takes to afford, with and without dying).

## 7. Hand-off log

*Step 0 is the plan; no sheet yet, no code. Next: the sheet.*
