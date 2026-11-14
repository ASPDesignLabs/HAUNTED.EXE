# The mice: life in the walls, and what it grows into

**Settled; being built** (14 November 2026, on `claude/gallant-dijkstra-9x79xe`, from main after the tidy-up). The owner gave the
brief, saw the mock-up sheets (`media/sheets/mice/`) and nine questions with a default each, and took every default ("All of your
defaults are good, so go ahead and build it in"). Section 6's steps are being built in order; the hand-off log (section 8) says how
far. Nothing here is tested by hand yet.

*The brief, in a line: mice in every level, harmless, scurrying about the crypt and through holes in its walls, gathering to preen
by warm light, telling one another of you; growing every two levels, sharply on run II's third, until on its fifth they come as an
enemy, the charnel rat, which the small ones run through the walls to fetch.*

## 1. What was asked

The owner's words, cut to their substance.

- "A new initially harmless secret NPC that acts as a background character that does no damage, mice. Small, very small."
- "Mice chirps that modulate as they get larger."
- "They scurry around the Crypts, few in the early levels, becoming more numerous as each run progresses."
- "Their eyes can be seen from a very far distance when facing toward the player, green."
- "They become a little larger every two levels themselves until they make their debut as a full sized enemy toward the middle of
  Run 2, level 5. The growth spurt will be sudden, Run 2 Level 3 should be the sharpest increase to their size until the full size
  NPC."
- "Wall art for the non-combat varieties to enter the walls that act as portals for their AI to quickly get around the map, with a
  minor delay figured for about 10% of the travel time by foot to the location without the use of the portal."
- "Normally they navigate the halls randomly, cannot open doors. When scurried or just at random they will be forced to path to the
  nearest wall portal."
- "Gather and preen themselves near warm sources of light until combat or a player is detected very close to them, giving the
  player many opportunities to witness them scurrying along for the intro."
- "Their AI is unique, they need to 'tell' another of their kind of an enemy. This allows for mice from far off locations accessible
  via their portals to 'tell' their kind of the danger so they can respond. These are flagged to the player."
- "These need to be present in every level, aside from the shop, backrooms, and severance experiences."

## 2. Settled (the owner took every default, 14 November)

| | Question | Settled |
|---|---|---|
| M1 | Sizes | As the sheets: a box of 0.080 of a cell on run I's first two levels, a step of about x1.18 every two levels, x1.73 on run II's third (the spurt), and the charnel rat at 0.62 (x1.94) from run II's fifth. "The sharpest increase until the full size" read as the spurt being the biggest step before the debut, the debut a jump of its own |
| M2 | After the debut | The small ones stay, on every level from then on (runs III and IV, and past 40), as the messengers; the rats come from level 15 on |
| M3 | Hurting them | The harmless ones can't be hurt: spells pass through them, the autopilot never aims at them, a blast near them only scatters them. From level 15 a messenger dies to one hit, and is not a kill |
| M4 | Where | Every level but her rooms, the Backrooms and Severance. A few on the tutorial's levels 1 and 2, away from the lessons; on a boss level a handful that flee when the fight begins |
| M5 | Secret | No caption, no bestiary page and nothing on the map until level 15. Then the page comes, with their growth drawn on it |
| M6 | The green | The brighter one (B on `dark.png`) |
| M7 | The record | The rat takes the record's 24th and last place (`REC_KINDS`); the kind after it will need the record widened, a new save version |
| M8 | Holes | The small ones' alone: the rats walk |
| M9 | The flag | A mark over the messenger, and its chirp |

## 3. The sizes

The box is the sprite's 64 pixels in cells, as `KINDS[].scale` is; each is drawn to fill its box, as the spiderlings are.

| Levels | Box | Look |
|---|---|---|
| 1-2 | 0.080 | a house mouse: grey-brown, big ears |
| 3-4 | 0.095 | |
| 5-6 | 0.110 | leaner |
| 7-8 | 0.130 | the snout longer, the incisors showing |
| 9-10 | 0.155 | rat-shaped |
| 11-12 | 0.185 | darker, brown, wet; an ear notched |
| 13-14 | 0.320 | the spurt: hunched, the spine and ribs showing, bald patches |
| 15 on | 0.185 small; the rat 0.62 | the small ones as on 11-12, now the messengers; the charnel rat low and long, a hound's length |

## 4. How they behave

**Not creatures.** The small ones are a list of their own (`Mouse`, `G.mice`), not `Mob`s: outside `MAXMOBS`, the hunt, the encounter
budget, the tally, the map and the autopilot's aim. They draw on their own dice (`G.mrs`, seeded from the level), never `G.rs`, so
before level 15 they move nothing of the simulation: the dream's log matches the build before line for line, their own lines aside.

**Holes** (`MHole`, `G.mholes`): four on a run's first level to eight on its ninth (three on a drawn level, four on a boss's), on a
plain stretch of the run's own wall facing an open floor cell, at least six steps apart. The wall's face becomes the hole baked on
its material (`T_MHOLE`: each material's first look with the hole at that level's size). Any hole leads to any other: going in, a
mouse is nowhere for a tenth of the time it would take on foot at the pace it went in at, doors counted as open (`mh_dist`); its eyes
show in the far hole for a moment before it comes out.

**Their days.** Each wanders: a cell or a few away, a pause to sniff, on (`MO_WALK`, `MO_IDLE`); they can't open doors. Now and then
one goes to the nearest hole and comes out of another. A warm light within twelve steps (a torch, candles, a fire pit, a brazier, a
chandelier, a lantern, a mine lamp, magma; not a crystal or a geode) draws them to sit by it and preen for a while (`MO_PREEN`).

**Scurried.** You within two and a half cells, or within four in sight and moving; a noise the dead would hear (a cast, a blast, a
door, a death) within its reach; a blast within its radius and two: it runs for its nearest hole (`MO_FLEE`), and comes out
elsewhere.

**The tell.** The first of them to be scurried, if any calm one is out, carries word (`MO_RUN`, the mark over it): to the nearest calm
one by foot or through the holes, whichever is quicker; they sit up nose to nose and chirp (`MO_TELL`, a second and a bit), and the
one told carries it on. Before level 15 the end of it is the level's mice going to ground for half a minute or so. From level 15 a
messenger goes first to the nearest calm charnel rat: told, the rat rears and squeals and goes hunting to where you were seen
(`mind_track`). Two messengers at most at once; a messenger gives up after 25 seconds. From level 15 one hit kills it, and the word
dies with it, as a caller's alarm does.

**On a boss level** they are out until the fight begins, then all go to ground for good. On a drawn level there are three, kept off
the lessons' trigger cells.

## 5. The charnel rat (`MOB_RAT`, the 24th kind)

"CHARNEL RAT", from level 15: a melee biter that roams (`RL_ROAM`, as bats and hunting spiders do), no doors, not undead. Health 16
(a bone hound 14), pace 1.8 (a hound 2.0), box 0.62, rank family `PF_HIDE` (`R_WOOD`, as the hunting spider's), weak to fire (x1.5),
hardy to poison (x0.5); its bite is physical (`FA_RBITE`): in low, a bite, a scuttle back. Eyes green (`EC_GREEN`). Its budget cost
0.9, its experience 10; it may be gilded. Its own sounds: a hiss as it notices you, a squeal as it is told, bites and dies. On level 15
one comes, alone, as every newcomer does; from 16 one or two roam every level (three from 21), on the game's own dice, so that the
mice have someone to fetch (the set pieces spend most of a level's budget, and as a wanderer alone it was rare: three layouts of
level 17 had none).

**What it does not have yet.** Her hall has twenty-two places for portraits and all are taken, so the rat has no portrait and no feat
(the feats are the portraits' medallions); the portraits' order and gold leave it out, so every other kind's stay as they were.
Where it hangs is the owner's to choose.

## 6. Steps

1. The art: the small ones' six sprites made for the level's stage in `crawl_art_level` (`SP_MOUSE_*`), the rat's (`SP_RAT_*`:
   walking towards you, its side, its back, the bite, rearing, dead), the holes on each material (`T_MHOLE`), the green in the
   palette's last entry (`C_CLEAR`'s, which is never drawn). `make sheets`' `mice`.
2. The rat wired into every kind table.
3. The mice: holes, wandering, warmth, scurrying, holes as portals, the tell, the boss level and the drawn levels.
4. Drawn in play: their sprites by pose and view, the eye pass, eyes in a hole, the mark; the chirps, the rat's sounds.
5. The bestiary's page and its growth, held back until the rat is met.
6. Tests: the proof that levels 1-14 move nothing; smoke; the sanitizers' long runs; the dream from level 15 against the build
   before on eight seeds and more.
7. The write-up: this log, `CLAUDE.md`, `docs/catacomb/creatures.md`, `docs/HARNESS.md`, `docs/KNOWN-ISSUES.md`.

## 7. Debug switches

`CRAWL_MICE=0` none; `CRAWL_MICESTAGE=n` the small ones at stage n whatever the level; `CRAWL_TPMICE=1` stood before the nearest
gathering when you take over; `CRAWL_MICEDBG=1` their states on stderr. `EVENT mice` (the level's count and holes), `EVENT
mouse_tell`, `mouse_told_rat`, `mouse_slain`, `mice_hide`.

## 8. Hand-off log

- **14 November.** The brief, the sheets, the questions; the owner took the defaults. This plan written.
