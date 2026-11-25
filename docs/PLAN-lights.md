# PLAN-lights: lights placed by hand, the generator's shown, new hanging lights; the torches at three left

The owner (25 November 2026): "I'd like the ability to place each themed ceiling light, being able to pick from each style. We might
require a few additional gothic styles to our hanging lights." And a new rule: "When a player has only three enemies to eliminate on
the level we cause all torches to pulse a little bit, or shift their color to one in our palette." Asked, they settled:

1. **Pulse and recolour**, both, at three left.
2. **Four new hanging lights** for a sheet: an **iron corona** (a great wheel of wrought iron, spikes and candles, on four chains),
   a **sanctuary lamp** (one lamp of red glass on three fine chains, a small steady glow), a **bone candelabrum** (antlers, ribs and
   skulls bound into a chandelier, candles in the eye sockets), a **pierced cage lantern** (a tall lantern of pierced iron throwing
   dappled light). The sheet goes to the owner first; nothing goes in the game before it is approved.
3. **The generator's lights shown in the editor**, to be moved, removed or replaced: "Show me placements that the generator will use so
   I can move them. Allow me to remove, move, or replace them as I see fit." A drawing that carries lights of its own gets only those.
4. The world editor's rolls of creatures and of gold are rolled over the whole level (docs/PLAN-mapedit.md).

## 1. The format the game reads (the editor's export; built to, not the other way round)

```c
enum { DL_CHANDELIER, DL_LANTERN, DL_CANDLESTAND, DL_BRAZIER, DL_MINELAMP, DL_CRYSTALS, DL_FIREPIT, DL_MAGMA, DL_TOTEM,
       DL_CORONA, DL_SANCTUARY, DL_BONES, DL_CAGE };           /* the last four: the new hanging lights, once their sheet is
                                                                  approved; till then the game lays a chandelier (the three
                                                                  hanging) or a lantern (the cage) in their place */
typedef struct { int style, x, y; } DrawnLight;                 /* its cell in the drawing; hanging ones hang over its middle,
                                                                  standing ones stand in it as the generator's do */
/* Drawn's row, after the hazards: light, nlight. None: the generator's own, as now. Any: those alone, and none of the
   generator's light props (chandelier, lantern, candlestand, brazier, mine lamp, crystals, fire pit, magma, totem). */
```

Each style is laid as `gen_level.c` lays it (`chandelier`, `lantern`, `candlestand`, `brazier`, `minelamp`, `crystals`, `fire_pit`,
`magma`, `totem`): the same sprite, frames, sway and light. The torches on the walls are the events' (docs/PLAN-events.md), not these.

**The generator's lights, listed.** The game lays a drawing as it would (the drawing's seed, its dressing) and lists the light props it
hung or stood: `int crawl_drawn_lights(DrawnLight *out, int max)` after `crawl_drawn_text`, the browser's door `web_drawn_lights` (a
static array: the count, then style, x, y for each, in the drawing's cells), and the harness's `build/host_debug drawnlights <export>`
printing them. The editor loads the game (`play/haunted-core.js`) in its own page to ask, and gives the owner what comes back as
lights of their own, to keep, move or remove.

## 2. Three left

On any level, generated or drawn, when the creatures still to kill (`G.kills_total - G.kills`) fall to three or fewer and above none,
every torch (the walls' `T_FEAT + F1_TORCH` frames, the events' torches, and the light props above) takes a colour from the palette
(cold and pale, chosen on a sheet of frames for the owner) and its light breathes slowly, brighter and dimmer, a little. It is drawing
only: nothing in the simulation, the saves or the dream's numbers may move, and the dream's log must match the build before, line for
line (CLAUDE.md). If a creature's senses read the light (`minds.c`), the pulse is applied where the frame is drawn, not to what they read.

## 3. Steps

| | Step | Who |
|---|---|---|
| 1 | The editor: layers; the rolls; the lights' tools, styles and the generator's lights asked of the game | the editor's track |
| 2 | The sheet: the four hanging lights (and the barred door, relic, socket, lever and plate owed from docs/PLAN-events.md) | an agent; **stop for the owner** |
| 3 | The game: `DrawnLight`, the reader, laying them, the listing door; three left | an agent |
| 4 | The new lights in the game, once the sheet is approved | after step 2 |

## 4. Hand-off log
