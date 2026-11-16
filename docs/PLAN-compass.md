# The compass: a needle for what you have asked to find

**Built and released as 1.0.0b21.9, "A Needle of Bone", harness only** (16 November 2026, on `claude/stoic-ramanujan-3iqb3m`, from main after 1.0.0b21.7, and released on top of 1.0.0b21.8, "The Antechamber", which another session put out first). The owner asked for
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

- **A dial on the HUD** (a plate at the top right of the view; on touch, a tablet under MAP, blank until bought as the map's stone is). It turns as you turn so that its **N** tick is
  true north; a needle in the objective's colour leans toward the thing tracked. Under it, a short readout with the distance in
  cells and the quarter: `EXIT 34 NNE`. It is a bearing, never a route: it may point through a wall, and the way round is yours to
  find. That is what keeps it from replacing a wisp.
- **The objectives are the wisps' own.** `wisp_choices()` already lists them and `wisp_target()` already says where each is, past
  the keys an exit's vault needs first. The compass reads the same list: the exit always; each key lying, with Keymaker; the altar's
  secret while its wisp is out. A new wisp kind gets a needle with no more work, and gear and skills gate it for free.
- **Choosing.** In the L menu each row gains a second action, *TRACK* (the Back key: see step 4), beside casting. Tracking is free and costs no soul; the choice is
  kept as `RUN.wisp_kind` is, but separately (`RUN.compass_kind`, `RUN.compass_color`) so that "L always casts this" is untouched.
  With nothing tracked it tracks the exit. If the thing tracked is gone (the key taken, the secret found) it falls back to the exit
  and says so once.
- **Nothing to point at.** Inside a pocket the exit is the way back (as the exit wisp's). In her room, in the Backrooms and in a boss's
  hall where `wisp_target()` finds nothing, the needle rests and the readout says `NOTHING TO FIND`.

## 3. For whoever plays it (the owner dogfoods this)

- The needle is not read by colour alone: each objective has its own tip (the exit a wisp's flame, a key its bit, the altar an eye) as well as
  its ramp, and the readout says the same in words.
- The dial turns smoothly but **never jitters or spins for show**: it eases to the bearing, and when the bearing is a wall's length off
  it does not hunt. With the small-life movement setting on *reduced* or *off* (docs/PLAN-ambience.md) the dial still turns, since it
  is information, but the needle does not swing past and settle.
- It is on the HUD where the eye already goes; it is not drawn on the map overlay, which stays as calm as it is.
- The sound of buying it is the map's; the compass makes none of its own in play.

## 4. Art

New art goes to the owner as a sheet before it goes in the game (`make sheets`). Three pieces, in the carved stone of docs/PLAN-ui.md,
cut with the `sc_*` kit:

1. **The dial** for the HUD's stone: a disc of old stone, chipped, cracked and mossed (they turn with it), bone studs set in its rim for the winds and a gilt one for north, a needle of bone whose tip carries the colour (the exit a wisp's flame, a key its bit, the altar an eye), a skull for its pin. The first sheet drew a clean modern dial, which did not belong; the second pass is this. A mark `TSM_COMPASS` beside `TSM_SCROLL` in the stones' list (`NTSM`).
2. **The prop** in her room, floating at her right as the map floats at her left (`draw/sprites.c` 1226; `draw/hud.c` 47 for its tag).
3. **The shop pane** card in `draw/tab.c` beside the map's: the price in gold or "YOU ARE $n SHORT", and "BUY THE MAP FIRST" while
   there is no map.

The sheet comes first, on its own, and nothing else is built until it is approved.

## 5. Steps

| | Step | Proof |
|---|---|---|
| 0 | This plan; the sheet (section 4). **Stop for the owner** | the sheet |
| 1 | **Done.** `RUN.has_compass`, `compass_kind`, `compass_color`, saved in the byte after the wisp's three in the page's fixed part (bit 0 held, 1-2 the wisp's kind, 3-4 its key): not in `LG_CASTER`'s flags, which are the ledger's kept-through-death state, so what a death clears is written with the page. That byte was always written 0 and never read, so no new version is needed; an older page reads as none. `docs/SAVES.md` and `docs/HARNESS.md` say so | `CRAWL_SAVETEST=1`: 11 cases, 0 bad; the dream's log matches the build before on seeds 3 and 5 for 1200 s, line for line |
| 2 | **Done, harness only.** `BUY_COMPASS` in `her_rooms.c` (`COMPASS_FOR_SALE()`: a map, no compass), `COMPASS_PRICE` 2500 and its place at her right (`COMPASS_X/Y`, the map's mirror across her) in `wisps.c`; `SP_COMPASSWARE` (the sprite, `parts/hall.c`, drawn as the sheet's dial with a bone loop above it); the price over it (`hud.c`), its prompt, `compass_look`, the wares page (`tab.c`, the sheet's words; A/D now go round the pedestals, the map and the compass); `compass_buy`, `EVENT compass_bought left n`; `CRAWL_TPCOMPASS=1`. Bought, it leans toward the way out. The page has no dial yet (step 5) | frames `media/sheets/compass/step2_room.png`, `step2_page.png`; bought at 2500 with 3000 held (left 0 by the harness's gold), refused with 100 ("YOU ARE $2400 SHORT"), not shown without the map; the dream's log matches the build before on seeds 3 and 5 for 1200 s; the save test still 11 cases, 0 bad |
| 3 | **Done, harness only.** `compass_read()` (`wisps.c`) from `wisp_target()`: the bearing (degrees clockwise from north, which is up the map, y falling), the distance in cells, and what the needle shows (an exit behind a vault shows that vault's key, as the exit wisp goes); `compass_words()` the two lines under the dial (`EXIT`, `GOLD KEY`, `ALTAR`; `34 NNE`, `HERE` within a cell and a half); `compass_tick()` each frame: the needle's own angle eased (`G.compass_bear`, 8 a second, shortest way round), and a thing gone falls back to the exit with `THE NEEDLE TURNS BACK TO THE WAY OUT`. In her room, dead, or with nothing to lean toward, it rests. Distance is straight, not the route | `CRAWL_COMPASSTEST=1`: 16 cases on level 1, 32 on 5, 12 and 23, 0 bad; `CRAWL_COMPASSLOG=1` on level 2 reads `GOLD KEY` for an exit in a gold vault; the dream's log matches the build before on seeds 3, 5 and 7 for 1200 s |
| 4 | **Done, harness only.** The L menu's TRACK is the **Back key** (`update_wispmenu`, `compass_track`): Backspace on a keyboard, **X** on a pad (`controls.c`: X in a menu sent Back only with the augments up) and the left stone, BACK, on touch, which already sent it: no new key on any of them. It is free, casts no wisp and leaves `RUN.wisp_kind` and *L always casts this* alone. The row the needle leans toward wears a small dial; a line `BACK AIMS THE COMPASS` shows once there is a compass (and the plate grows 11 px). Keys and the altar's secret appear as rows as they do for the wisps, so Keymaker gates the keys. A thing gone falls back to the exit (step 3). `EVENT compass_track kind c colour c` | frames `media/sheets/compass/step4_menu.png`; scripted input tracks a key and then the exit and the marker follows; `CRAWL_COMPASSTEST=1` now also tries a key lying nowhere (18 cases on level 1, 17 on 4, 32 on 12, 0 bad); the dream's log matches the build before on seeds 3, 5 and 7 for 1200 s |
| 5 | **Done, harness only (touch by the harness's stage, not by a finger).** `ui_compass` (`crawl_ui.c`, the sheet's dial moved into the game; `uisheet compass` draws with it); the **HUD plate** (`render_compass`, `hud.c`) at the **top right of the view, below the news**, not the sheet's bottom right, where the spell and rune sockets and a carried thing already stand: the dial, what it leans toward and how far, its width what the words need; **touch's tablet** `TB_COMPASS` under MAP in the left column (`controls.c` `touch_geo`, `frame.c`; the left strip is that much shorter), worn blank till bought, `COMPASS` or `NORTH` or `N` by its width, the dial in its socket, a tap opens the L list (`TCH.compass_tap`, handled with the wisps' keys in `lessons.c`), hidden in menus; the in-picture plate gives way when the tablet is up; the **wares page** has its dial. The pad needs nothing more: it sees the plate, and X in the L menu is Back (step 4). `HOST_STAGE=WxH` for the harness | frames `media/sheets/compass/step5_*.png`: the plate reading `GOLD KEY / 36 S` and turning with the view, touch's stage at 780x360 and 700x320 with the tablet live and worn blank, the wares page; the dream's log matches the build before on seeds 3, 5 and 7 for 1200 s. **Not run:** a finger on the tablet, the Windows build, the browser build (CI compiles them) |
| 6 | **Done, harness only.** Cleared in `death_fall` (`ledger.c`) before `death_write`, so a death page does not keep it; **not** in `death_wake` as first planned: that runs only when a death costs something, and the deaths that cost nothing (the dream, a pocket, the lessons, a code used) restart the level and never reach her room, so they keep it, which is the owner's rule (*lost on a death that reloads into the Item Shop*). The death card names it, **last** (`..., 1 LINE, A COMPASS GONE WITH YOU`): its line is 64 characters over two, so what it already names comes first and the compass is the first dropped from a full line. That settles section 6's first question as a default, not the owner's word. the harness's `CRAWL_KILLAT=t` (a person struck down at that time: a death that costs) makes it | `step6_card.png`: `$416, 7 PIECES, 1 LINE, A COMPASS GONE WITH YOU`; the page the death wrote loads with no compass (`EVENT run_loaded compass 0`), a control with one loads with it; after the wake: gold $0, level 1, the compass back on sale at $2500, the map still hers (`step6_after.png`); the dream's log matches the build before on seeds 3, 5 and 7 for 1200 s; the save test and the bearing test still pass |
| 7 | **Done, harness only.** The dream's logs match the build before (b4baffc, whose game code is main's) on **8 seeds (1, 3, 5, 7, 9, 11, 13, 15) for 3,000 s each, line for line**; `make asan` clean (UBSan too, leaks off as the notes say) on both selftests, a dream with a compass for 1,500 s, and a person's run (L list, TRACK, touch's stage, `CRAWL_KILLAT=16`, the costly death, the wake, the vow); the balance note in `docs/KNOWN-ISSUES.md`. `tools/tidy_check.sh` was **not** run: it proves that code moved or cut changed nothing by comparing objects byte for byte, and this change adds code, so every object it touches differs by design; the dream's logs are the proof that stands in its place | logs, sanitizers, the note |

Touch has no L menu of its own beyond the wisps' stone, so TRACK goes where the wisps' choices already are on touch; to be confirmed
against `controls.c` when step 4 is reached.

## 6. Open, for the owner

- ~~Should the death card (`G.dlost`) list the compass among what is gone?~~ Done as a default, last in its line (step 6); the owner may say otherwise. Was: Should the death card (`G.dlost`) list the compass among what is gone? Today it lists gold, pieces, lines, the BFS9000, souls, coins
  and atonement; the map is not on it because it stays.
- 2500g is as asked. Income per level has not been measured against it; step 7 should put the number in `docs/KNOWN-ISSUES.md`
  (how many levels of a typical run it takes to afford, with and without dying).

## 7. Hand-off log

*Step 7 done, harness only (16 November 2026): the proof, see the table. Everything in the plan is built; nothing has been played by hand. Left for the owner: play it (the plate's place and size, the tablet on a phone, Backspace for aiming, 2500), say whether the death card should name it, and the word for a release (the README's "What has been verified", the page, the release notes and the version are not touched: they describe the released build only).*

*Step 6 done, harness only (16 November 2026): death, see the table. Next: step 7, the proof (the dream on eight seeds against the build before, `make asan` and the long runs, `tools/tidy_check.sh`, the balance note in `docs/KNOWN-ISSUES.md`, the README's and the page's words when it is released).*

*Step 5 done, harness only (16 November 2026): the dial on the HUD, on touch's tablet and on her page. Next: step 6, death.*

*Step 4 done, harness only (16 November 2026): TRACK, see the table. Next: step 5, the HUD dial, the touch tablet and the pad's layout (the first thing a person will see of it).*

*Step 3 done, harness only (16 November 2026): the bearing, see the table. Nothing draws it yet (step 5); step 4 is the L menu's TRACK, which is what lets it lean toward anything but the exit. The wares page lost its last sentence ("THE MAP DOES NOT GO") at the owner's word. Next: step 4.*

*Step 2 done, harness only (16 November 2026): buying, see the table. Bought, it does nothing yet: its needle, the L menu's TRACK and its death come in steps 3 to 6. Next: step 3, the bearing.*

*Step 1 done (16 November 2026): the state and the save, see the table. Debug switches `CRAWL_HASCOMPASS=1`, `CRAWL_SAVETEST=1`. Next: step 2, buying.*

*Step 0: the plan, and the sheet (16 November 2026): `build/uisheet compass <out.png>`, the dial drawn in the mock-up (`dial()` in `tools/uisheet.c`, to move into `crawl_ui.c` once approved; `build/uisheet` now links `crawl_title.c` for the touch tablet). Redrawn once (the first dial looked like a watch; now a relic). Kept as `media/sheets/compass/compass.png`: the HUD plate, the dial turning, the three tips and rest, the touch tablet, the prop, her wares' page. Waiting on the owner's look; nothing in the game has changed.*
