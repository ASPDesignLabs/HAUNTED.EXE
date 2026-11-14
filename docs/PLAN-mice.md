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

`CRAWL_MICE=0` none (the wall spiders too); `CRAWL_MICESTAGE=n` the small ones at stage n whatever the level; `CRAWL_TPMICE=1`
stood before the nearest gathering when you take over (2: before a hole, a mouse looking out); `CRAWL_SKIT=0` no wall spiders;
`CRAWL_TPSKIT=1` stood two cells from one on its wall (`CRAWL_SKITGO=1`: it crawls off at once); `CRAWL_TPSKIT=2` one put a
little short of a wall's edge and crawling slowly round it and back, you where both faces show, `3` the same into a corner (that
one minds nothing, and the controls card is put away); `CRAWL_TPSKIT=4` one on the face before a picture with bare stone either
side, crawling at the picture's own height to two faces past it and back, you two cells off; `5` the same before three pictures
together; `CRAWL_PICS=1` lists every picture and its keeping each time the stone is traced. `EVENT mice` (the level's count and holes), `EVENT mouse_tell`,
`mouse_told_rat`, `mouse_slain`, `mouse_gaveup` (a messenger giving its word up after 25 s), `mouse_stall` (two seconds on one step
of its way, stepped on: it should never come), `mice_hide`; `EVENT skitters`, `skitter_run`, `skitter_told`, `skitter_web`,
`skitter_slain`, `skitter_ways` (the loops traced again), `skitter_jump` (one moved more than 0.3 of a cell in a frame: it should
never come), `skitter_ways_broken` (a loop that did not close: it should never come), `skitter_onpic` (one within half its
legs' reach of a picture: it should never come), `skitter_noway` (a mark it has no way to); `EVENT skitters` counts the faces and
the pictures.

## 8. The wall spiders

The owner's, after playing the mice (14 November): "spiders that trawl along the wall in much the same way as the mice, acting as a
background character for a bit that can alert other spiders to your presence. They run to the nearest web spider or hunting spider
if present. If not they wander around until they find a web in a corner. This doesn't need to be a new kind, these are purely for
world building and set dressing." Defaults taken, each easy to change:

- **Not a kind.** `Skitter` in `G.skit` (`MAXSKIT` 10), on dice of their own (`G.srs`). Drawn from the spiderling's own pieces: on
  the stone its back (`SP_SKIT_A`, `SP_SKIT_B`, section 8.2, `SK_SIZE` 0.17), slain its front turned over (`SP_LING_A`, `SK_SCALE`
  0.14); her red mark and its iron turned to the walls' dust by a table (`sk_lut`), its eyes through the eye pass in ember.
- **Where.** Every level the mice are on: two on a run's first levels to five on its ninth, two more in the Catacombs; two on a drawn
  level and on a boss's.
- **Their days.** They never leave the stone (section 8.1). They crawl a few faces on round it (`sk_wander`), up and down it (a tenth
  of the wall's height to nine tenths), and rest.
- **Scurried** by you within two cells (three and a half in sight and moving), a noise within half its reach, a blast, a fight within
  three cells: to the nearest calm web or hunting spider within 36 cells' crawl round the stone (`sk_find_spider`), carrying word
  (the mark, in ember). Told (`sk_tell`): a hunting spider goes where you were seen (`mind_track`), as a told rat does; a web spider,
  which cannot leave its web, watches 30 seconds (`Mob.told_t`: its eyes stop blinking) and its web thrums, and if it is a nest's
  (from level 15) it calls the nest, as its tripwire does. A web spider is told from the corner behind its web; a hunting spider from
  the wall nearest it, once within 2.2 cells and in sight. With no spider to tell it makes for the nearest corner web (`SP_COBWEB`)
  within 40 cells' crawl, or wanders until one is within six (giving up after 25 seconds), and sits in the corner behind it 30 to 60
  seconds.
- **Killed** only while carrying word, by one hit, and the word dies with it; not a kill. It drops off the stone to the floor.
- **On a boss level** they go into the nearest web when the fight begins, for good.
- **The simulation.** A web spider told only shows it, so before level 11 (the hunting spider's first) nothing they do reaches it.

### 8.1 The stone's ways

The owner, after playing them (14 November): "Spiders we are using for this are not actually sticking to the walls. We might need to
develop a pathing system for them to do this. They kind of float and fly around, which looks a bit odd." They had walked the floor's
ways from one cell's middle to the next and were only drawn pushed toward a wall beside the cell. Where the wall changed side or
stopped (a corner, an opening, a doorway) the drawing slid across open air to the other wall, or sank to the middle of the floor.

Now every face where open floor meets a wall (`K_WALL`) is traced, with the wall on the left, into closed loops (`wf_build`, `WF`):
one round each room's and corridor's edge, out through every doorway along its jambs (a spider fits the crack a door leaves), and
one round each block standing free. A face is its open cell and the side its wall is on, and its joints with the faces either side
go straight on, into a corner (the same cell's next face) or round the wall's edge (onto the wall's other side). A spider's place is
its face, how far along it and how high, held `WF_IN` (0.02 of a cell) off the stone. Its way stops that short of an inner corner and
goes that far past an outer one, so the way is unbroken all round. A spider goes only along its loop, the shorter way round
(`wf_gap`), and up and down the stone, in a straight line in those two (`sk_crawl`), so it cannot leave the stone. They are put only
on loops of twelve cells or more, since one on a block standing free could only go round it. Whom a spider is told from, and webs,
are found through the open floor to the nearest place on the stone (`wf_near`). When a wall moves (a secret's block), the loops are
traced again and each spider put back on its face or the nearest (`skit_rebuild`).

### 8.2 Painted on the stone

A picture standing 0.04 off a wall stands out from it as soon as the wall turns away, and its far half goes into the stone. So a
wall spider is drawn as the walls are (`skit_decal`): each column's ray is met with its face's plane, and the picture's texel is
found by how far along the stone and how high, turned to its heading. It narrows as the wall turns away, and what of it reaches past
a joint is drawn on the next face, folded into the corner or round the wall's edge. Its eyes are put where they fall on the stone and
drawn by the eye pass's own code (`eyes_put`, cut from `render_eyes`, which keeps the billboards' part). The picture is new: the
spiderling from above, its own legs, shell, mark and eyes (`spr_skit`, in `parts/spiders.c`). It is the last row of `make sheets`'
`mice`, and is kept with frames of one going round each kind of corner as `media/sheets/mice/wallspiders.png`. It went in
before the owner had seen it, since the fix needed it. The owner's word on it: "The legs for it are not accurately placed." They had
all come from one point in the middle of the shell, like a star, the back pair out from under the abdomen, all one length and
bunched in forks, the eyes too big and mid-shell. Redrawn as a spider is made: the legs from under the sides of the shell, front to
back, the first two pairs forward, the third out and back, the fourth back along the abdomen, the first and fourth longest and the
third shortest; each a thigh, a shin and a finer foot, banded at the knee and the next joint, curling forward in front and back
behind; a pear of a shell with a waist, the eyes a pair with a seam between at its front edge and the small ones behind, the palps
and fangs before them, chevrons down the abdomen behind the mark. The sheet was made again from it.

### 8.3 Round the pictures; past the doors

The owner, 14 November: "Spiders can slide under/over doors, open or closed. They should navigate around pictures."

- **Doors.** Settled as built: the stone's ways run through every doorway along its jambs, so a spider goes past a door open or shut.
- **Pictures** (`wf_pic`, `wf_pictures` when the ways are traced). Each face's picture is measured from its own texels: what stands
  on its stone (`fover`: a niche, a saint, a banner, a frame, a torch and its three flames), a mice's hole at its foot (the hole
  against the stone it was cut in), or a wall that is a thing of its own whole (the way out, a vault's door, a hazard's alcove, an
  unfaced niche or torch). Its width is taken either way round, since a face may be drawn mirrored. A painted web (run II's
  `F2_WEB`) is not one: they are spiders. A spider keeps `PIC_MU` (0.06) along the stone and `PIC_MH` (0.085) up it clear of a
  picture: its legs' reach. A door (`door_tex`) leaves its top row free, so they go over doors; any other picture that leaves no
  way past (a floor-to-lintel niche, a saint in her alcove) is not passed.
- **Finding the way** (`sk_plan`). Each loop's stone is a grid (`wf_occ`): 18 rows from 0.06 of the wall's height to 0.94, and
  columns as near 0.05 wide as divide the loop evenly, so it closes round; a cell is taken if its middle is within a picture's
  keeping. A spider's way to its mark is searched out from its cell through the free cells (`sk_flood`: eight ways, no corner cut,
  up to 45 cells' crawl either way round), then pulled straight from turn to turn as far as each can be seen from the last
  (`sk_los`). Its turns are kept as waypoints (`wps`, `wph`, twelve, planned again when they run out) and it crawls from one to
  the next (`sk_crawl`). So it goes under a torch, over a low tablet, down the gap between two pictures and along under the next,
  or back the other way round the room when the pictures leave no way. A mark on a picture is moved to the nearest clear height
  there (`sk_free_h`); one with no way to it is not made for (`EVENT skitter_noway`), and a spider scurried to no end is not
  scurried again for 1.5 seconds, nor looks for a web in a boss's fight more than every two.
- **Wandering** (`sk_wander`) is to somewhere it can reach: a cell the search found within a few cells either way round, at least
  0.6 of a cell along or four rows up or down from where it is. On level 3 of seed 3, 117 of the 712 faces have a picture.

## 9. Hand-off log

- **14 November.** The brief, the sheets, the questions; the owner took the defaults. This plan written.
- **14 November, later.** Built, steps 1 to 6, on `claude/gallant-dijkstra-9x79xe` (harness only; nothing played by hand). The art
  (`src/art/parts/mice.c`: the small ones made for the level's stage, the rat at init, the holes on each material, `make sheets`'
  `mice`, kept as `media/sheets/mice/game.png` beside the mock-ups the owner saw); the rat in every kind table but the portraits
  (section 5); the mice (`src/crawl/game/mice.c`); drawn in play with the eye pass and the mark; the chirps (`snd_sfx_pitch`); the rat's
  page held back and its growth strip; `CRAWL_MICE`, `CRAWL_MICESTAGE`, `CRAWL_TPMICE`. The green is the palette's last entry, C_CLEAR's
  own: thirty frames of the title, the menus, the bestiary, her room, the crypt and HAUNT.EXE with it painted magenta and the mice off
  showed none of it, so nothing else draws it. From level 16 one or two rats roam each level (section 5). The proof, the balance runs
  and the sanitizers are in docs/KNOWN-ISSUES.md: levels 1-14 log as the build before; from 15 the deaths are within the noise. Found
  and fixed on the way: the word going round three mice in a ring where no hole was in reach. Left: the owner's hand on it (how small
  the early ones read, how often they tell, the chirps); the rat's portrait. CI built and smoke-tested both the Windows and the browser's builds on the branch (run 137).
- **14 November, after the owner played the mice.** They "seem to scatter around properly", but now and then one span in place with
  its mark up: the mice's running was wrong (docs/KNOWN-ISSUES.md has how and the counts), and is fixed. The owner asked for the
  spiders on the walls; built as section 8, its defaults taken as listed there, with a first round of balance runs finding the spiders
  running too far to tell (now within 36 steps). The proof (levels 1-10 as the build before), the balance from 11 and from 15 within
  the noise, the sanitizers and CI are in docs/KNOWN-ISSUES.md. Left: the owner's hand on the spiders and on the mice since the fix.
- **14 November, after the owner played the spiders.** They floated and flew; built as sections 8.1 and 8.2, the stone's ways and
  the spiders painted on it, with a new picture of the spiderling from above (pushed as a4be3b5, CI run 140). The owner: the legs
  were wrongly placed; redrawn (section 8.2), the sheet made again, committed (7b7bf49) and held back from the branch until the owner
  passes the sheet. The proof, the balance from 11 and 15 (within the noise), the frames unchanged with the spiders off, and the
  sanitizers are in docs/KNOWN-ISSUES.md. Left: the owner's word on the redrawn art, then their hand on it; whether the spiders
  should keep off painted faces and shut doors (both in docs/KNOWN-ISSUES.md).
- **14 November, the owner's word on both, and to main.** "Spiders can slide under/over doors, open or closed. They should navigate
  around pictures. Go ahead and merge with main." Built as section 8.3: a first try routed round one picture at a time and crossed
  banners' tops between niches, so the way is searched on a grid of the stone. The proof (levels 1-10 as the build before), the
  balance from 11 and 15 (within the noise), the sanitizers: docs/KNOWN-ISSUES.md. Folded into main as one squashed commit, no
  version bumped. Left: the owner's hand on the spiders since their first build (kept to the stone, redrawn, round the pictures);
  whether niches' lintels should be crossed as doors' are; the rat's portrait.
