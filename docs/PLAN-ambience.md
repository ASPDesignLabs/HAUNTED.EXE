# The crypt's small life: things that notice you

**Settled in outline; the core, the art, the floor and the air built (L0 to L3)** (15 November 2026, on `claude/gallant-dijkstra-9x79xe`, from main after the mice). The owner
asked for "world building elements ... that react to the player's presence and do not require the player to do much of anything.
Stuff that brings life to these dead, damp, and dark crypts", was offered twelve, and took them all: "every single suggestion sounds
great". Nothing here is built or tested by hand.

*The brief, in a line: the crypt notices you. Moths, bats, beetles, toads, drips, painted eyes and old bones answer your passing, and
ask nothing of you, hurt nothing, and are never in your way.*

## 1. The twelve

| | What | Reacts to you by | Leans on |
|---|---|---|---|
| A1 | **Portraits that watch**: saints and banners turn their painted eyes after you, a pixel or two at a time | your bearing | the pictures found per face (`wf_pic`) |
| A2 | **Bats** roosting in high corners; a few drop, circle the room once and resettle elsewhere | coming near | the ceiling's corners; the mice's way of choosing cells |
| A3 | **Eyes in the niches**: pairs blinking in empty dark; shut and withdraw when your lantern finds them | your light and bearing | the Eyes rule (bright from anywhere); `eyes_put` |
| A4 | **Hanging things** (chains, censers, lamps) swing a little after you pass | passing | wall faces; a swing's decay |
| A5 | **Moths and flies** round torches and lanterns; scatter as you pass, drift back | passing | `mo_warm`, the mice's warm lights |
| A6 | **Flames that lean**: torches gutter and flatten as you pass, harder when a door opens near | passing, doors | the torch's three frames |
| A7 | **Beetles and woodlice** scuttle from your lantern into cracks; step on one and it crunches | light, feet | the mice's holes and floor-walk |
| A8 | **Drips and puddles**: water falls into dark pools, rings spread, your steps leave wet prints that dry | feet | floor decals |
| A9 | **Toads** in the dampest levels, hopping off with a croak | coming near | the pools of A8 |
| A10 | **Webs that shiver**: sway as you pass; walk through one and it tears; a spider comes to mend it later | passing | `F2_WEB`, the spiders' marks |
| A11 | **Glowing fungus**: luminous moss pulses brighter as you near, spores drift up when you pass | nearness | the lightmap |
| A12 | **Sleepers**: remains in niches settle as you pass (a jaw, a tick of dry bone, a curled finger). They never get up | passing | the niche pictures |

## 2. Rules for all of them

- **Not creatures.** The record is full (`REC_KINDS`, 24), so none is a `MOB_*`: each is a list of its own beside `G.mice` and
  `G.skit`, outside `MAXMOBS`, the hunt, the budget, the tally, the map and the autopilot's aim. Nothing is saved; none can be hurt,
  none blocks, none is counted, none hurts.
- **Dice of their own** (`G.lrs`, seeded from the level as `G.mrs` is, `G.rs` read and never drawn on), so levels 1 to 10 must log as
  the build before, line for line. Proved as the mice were.
- **Eyes.** Anything with eyes shows them bright from anywhere, in any light and fog (the A2 bats', A3's niches', the toads').
- **Calm switches** (the owner is autistic and dogfoods these; motion and noise that cannot be turned down is a bug): two settings,
  *small life: movement* and *small life: sound*, each on / reduced / off, in the settings dialog and the browser's menu, kept with
  the other settings (not the save). Off draws and sounds exactly the build before. Reduced keeps the still things (the puddles, the
  fungus's glow, the hanging things at rest) and drops the flitting and the croaking.
- **Never where it hurts play.** None within the lessons' triggers (the mice's `mo_cell_ok`), none in her rooms, the Backrooms,
  Severance or the shop; fewer on boss levels, and none in a fight's arena.
- **New art goes to the owner as a sheet first** (`media/sheets/ambience/`), then the game.
- **Backups.** Nothing here touches the save; the reminder stands (`reg export "HKCU\Software\HalloweenVGASaver" ...`).

## 3. Steps

1. **L0 the core**: the list, `G.lrs`, `life_update` / `life_draw` beside the mice's, the settings, `CRAWL_LIFE=0`, `CRAWL_TPLIFE`.
2. **L1 the sheet**: every new sprite and decal drawn (moth, fly, bat roosting and flying, beetle, woodlouse, toad, drip, ring, wet
   print, fungus, spore, niche eyes, sleeper's bones); `make sheets`; to the owner before any is used.
3. **L2 the floor**: A8, A7, A9.
4. **L3 the air**: A5, A6, A2.
5. **L4 the stone**: A1, A3, A4, A12.
6. **L5 the damp**: A10, A11.
7. **L6 the proof**: levels 1-10 as the build before; frames unchanged with the life off; eight seeds of balance (a longer view must
   not slow the frame: the smoke's 2 ms a frame); the sanitizers; CI. Then main, at the owner's word.

## 4. Hand-off log

- **15 November, L0.** The core built: `src/crawl/game/life.c` (the `Life` list, `G.lrs`, `life_level`, `life_update`, the
  settings' gates), the two calm settings (`AppConfig.life_move`, `life_sound`; the registry's `LifeMove`, `LifeSound`; two
  drop-downs in the Windows dialog; the browser has no menu for them yet and runs both on), `CRAWL_LIFE=0`. No kind is made yet.
  Proved to move nothing: the dream on seed 3 at levels 3 and 12 logs as the commit before, but for its timing line. The Windows
  build is compiled only by CI (no mingw here).
- **15 November, L1.** The art drawn (`src/art/parts/life.c`, 29 sprites, `SP_MOTH_A` to `SP_SLHAND_B` before `NSPR`; the bat and
  fungus carry `LF` in their names since a roosting bat and a fungus already stood in the props) and its sheet made
  (`build/sheet life <png>`, kept as `media/sheets/ambience/life.png`). Sent to the owner; none of it is drawn in the game yet, and
  none will be till they have passed the sheet.
- **15 November, L1 again.** The owner: the art "seems a bit basic with too much of a focus on simple shapes". Redrawn from each
  creature's anatomy with the surface worked over (`life.c`'s `lf_*`: a key light laid over the whole last, `lf_relight`; grain,
  fur and scale in strokes; a wing painted by its distance from its root, with veins and bands; the bat's arm, wrist and four
  fanned fingers with a scalloped membrane; the beetle's grooved elytra and spined legs; the toad's glands and warts; a skull with
  sutures and a crack, a hand of every small bone with a tarnished ring), nothing quite alike on its two sides. The sheet made again.
- **15 November, L2, the floor** (harness only). Built in `src/crawl/game/life.c` and drawn in `draw/sprites.c` (after the wall spiders):
  A8 puddles that drip (a drop forms above one, falls, rings spread and fade; wet boots leave a print each half-cell for eight, paler
  each, gone in 14 s), A7 beetles and woodlice that wander, stop, run from you inside two cells (a cell and a half the louse) and
  slip into the stone, or are crunched if stepped on and come back elsewhere after a long while, A9 toads by the puddles from level 6
  that hop off, three at the most, with a croak. Counts by the run's step (3 to 8 puddles, 5 to 12 beetles, 3 to 8 lice), two,
  three and two on boss and drawn levels, no toads there. Reduced movement keeps the puddles and the prints and the sat toads;
  off shows none. The water and prints were redrawn lying low on the box's floor. Proved: dream logs on levels 1, 3, 5, 8, 10 and 13
  of seeds 3 and 5, 600 s each, line for line the commit before's; frames with `CRAWL_LIFE=0` byte for byte (9 of 9); ASan and
  UBSan clean on two 900 s dreams and a staged run; the smoke at 1.53 ms a frame. Not heard by anyone: the drip is `SFX_DRIP`, the
  crunch `SFX_CHEW` high, the croak `SFX_SQUEAK` low, borrowed till the owner says they will do. Left: the owner's eye on them in
  play, and A5 to A12.
- **15 November, L3, the air** (harness only). A5: moths (and, where `crawl_decay` is over 0.35, flies) orbit each warm light, ten lights
  at the most, one to three moths a light; they scatter into the dark when you come within a cell and a half (or two and a half
  and moving), wait, and return. A6: each torch's wall cell guttering when you pass within two and a third cells moving, and when a
  door within five cells swings, harder: its flame held to the two leaning frames, alternating quickly, for 0.9 s or 1.3 s, then
  five seconds' rest (`torch_frame`, read by `draw/walls.c`; nothing of the light itself changes, since what the dead see of a
  torch is in the simulation). A2: bats, on levels 1 to 10 only (from 11 a roost of the dead's own is the warning, and a harmless
  one would blur it), two to four roosts by walls, one to three bats in each; within two and a half cells some drop, circle for four to
  seven seconds, never come at you, and settle in another roost in sight. Their eyes pale, not the real bats' ember. Reduced
  movement draws none of these. Proved as L2 was: 12 dream logs (levels 1, 3, 5, 8, 10, 13, seeds 3 and 5, 600 s) line for line the
  commit before's but for a timing summary; frames with `CRAWL_LIFE=0` 9 of 9; ASan and UBSan clean on three 900 s dreams and
  three staged runs; the smoke 1.6 ms a frame. Left: A1, A3, A4, A12 (the stone), A10, A11 (the damp), the owner's eye.
- **15 November.** The twelve offered; the owner took them all. This plan written; no code yet. Next: L0, then L1 and the sheet.
