# The bestiary and the dressing: who lives down there, and when you meet them

**Status: agreed; asset generation under way.** Written at 1.0.0b2 as a companion to
[`PLAN-descent.md`](PLAN-descent.md). It replaces that plan's section 5 (Enemies) and its questions
11-14; everything else there (sets, wall tiles, bosses, bonus levels, loops) stands.

**Decided (22 October 2026):** the owner reviewed the plan and found it sound, so every question
below (B1-B34) takes its recommended option (marked *). Under B34 that means the descent plan's
proposals stand as its defaults too. Work began with the assets: the rank tables and the sprites
for the new creatures and decorations, each shown on a sheet (`make sheets`) for a yes before it
goes into play.

**Where it stands (22 October 2026): assets drawn, none of it in play yet.**

- **Rank tables:** `crawl_rank_lut` in `crawl_art.c`.
- **Creature sprites:** all eleven new creatures, plus egg sacs, spiderlings and the imp's ember.
- **Decorations:** the dressing for all four sets.
- **Sheets:** `tools/sheet.c` draws them by rank; `make sheets` writes one PNG per group, and a
  line-up of every creature at its proposed size, to `build/sheets`.
- **Checks:** the game itself is unchanged, and the smoke test, the frame time and an ASan run are
  clean.
- **Approved:** the owner liked every sheet, and the line-up's sizes, as a first shot (22 October).
  Their look and feel will be refined as they playtest, so the drawings are not final.
- **Decided while drawing:**
  - Knights and deacons have their own rank families: plate (`PF_ARMOUR`, whose rank V darkens the
    iron rather than going pale) and robe (`PF_ROBE`). With only their bones taking the rank, a
    knight in full plate would hardly show it.
  - The knight's blade is drawn in `R_GHOST` so that it stays steel at every rank.
  - The deacon's raising light is the blue of magic, not green: the palette has no full-bright
    green.
  - Rank shown in the eyes is left for the renderer.

## The brief

> Enemy types are important, as is variety. Think hauntings, like zombies, different types of
> skeletons, spiders that navigate the world, an imposing wraith that follows collision, and a suite
> of palettes for each that allows for far more mixed variety of strengths. It should play like a
> real RPG.
>
> We should plan on how we roll out each enemy type, so all of them are not visible in the first few
> levels. Same with our decorations, they should slowly be introduced to the player as they dive
> deeper into the crypts.

## 1. What the game does today

From the code at 1.0.0b2 (`src/scene_crawl.c`, `src/crawl_art.c`):

| Creature | From | HP | Speed | What it does | Today's counter |
|---|---|---|---|---|---|
| Skeleton | 1 | 20 | 0.9 | Once it has seen you within 9 cells, it stands and throws bones (leading you a little) | Anything; it is the bread and butter |
| Wraith | 1 | can't be hurt | 0.5-0.75 | Drifts straight at you **through walls**, shrieks within two cells (chill), then flees and fades; melts away after 30 s | None needed: it only frightens |
| Bat | 1 | 10 | 1.7 | Circles your face; bites after 3 s of contact | Frost, sparks, seeker wisps |
| Wall skull | 2 | 1 | fixed | Wakes when you walk at it, gathers fire for 3 s, throws a fireball (double damage) | Any single hit, before it fires |
| Shield-bearer | 3 | 30 + shield | 1.0 | Closes in; a sword after a 0.45 s wind-up; the shield stops spells from the front until it breaks | Bone spear, chain lightning (pierce, arc) |
| Spider | 4 | 15 | fixed | Waits in a corner web with only its eyes showing; drops within two cells, bites (poison, slow), climbs back | Frost, wisps; the hunter's hood spots it |
| Guardians | 1 | | | One per silver or crimson vault: a shield-bearer (a skeleton before level 3) or, from level 4, a spider | |

- **Strength.** HP grows 10% a level, damage is 4 + level, throws come 5% quicker a level. Nothing
  caps them.
- **Numbers.** Four wanderers at the start (five from level 4), 7-40 steps away and out of sight, half
  of them skeletons, a quarter wraiths, a quarter bats, a fifth shield-bearers from level 3. Then 2-4
  wall skulls, 2-4 spiders and the guardians. After 11 s with nothing happening, something wakes 4-8
  steps ahead, while fewer than eight wanderers are alive. `MAXMOBS` is 20.
- **Finding you.** Creatures step from cell to cell. Within 7 cells and in sight, they take the
  neighbouring cell nearest you; otherwise they wander at random. There is no path-finding: turn a
  corner and they lose you. Wraiths ignore walls altogether.
- **Colour.** Every sprite is drawn from shared 16-step ramps (bone, iron, wood, ghost, purple and so
  on). The renderer can already swap one ramp for another per sprite as it draws (`Draw.remap`, used
  for the merchant's wares), and darkening stays inside a ramp, so a recoloured sprite still lights
  and fogs correctly.
- **Decorations.** Every kind is in play from level 1: bones, skull piles, cobwebs, urns, candles,
  chains, meat hooks, roots, web sheets, totems at crossings, chandeliers, fire pits, graveyards and
  gibbets. Only their density rises with decay.
- **Sounds not yet used by CATACOMB:** `SFX_GROAN` (a zombie groan) and `SFX_DIRT` (earth breaking)
  are already in the synth, ready for zombies rising from graves. `SFX_WAIL`, `SFX_CHATTER` and
  `SFX_CACKLE` are there too.

## 2. Rules for every creature

1. **One new thing a level.** A level brings at most one new creature and at most one new rank. Its
   first appearance is a lone one, somewhere lit, not in a mixed fight, so it can be learnt.
2. **Foreshadow.** A creature's sign shows a level or two before the creature does: graves before
   zombies, webs before spiders (already true), puddles before the Drowned, frost before the Wraith.
3. **Colour means strength, the same way everywhere.** One ladder of ranks across every family
   (section 4). Learn it once.
4. **A tell and a counter.** Every creature has a sound and a pose before it hurts you, and a counter
   the autopilot can use. Otherwise the screensaver becomes a death loop.
5. **Ranged killers are rare and slow to fire.** The wide halls taught this: bones from far away were
   the autopilot's main killer.
6. **Variety by mixing, not by replacing.** A level draws from a pool that widens as you go down. Old
   kinds mostly stay, in stronger ranks, rather than vanishing.

## 3. Navigation: how they find you

- **A hunt field.** A breadth-first search from the caster's cell over the map, out to about 24 steps,
  rebuilt whenever the caster changes cell (the map is 45x45: a few thousand cells, well under a
  tenth of a millisecond). Every hunting creature steps to the neighbour with the lower number. It
  goes round corners, through open doors and between rooms. The wisp's route already works this way
  (`wisp_bfs`, `build_route`).
- **Alerts.** A creature hunts once it has seen you, been hit, or heard a spell or a fight within a
  few cells. It gives up after a while out of sight and goes back to wandering. This is how
  Wolfenstein's guards behaved, and it is what makes a level feel inhabited.
- **Wall-bound or not.** Today's wraiths drift through walls. The new Wraith (section 5) and
  everything else walk the field. A ceiling field for spiders that crawl overhead is the same grid,
  ignoring graves and fire pits on the floor.
- **Doors.** Proposed: skeletons, zombies and the Wraith open ordinary doors (never vault doors);
  spiders, bats and hounds wait behind them. Question B9.
- **The autopilot.** It walks lanes and turns at right angles. Creatures that follow it round corners
  will make it harder. Something solid and unkillable in a one-cell hall can trap it, so it needs a
  back-off-and-route-round behaviour before the Wraith goes in.

## 4. Ranks: strength you can see

**How.** The Doom trick: a 256-byte translation table per rank, applied as a sprite is drawn. A rank
maps a family's body ramp onto another ramp. It costs nothing per pixel beyond today's remap, and it
needs no new palette entries (all 256 are taken). One sprite set per creature serves every rank.

**The ladder** (the same order in every family):

| Rank | Colour | Ramp | Word on the caption | HP | Damage | Speed | Souls |
|---|---|---|---|---|---|---|---|
| I | its own (bone, grey flesh, black chitin) | own | (none) | x1 | x1 | x1 | 1 |
| II | green | `R_MOSS` | ROTTEN | x1.6 | x1.15 | x1.05 | 2 |
| III | blue | `R_GHOST` | COLD | x2.4 | x1.3 | x1.1 | 3 |
| IV | red | `R_BLOOD` | BLOODY | x3.5 | x1.5 | x1.15 | 4 |
| V | black | `R_IRON` | BLACK | x5 | x1.7 | x1.15 | 6 |
| Gilded (rare) | gold | `R_GOLD` | GILDED | x3 | x1.2 | x1 | 5, and it carries treasure |

The numbers are a starting point for simulation, not a promise. Proposed: a rank means about the
same strength at any depth. Depth then shows in *which* ranks you meet, and the per-level HP and
damage growth flattens (question B13).

**The mix, by set:**

| | Set I (1-5) | Set II (6-10) | Set III (11-15) | Set IV (16-20) |
|---|---|---|---|---|
| Ranks met | I; II from level 3 | I-II; III from level 8 | I-III; IV from level 13 | II-IV; V from level 18 |
| Typical share | 85 / 15 | 55 / 35 / 10 | 20 / 40 / 30 / 10 | 25 / 35 / 30 / 10 |
| Gilded | none | from level 7, about one level in three | same | same |

Loops past 20 push the mix up a rung each time, capped at V.

**From the mock-up** (today's sprites run through the ladder; the sheet is in the session, and
regenerating it is a 60-line tool, see section 10):

- It reads well on skeletons, shield-bearers and wall skulls, at level 1 and at level 8.
- A creature that is already black (spiders, bat bodies) can't show rank V. It needs a stand-in:
  bone-pale ("deep things lose their colour") is proposed.
- Rank V bats nearly vanish against the dark. Bats may stop at rank IV, which are the ember bats the
  descent plan wanted for set IV.
- Moss that decay paints onto bones stays green at every rank, and blends into rank II. The ladder
  should apply after decay, or decay should skip ranked sprites.
- Eyes are drawn full-bright and survive any remap, so they can carry rank as well (dim ember, green,
  blue, red, white). Today a skeleton's eyes turn from ember to blue with decay; that would give way.
- The gold ramp has 13 steps, not 16. The table has to scale onto it; today's `ramp_at` clamps
  instead, which is fine for items but not for a whole body.
- **Spirits** (spectres, the Wraith, shades) are ghost-blue by nature. They stay off the ladder and
  show strength by size and eyes.

**A second cue besides colour** (question B12): higher ranks stand about 4% taller per rank, their
eyes brighten, and a caption names them the first time each is seen on a level ("A COLD SHAMBLER").
That way rank can be read without telling green from red.

## 5. The bestiary

The draft below has **eleven new creatures** beside today's six, four bosses (from the descent plan)
and a bench of extras to swap in. The names are drafts.

### Skeletons: the order's dead

| Name | From | What it does | Tell | Counter / weak to |
|---|---|---|---|---|
| Skeleton (today) | 1 | Throws bones | Arm raised, a short whoosh | Anything |
| Shield-bearer (today) | 4 (was 3) | Shield up, sword | Blade raised over the head | Bone spear, chain lightning; get round it |
| **Bone hound** | 8 | Skeletal dog, in pairs. Fast; closes the gap, bites and darts back | Clattering paws, a dry growl | Frost (slows), sparks, chain lightning; few HP |
| **Skeleton knight** | 13 | Plate armour, two-handed sword. Slow, with a long wind-up for a heavy blow. Plates fall off as it is hurt (three looks), and it gets quicker as they go | A scrape of steel and a raised blade, held for a long beat | Bone spear and lightning through armour; fire is poor until the plates are off |
| **Deacon** | 18 | A skeleton priest in robes. Keeps its distance and raises fallen skeletons from their remains; flees when you close in | A chant, and a green glow over a heap of bones | Kill it first: seeker wisps round corners; Dread |

Possible trait: from rank III, a skeleton's remains pull themselves together again after a few
seconds unless burnt (fire, hellfire) or finished with the bone spear. Once each. Question B5.

### Zombies: the buried

| Name | From | What it does | Tell | Counter / weak to |
|---|---|---|---|---|
| **Shambler** | 2 | Rises from a grave in a graveyard room as you pass. Slow and tough. Grabs: holds you for a moment and bites | Earth breaking and a groan (`SFX_DIRT`, `SFX_GROAN`); a hand out of the soil | Fire (x1.5); walk away from it. Frost does little |
| **The Drowned** | 7 | Bloated; rises from a floor puddle as you pass. Slow; its grab also slows you. Bursts when it dies, leaving a poison pool for a few seconds | Bubbles in the puddle | Fire; keep back from the burst |
| **Ghoul** | 12 | Fast, low, on all fours; leaps the last few cells. Stops to feed on bone heaps and heals; takes double damage while feeding | A wet snarl; the crunch of feeding | Frost; catch it feeding |

### Spiders: they get about

| Name | From | What it does | Tell | Counter / weak to |
|---|---|---|---|---|
| Web spider (today) | 6 (was 4) | Waits in a corner web, drops, bites (poison, slow) | Two red eyes in the web | Frost, wisps; the hunter's hood |
| **Hunting spider** | 11 | Walks the halls **on the floor or across the ceiling** by the hunt field, follows you between rooms, and drops or leaps when close | Skittering overhead; a thread catching the light | Frost; the hunter's hood spots it; wisps |
| **Brood mother** | 14 | Big and slow. Lays egg sacs that hatch spiderlings, a fast swarm of tiny one-hit creatures. Kill her and the unhatched sacs die with her | Sacs pulsing on the walls; a rising hiss as they split | Hellfire and chain lightning for the swarm, fire for the sacs |

### Spirits: hauntings proper

| Name | From | What it does | Tell | Counter |
|---|---|---|---|---|
| Spectre (today's "wraith", renamed) | 1 | As today: drifts through walls, shrieks, flees; can't be hurt | A moan | None needed |
| **The Wraith** | 9 | One per level, tall (about one and a half times your height) and slow. **It walks the halls like you do: solid, bound by walls, opens doors.** It follows you anywhere on the level. Its touch drains heavily and chills. Not killed by spells; fire, the wisp's light and Dread drive it back for a while. It goes when you take the exit | Doors opening on their own behind you; frost on the walls before it arrives; the torches gutter as it passes | Keep moving; turn and burn it back; carry the wisp. Questions B6-B10 |
| **Shade** | 16 | Invisible except inside light (the wisp's pool, torches, fire pits, the flash of a spell). Drains souls on touch | A cold patch; footsteps with no one there | Light: the wisp, hellfire, standing by a fire |

The Wraith is meant as pressure, like the ghost in Spelunky that comes for you when you dawdle.
Proposed: it appears at the far end of the level once you have spent about four minutes there. The
autopilot leaves at 4.5-6 minutes, so in a screensaver run it shows up late and herds the run
towards the exit.

### The deep (from the descent plan)

| Name | From | What it does | Counter |
|---|---|---|---|
| **Ember imp** | 17 | Small and quick, born from magma seams; lobs embers that burn the floor for a moment | Frost |
| **Gargoyle** | 19 | A carved figure in a wall alcove that wakes when you pass and swoops; stone skin shrugs off fire and frost | Bone spear, lightning |

### The bench: extras to swap in (question B2)

- **Poltergeist.** Something unseen in a room flings urns and bones at you. You find it by a cold
  spot and a flicker, and hit it with soul drain or wisps. The most "haunted" of the lot.
- **Haunted armour.** A suit of armour on a stand (a decoration from set I) that wakes. Could take
  the knight's place, or join it.
- **Mimic.** A treasure chest that bites. It breathes, very slightly. The autopilot loves treasure, so
  this needs care.
- **Banshee.** Wails from far off: chill and slow at range, with a long rising wail first.
- **Bone archer.** Ranged, with a long draw. Only in vaults and across great halls, if at all.
- **Crawler.** A legless torso that drags itself out of bone heaps. Low and slow.
- **Bloater.** A swollen corpse that bursts into a gas cloud.
- **Crystal sentinel** (from the descent plan). Slow and armoured; reflects frost; bursts into shards.

### Where they live

Rooms get residents and halls get wanderers:

- shamblers in graveyards;
- skeletons in ossuaries;
- spiders in webbed corners and, deeper, in egg-sac rooms;
- the Drowned in flooded rooms;
- imps by the fire pits of the deep;
- hounds, hunting spiders and bats roam the halls.

From set II, some come as packs: a knight with two hounds, a deacon with three skeletons, a brood
mother with her sacs.

## 6. Weaknesses

Strong (++), good (+), poor (-), useless (x). The autopilot's spell choice (`choose_spell`) would read
this table instead of today's handful of special cases.

| | Fire | Frost | Chain | Spear | Drain | Hellfire | Wisps | Sparks |
|---|---|---|---|---|---|---|---|---|
| Skeletons | | - | + | + | - | + | | |
| Shield / knight | - | | ++ | ++ | | | | - |
| Hounds, bats | | ++ | + | | | | + | + |
| Zombies | ++ | - | | | + | ++ | | |
| Spiders | + | ++ | | | | + | + | |
| Spirits (Wraith, shade) | drives back | x | x | x | + | drives back | | x |
| Gargoyle, sentinel | - | - | ++ | ++ | x | | | - |

## 7. The rollout, level by level

One new creature and at most one new rank a level, with each creature's sign a level or two ahead of
it.

| Level | Set | New creature | New rank | Shown here, for later |
|---|---|---|---|---|
| 1 | I The Crypts | Skeleton, bat, spectre | I | Quiet graveyards (zombies) |
| 2 | | **Shambler** | | Skull niches in the walls (wall skulls) |
| 3 | | Wall skull (was level 2) | **II green** | Broken shields and swords on the floor |
| 4 | | Shield-bearer (was level 3) | | Web sheets (spiders) |
| 5 | | **Boss: the Ossuary Warden** | | |
| 6 | II The Catacombs | Web spider (was level 4) | | Puddles and flood lines |
| 7 | | **The Drowned** | **Gilded** | Gnawed bones, a chain and collar |
| 8 | | **Bone hound** | **III blue** | Hoarfrost on the walls, a guttering torch |
| 9 | | **The Wraith** | | A bell, somewhere far off |
| 10 | | **Boss: the Drowned Abbess** | | |
| 11 | III The Hollows | **Hunting spider** | | Stripped carcasses |
| 12 | | **Ghoul** | | An armour rack, a fallen helm |
| 13 | | **Skeleton knight** | **IV red** | Egg sacs |
| 14 | | **Brood mother** | | Fossil ribs in the rock |
| 15 | | **Boss: the Bone Colossus** | | |
| 16 | IV The Deep | **Shade** | | Magma seams |
| 17 | | **Ember imp** | | Black altars, candles in rings |
| 18 | | **Deacon** | **V black** | Statues in alcoves |
| 19 | | **Gargoyle** | | |
| 20 | | **Boss: the Hollow King** | | |

- **Thinning out.** Spectres stop after set II, once the Wraith has taken over the role. Bats grow rare
  in the deep (red ember bats only). Wall skulls stay throughout, in higher ranks.
- **A creature's debut** is a lone one, near a light, in a room off the start rather than a hall
  crossing.

## 8. Decorations, introduced as you go down

Today everything is there from level 1. Proposed:

| Decoration | From | Until | Notes |
|---|---|---|---|
| Bone heaps, skull piles, cobwebs, urns, candles, chains | 1 | always | the basics |
| Chandeliers, fire pits | 1 | set II | hung and lit by the builders; the first room keeps its fire |
| Banners | 1 | always | tatter per the descent plan |
| Graveyards (quiet) | 1 | set II | zombies rise from level 2 |
| **Candle stand** (new, simple) | 1 | 2 | lights the hall crossings until the totems come |
| Totems (skull poles) | 3 | always | |
| Meat hooks | 3 | set II | |
| Gibbets, empty | 3 | set II | |
| Gibbets with prisoners | 4 | set II | |
| Web sheets | 4 | always | spiders from 6 |
| Puddles, flood lines, flowstone | 6 | set II | the Drowned from 7 |
| Roots through the ceiling | 6 | set III | |
| Lanterns, braziers | 6 | set III | the lights of the cut tunnels |
| Timber props, miners' lamps, a cart | 11 | set III | |
| Egg sacs, fungus | 13 | set III | |
| Crystal clusters, magma vents, obsidian spikes | 16 | set IV | the lights of the deep |
| Altars, statues in alcoves | 17 | set IV | |

- **Every room still gets a light**, so where a later light-giving decoration is missing on an early
  level, an earlier one stands in (the candle stand, chandeliers, fire pits).
- The early levels will look a little barer than they do now. That is the point, but it will show in
  the level 1 screenshots and previews.
- **Decorations that wake** (graves, puddles, egg sacs, alcove statues; perhaps armour stands and
  chests) always have a tell, so a person can learn which things are only things. Question B28.

## 9. RPG systems on top

Options, each a question below. None of them is needed for the creatures themselves.

- **Names on screen.** A caption slides in, the way the HUD pop-ups do now, the first time a kind or
  a rank is seen on a level: "A COLD SHAMBLER". Nothing stays on screen, so OLED panels stay safe.
- **Bestiary.** A page beside the wardrobe (Tab) that lists what you have met: kills, ranks seen, and
  weaknesses, filled in after a few kills with each spell. It would need a version 2 save.
- **Drops.** Higher ranks sometimes drop gold or a potion; the Gilded always drop treasure.
- **Growth.** Today the caster grows only through gear bought with gold. A light option: souls ever
  taken (not spent) raise a soul rank, worth a little max HP and spell power each.
- **Champions.** A rare creature one rank above the mix, with one trait (swift, stoneskin, venomous,
  vampiric, burning) and a flicker of full-bright light about it. Or named uniques ("Brother Aldric,
  the Unburied"), one a level.
- **Health bars** on bosses only, as the descent plan proposes. No damage numbers.

## 10. Building it

**Code.**

- A creature table (a `Kind`):
  - debut level, family, size;
  - base HP, speed and damage;
  - body ramps and rank stand-ins;
  - weakness row, behaviour flags, sounds.
- `Mob` gains `kind` and `rank`. The eighty-odd `m->type == MOB_...` tests in `scene_crawl.c` become
  flag tests, and `random_type` becomes a pick from the level's pool.
- `Draw.remap` becomes a pointer to a translation table: one table per rank per family, built once
  in `crawl_art_init`.
- The hunt field, alerts and door opening go beside `wisp_bfs`.
- `MAXMOBS` goes from 20 to about 32, for spiderlings and packs.
- New sprites: roughly 6-10 frames per creature, 64x64. That is about 80 more slots in `NSPR`, near
  330 KB; trivial.
- Each creature gets a sound or two in `snd.c`. Graves and puddles get rising animations.
- Saves: unchanged, unless the bestiary or soul rank goes in. Those mean a version 2 save that still
  loads version 1.
- `tools/sheet.c` (`make sheets`) links `crawl_art.c` and writes a PNG of each group of sprites by
  rank, and a line-up at the sizes they are drawn. It is how each new creature is shown for
  approval.

**Order** (question B29):

1. **Groundwork, no new creatures.**
   - The creature table, ranks, the hunt field and alerts.
   - The new debut levels and the decoration schedule.
   - The weakness table in the autopilot.
   - Today's six creatures, now in ranks and following you round corners. A beta on its own: it
     changes how every level plays.
2. **Set I newcomers:** the shambler and the graves.
3. **Set II:** the Drowned, the bone hound, the Wraith (with the autopilot's back-off).
4. **Set III:** the hunting spider, the ghoul, the knight, the brood mother.
5. **Set IV:** the shade, the imp, the deacon, the gargoyle.

Bosses and bonus levels follow the descent plan. The descent plan's step 1 (sets, names, decay
schedule) is best done before step 1 here, since debuts and rank mix are laid out by set.

**Checks, every step:**

- a sprite sheet of each new creature, all ranks and frames, sent for a yes before it goes in;
- ASan/UBSan runs;
- eight-seed 50-minute autopilot runs against the previous build: deaths, levels reached, level
  times;
- `build/host bench` under 3 ms a frame (1.7 now) and `apl` for picture level;
- a Wine smoke test;
- then the owner plays it. Navigation will make today's creatures noticeably more dangerous, so expect
  a balance pass in step 1.

## 11. Achievements and the bestiary page

Asked for by the owner on 22 October, after the art was approved; the answers below are theirs.

**Kept in the save, per save.** The progress save and each of the three graves carry their own
counts and achievements, so loading an old grave rolls them back to what that grave knew. That needs
a version 2 save (version 1 still loads, with everything at zero) and a larger slot than today's 256
bytes (`SLOT_MAX`), since the counts alone come to about 300 bytes.

**Everything counts.** Kills, secrets and level clears count whoever makes them, the autopilot
included, and the pop-ups show in screensaver mode too.

**Achievements:**

- **Slayer**, one per creature per rank it comes in. Its lowest rank needs 60 kills, then 40, 25,
  15 and 8 for each rank up; gilded needs 3. Only the ranks a creature can actually be met in are
  listed: a deacon, first met in the Deep, starts at rank II. Spectres and the Wraith can't be killed
  and have none.
- **Secrets:** the first, then 5, 10, 20, 30, 40 and 50 found (the steps after the first are a
  default, not the owner's).
- **Levels:** one for clearing each of levels 1 to 20, then one for each further loop (40, 60, 80
  and on, as far as anyone goes).

**The tracker.** A pop-up in the upper right of the view, in the manner of Steam's:

- a small framed box slides in with the creature (or a trophy) and its name, the achievement's
  name, and a chime;
- it stays about four seconds, then slides out;
- a progress note shows halfway to an achievement ("SKELETONS 30 / 60");
- several unlocking at once queue up.

Nothing stays on screen, so OLED panels stay safe.

**The bestiary page,** opened with B and paged with W/S/A/D:

- one creature to a page, drawn large in each rank you have met it in;
- its name, a line or two in the game's voice, and the kills in each rank;
- weaknesses filled in after a few kills with each spell;
- creatures not yet met show as a dark silhouette with "???";
- a last page lists the achievements, earned and not.

B is a new key for the platform layer and the harness.

**Order:** after the groundwork (it needs the creature table and ranks), as its own step before the
new creatures go in, so that each creature arrives with its page and achievements.

## 12. Questions for the owner

Answer as many as you like ("B1 a, B6 b"). Anything left unanswered takes the recommended option
(marked *).

**The creatures**

- **B1.** The rollout in section 7 moves three of today's debuts later: wall skulls from 2 to 3,
  shield-bearers from 3 to 4, web spiders from 4 to 6. a) fine* b) keep today's debuts and fit the
  new ones round them.
- **B2.** Which of the bench do you want, and in place of which drafted creature? (poltergeist,
  haunted armour, mimic, banshee, bone archer, crawler, bloater, crystal sentinel)
- **B3.** How many kinds in all? a) about seventeen, as drafted* b) fewer and more polished, about
  twelve c) more, about twenty-five, so loops keep surprising.
- **B4.** Spiders that walk the halls arrive on level 11 in the draft. a) fine, the Hollows are their
  place* b) earlier, about level 7-8 c) also let rank II and up web spiders come down and hunt.
- **B5.** Skeletons that pull themselves back together (rank III and up, once, unless burnt)?
  a) yes* b) no.

**The Wraith**

- **B6.** Can it be killed? a) no: driven back by fire, the wisp's light and Dread; it leaves when you
  take the exit* b) yes, but very tough, for a big reward c) only by a ritual on the level (lighting
  braziers).
- **B7.** When does it come? a) after about four minutes on a level, from the far end* b) on every
  level from its debut, from the start c) on some levels only.
- **B8.** Its touch: a) heavy drain and a chill that slows* b) death outright c) it takes souls or
  gold.
- **B9.** Doors: a) it opens them, slowly; doors opening on their own are its tell* b) doors stop it,
  so rooms are refuges.
- **B10.** Names: today's harmless drifters are called wraiths. a) they become spectres and the new
  one is the Wraith* b) keep them as wraiths and give the new one a name of its own (suggestions
  welcome).

**Ranks and colours**

- **B11.** a) one ladder for every family: own colour, green, blue, red, black, gold* b) each family
  its own named colours c) the ladder plus a few family specials (an ember-red bat, a pale deep
  spider).
- **B12.** A second cue besides colour (size, brighter eyes, a name caption)? It matters if you or
  anyone you show it to tells colours apart poorly. a) yes, all three* b) colour only.
- **B13.** a) a rank means about the same strength at any depth; depth changes the mix* b) ranks on
  top of today's growth per level c) a step up per set.
- **B14.** The Gilded: a) rare, tough, carry treasure, and run when hurt* b) just the top rank
  c) leave them out.
- **B15.** Champions: a) not yet, ranks are enough to start* b) rare champions with one trait
  c) one named unique a level.

**Making it an RPG**

- **B16.** Growth: a) gear stays the only growth b) souls taken raise a soul rank, a little max HP and
  spell power each* c) full experience and stats.
- **B17.** Drops: a) souls only, as now b) higher ranks sometimes drop gold or a potion; the Gilded
  always drop treasure* c) also, rarely, gear.
- **B18.** A caption the first time a kind or rank is seen on a level? a) yes* b) no.
- **B19.** A bestiary page by the wardrobe? a) yes, in step 2 or later* b) not now.
- **B20.** Weaknesses: a) every family has them, as in section 6, and the autopilot plays to them*
  b) keep it light, as now.
- **B21.** Health bars: a) bosses only* b) a small bar over any creature once it is hurt.

**Who is where**

- **B22.** a) rooms with residents plus wanderers in the halls* b) wanderers only, as now.
- **B23.** Something waking ahead after 11 s of quiet: a) keep it for the autopilot, much rarer when
  you play, so a cleared level stays mostly cleared* b) keep it for both c) none.
- **B24.** Packs (a knight with hounds, a deacon with skeletons): a) from set II* b) no.
- **B25.** Old kinds: a) mostly stay in higher ranks; spectres retire after set II and bats grow rare
  in the deep* b) everything stays.

**Decorations**

- **B26.** The schedule in section 8 moves hooks, gibbets, totems, web sheets and roots later than
  level 1, and early levels look barer. a) fine* b) keep everything from level 1 and only add new
  things deeper.
- **B27.** A sign of each creature a level or two before it comes: a) yes* b) no.
- **B28.** Decorations that wake: a) yes, always with a tell (graves, puddles, sacs, statues)*
  b) also armour stands and chests c) no: decorations are only ever decorations.
- **B29.** Anything you want in the dressing that isn't listed (coffins, sarcophagi, bells, ossuary
  chandeliers made of bones, candles on skulls...)?

**Process**

- **B30.** Order: a) groundwork first (ranks, navigation, the schedule), then the creatures set by
  set* b) the four you named first (zombies, skeleton types, walking spiders, the Wraith), then the
  rest.
- **B31.** The descent plan's sets (its step 1) before this? a) yes, sets first* b) creatures first,
  on today's eight-level cycle.
- **B32.** A sprite sheet of each creature for your yes before it goes in? a) yes* b) only for the
  big ones.
- **B33.** Releases: a) a beta per step* b) fewer, bigger ones.
- **B34.** The descent plan's questions on the sets (1-10) and bosses (15-20) are still open, and the
  first step needs 1-4 and 28. Answer them now, or pick defaults?
