# The crypt's small life: things that notice you

**Settled in outline; not begun in code** (15 November 2026, on `claude/gallant-dijkstra-9x79xe`, from main after the mice). The owner
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

- **15 November.** The twelve offered; the owner took them all. This plan written; no code yet. Next: L0, then L1 and the sheet.
