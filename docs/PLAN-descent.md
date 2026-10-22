# The descent: themes, wall tiles, enemies, bosses and bonus levels

**Status: a proposal, nothing built yet.** Written at 1.0.0b2 for whoever picks this up. The owner's
brief is quoted first; everything after it is a proposal to discuss, and the questions at the end need
answers before the work starts. Section 8 lists the code it touches.

## The brief

> Themes change every five levels, with each set decaying as the levels progress. Four sets, with
> additional palette adjustments if the sets are looped (past level 20). Stick with rocky, eerie
> textures: different types of rock, embedded glowing stones in some walls in the much later levels.
> It should feel like we are travelling deeper, with the potential for riches and great danger shown
> on the very walls themselves. This means revisiting some textures, like the brick walls. More
> banners as the levels progress, more tattered as we go deeper.
>
> Also wanted: ideas for enemies, new wall tiles, level themes, and a boss and bonus level structure.

## 1. What the game does today

- **Look per level.** `crawl_art_level(level)` (in `src/crawl_art.c`) retints the stone, floor,
  ceiling and bone ramps with one of eight tints (blue-grey, damp green, sandstone, cold blue, rust,
  violet, ash, bone-pale), cycling every eight levels. It dims slightly each level up to level 9 and
  regenerates the wall textures with a level seed. `decay_tex` then adds grime, creeping moss, drips
  and cracks, scaled by `decay()`, which goes from 0 on level 1 to 1 by level 8.
- **Wall textures** (`T_*` in `src/crawl.h`, 64x64, procedural): stone, rubble, skull wall, red brick,
  moss, burial niche, banner (one design: red with a skull and crossbones), three torch frames, doors,
  jambs, vault doors, the exit runes, the merchant's sign.
- **Room themes** (`TH_*`): corridor, ossuary, crypt, brick, moss. `theme_tex` picks a wall texture per
  theme. Banners appear only in crypt and brick rooms, on about one wall in five there.
- **Names.** Eight level names cycle: The Ossuary, Hall of Torches, Crypt of the Nameless, The Bone
  Gallery, The Drowned Vault, Catacomb of Saints, The Charnel House, Warrens of the Dead.
- **Enemies** (`MOB_*`):
  - Skeletons throw bones.
  - Wraiths loom, shriek and flee.
  - Bats bite and circle.
  - Wall skulls (from level 2) gather a fireball.
  - Shield-bearers (from level 3) close in with swords; their shields can break.
  - Spiders (from level 4) drop from corner webs and poison.
  - Vault guardians.
  - Damage is 4 + level (less after deaths on a level); throws get 5% quicker per level. Nothing caps
    either, so past level 20 they keep climbing.
- **Palette budget: full.** All 256 colours are spoken for in 16-entry ramps. New looks come from
  retinting ramps per level, not from adding colours. `R_FLAME` (fire) and `R_GLOW` (blue magic: spells,
  the wisp, the exit runes) are full-bright and ignore lighting; gold is lit like everything else and
  twinkles.
- **Pace.** The autopilot takes about 5.6 minutes a level and reaches level 6-9 in 50 minutes. A
  screensaver session will rarely see level 20; a person playing a long run, or a run resumed from
  saves, will.

## 2. Four sets of five: going down

Level `L` belongs to set `((L - 1) / 5) % 4`, at step `(L - 1) % 5` (0-4) within it, on loop
`(L - 1) / 20`. Steps 0-3 are ordinary levels; step 4 (levels 5, 10, 15, 20) is the set's boss level
(section 6). Bonus levels sit beside the sets (section 7).

The story the walls tell: first the crypts people built, then the tunnels they cut, then the caves
that were there before them, then what lives at the bottom.

| | I. The Crypts (1-5) | II. The Catacombs (6-10) | III. The Hollows (11-15) | IV. The Deep (16-20) |
|---|---|---|---|---|
| What it is | Burial vaults built by an order of monks: dressed stone, mortar, carved niches | Tunnels cut into wet rock beneath them; water has been in here | Natural caverns and an abandoned mine the diggers broke into | Black volcanic rock, crystal and fire; the bones here are fused into the stone |
| Main rock | Pale limestone ashlar, grey-blue | Dark slate and shale in strata, chisel-scored | Granite and basalt (columnar joints), timber props | Obsidian and black glass with a sheen |
| Growth | Moss, cobweb | Black algae, white calcite flowstone and drips | Lichen, roots, pale fungus | Crystal crust, ash |
| Floor / ceiling | Flagstones / vaulted dark | Wet flags with puddles / dripping rock | Gravel and rubble / rough rock, roots | Cracked glassy rock / glinting ceiling |
| Riches on the walls | Gilded niches, offerings | Silver veins in the slate | Copper-green and iron-red ore, quartz veins | Gold veins and glowing gem seams (the only glowing walls) |
| Danger on the walls | Scratches inside niches, by the end of the set | Flood lines and handprints, drowned bones in silt | Claw gouges, drag marks, fossil ribs of something huge, cave-in cracks | Magma glowing through fissures, scorch, bones half-melted into the glass |
| Light | Torches, chandeliers, candles | Fewer torches, lanterns, the odd brazier | Miners' lamps on timber, fire pits | The walls themselves: seams, geodes, magma |
| Banners | The order's banners, whole to fraying | Mouldering, sagging, stained | Torn, scorched, nailed to timber | Rags and bare poles |
| New enemy (section 5) | (today's roster) | The Drowned | Gargoyle, Ghoul | Shade, Ember imp |
| Boss (section 6) | The Ossuary Warden | The Drowned Abbess | The Bone Colossus | The Hollow King |

**Decay inside a set.** Each set starts fresh and rots towards its boss: `decay = step / 4` (0, .25,
.5, .75, 1) drives the existing grime, moss, drips and cracks, plus banner tatter and the danger tiles.
Depth comes from the set itself (darker base ambient and rock each set), so level 6 looks newer than
level 5 but deeper. Question 1 asks whether you would rather decay keep climbing across all 20.

**Level names.** Twenty, five per set. A draft, to be replaced:

- I: The Ossuary, Hall of Torches, Crypt of the Nameless, The Bone Gallery, *The Warden's Hall*
- II: The Drowned Vault, Catacomb of Saints, The Weeping Stair, Cistern of Bones, *The Abbess Below*
- III: The Charnel Mine, Warrens of the Dead, The Fossil Road, The Broken Seam, *The Colossus Deep*
- IV: The Glittering Dark, Veins of the Earth, The Ember Root, The Black Glass, *Throne of the Hollow King*

## 3. Wall tiles

Keep the texture slots as *roles* and regenerate them per set in `crawl_art_level`, so the generator
and room themes keep working unchanged. Add two new roles (riches, danger) and a glowing variant.

| Role (slot today) | I. Crypts | II. Catacombs | III. Hollows | IV. Deep |
|---|---|---|---|---|
| Main wall (`T_STONE`) | Limestone ashlar, large blocks | Layered slate, chisel marks | Granite / basalt columns | Obsidian with conchoidal fractures and a sheen |
| Rough wall (`T_RUBBLE`) | Crumbling ashlar | Fractured shale with flowstone | Boulder cave face | Shattered glass rock |
| Masonry (`T_BRICK`, **reworked**) | Rough-hewn stone blocks with deep joints (replaces the red clay brick) | Patched block repairs with a water line | Mine face with timber props | Basalt columns |
| Growth (`T_MOSS`) | Moss | Algae and calcite drips | Lichen and roots | Crystal crust |
| Feature A (`T_SKULLS`) | Skull wall | Bones in silt | Fossil ribs and vertebrae in rock | Bones fused into glass |
| Feature B (`T_NICHE`) | Burial niche | Flooded niche | Boarded mine adit | Geode cavity (glows) |
| Banner (`T_BANNER`) | The order's banner, five tatter stages | Same, mouldering | Same, torn and scorched | Rags, bare pole |
| **Riches** (new) | Gilded niche | Silver vein | Ore and quartz veins | Gold vein and gem seam (glow) |
| **Danger** (new) | Scratched stone | Flood line, handprints | Claw gouges, drag marks | Magma fissure (glow, pulsing) |

Notes:

- **The red brick.** Today it reads as town masonry. Proposed: rough-hewn stone blocks in set I,
  patched repairs in set II, then gone (the deeper you go, the less anyone built). The merchant's room
  is red brick too; question 4 asks whether it stays as it is.
- **Banners.** More of them with depth: in set I only in crypt rooms, from one wall in ten on level 1
  to today's one in five; from set II they spread into halls and caves as well, up to about one wall in
  three by set IV. Tatter goes from 0 to 4 with decay: whole, frayed hem,
  holes and slashes, burned and half gone, a rag on a pole. Whose banners they are deeper down is
  question 6.
- **Glowing stones** only in set IV, faint glints of quartz in late set III as a foreshadowing.
  Full-bright pixels already ignore lighting (the torches and exit runes work that way). Two cheap ways
  to make them live:
  - **Palette cycling**, the VGA trick: rotate a few entries of a full-bright ramp each frame in the
    frame's own palette. That gives a slow pulse for gems and a crawl for magma at no cost per pixel.
  - Each glowing wall adds a weak static light so the glow spills onto the floor (light budget 160;
    levels use 57-71 now).
- **Colour budget.** Without new ramps: gem glow from `R_GLOW` (cyan-blue, matching the magic), magma
  from `R_FLAME`, and gold veins from the gold ramp (lit, not glowing; they would glint in torchlight
  and twinkle like the treasure does). For other gem colours (amethyst, emerald), repurpose a ramp
  that set IV doesn't otherwise use (e.g. `R_BRICK` once brick is gone). That needs an audit of which
  sprites use which ramp. Question 8.
- **Secret walls.** `decay_tex` already scratches cracks into ordinary walls at depth, and secret walls
  are found by their cracks. The secret cracks must stay distinct: lit edges, a different shape, close
  range only. Check every set's textures against them.

## 4. Palette moods for loops (levels 21 and on)

After level 20 the four sets repeat with a mood laid over the set's own tints, a darker base ambient
and a numeral on the level name ("The Ossuary II"). Draft moods:

- **Loop 2 (21-40) Blood:** red-black shadows, warmer light.
- **Loop 3 (41-60) Frost:** cold blue-white, rime on the walls (a texture overlay like the moss).
- **Loop 4 (61-80) Ash:** grey and desaturated, ember accents.
- **Loop 5 and on, Void:** violet-black, with the glow hues shifted.

Difficulty needs a curve for loops: today's damage and throw speed keep rising without limit (question
24).

## 5. Enemies

**Superseded by [`PLAN-bestiary.md`](PLAN-bestiary.md)**, which also answers questions 11-14. Kept
here for the record.

Rules learned so far:

- In wide halls, ranged enemies are the killers (bones from far away were the main cause of the
  autopilot's deaths). New enemies should mostly be ambushers and closers, or be readable and dodgeable.
- Every enemy needs a counter the autopilot can use, or the screensaver turns into a death loop.
- Sprites are 64x64 billboards with a few frames.
- What exists to build on: bone and fire projectiles; melee; poison, slow and chill; spawning from walls
  and corners; fleeing.

Proposed newcomers, one or two per set (question 11: how many):

- **The Drowned** (set II). A bloated corpse that rises from a floor puddle as you pass. Slow and tough.
  Its grab holds and slows you while it bites. Fire works well on it; frost barely touches it.
- **Gargoyle** (set III). A carved figure in a wall alcove, asleep like the wall skulls, that wakes
  when you pass and swoops. Stone skin shrugs off fire and frost; bone spear and lightning crack it.
- **Ghoul** (set III). Fast, low, four-limbed. It leaps across wide halls to close distance, and stops
  to feed on bone piles to heal (deny it the piles).
- **Shade** (set IV). Invisible except inside light: the wisp's pool, torches, fire pits, the flash of
  a spell. It drains souls on touch. This makes the lights and the wisp matter.
- **Ember imp** (set IV). Small and quick, born from magma seams. It lobs embers that burn the floor
  for a moment.
- **Crystal sentinel** (set IV, optional). Slow and armoured. It reflects frost and bursts into shards
  when it breaks.

Cheap variety for older enemies: per-set variants by tint and a detail. Barnacled skeletons in set II;
soot-black skeletons and ember bats in set IV. Bats thin out as you go deeper, and spiders gather in
the Hollows.

## 6. Bosses

One at the end of each set, on levels 5, 10, 15 and 20.

**The level.**
- A normal level, a little smaller, with the exit inside or beyond a boss arena: a big chamber dressed
  for the set. The exit runes stay sealed while the boss lives ("THE RUNES WILL NOT OPEN WHILE IT
  LIVES"). The wisp points at the arena.
- Crossing the threshold:
  - the doors grind shut behind you;
  - a name card shows, like the level card;
  - the music changes;
  - a health bar appears along the top of the view.
- On a kill, a hoard spills out, the runes open, and the tally adds a BOSS row with the time taken.

**Dying in a boss fight.** Proposed: restart at the arena door, not the whole level. The boss returns
at full health, and the existing retry easing applies (each death softens its blows).

**The four.**

1. **The Ossuary Warden.** A skeleton knight twice your height behind a tower shield. Spells hitting
   the shield from the front do nothing: get round it, or batter the shield until it breaks (it
   already can for shield-bearers). Every so often it calls skeletons up out of the bone piles.
2. **The Drowned Abbess.** A wraith queen in a flooded nave.
   - She snuffs the arena's torches; in the dark she can't be hurt.
   - Relight the braziers (E) or bring the wisp close, and she is exposed.
   - She shrieks (chill), and the Drowned rise to her bell.
3. **The Bone Colossus.** A hulk built from the fossils in the walls, filling the cave. Slow. Its
   stamps bring stones down from the ceiling; their shadows show on the floor first. Break the glowing
   sockets at its shoulders, then the skull.
4. **The Hollow King.** A lich in a geode heart, shielded by four glowing crystal pillars.
   - Break the pillars to bring the shield down.
   - He blinks between the remaining pillars and raises Shades.
   - At half health the magma seams flare and part of the floor burns.

**Rewards.** A boss hoard and a **relic**, one per boss, that can't be bought (e.g. a crown, a fifth
ring setting, a robe of the deep). Question 18.

**The autopilot needs boss tactics:** keep 3-5 cells away, move along the arena's lanes, use its best
spell, and do the boss's one trick (go round the shield, light a brazier, aim at the glowing sockets,
break pillars). Tune it with simulations like the rest.

**Looped bosses:** the same four in the loop's mood, with more health and one new move each (question
20).

## 7. Bonus levels

**How to get in.** Proposed, in the Wolfenstein manner: on the fourth level of each set (4, 9, 14, 19),
one of the secret closets has a second, marked exit at its back. It leads to that set's bonus level,
then on to the boss. This ties the new secrets to something beyond loot. Alternatives: unlock a set's
bonus by finding every secret in it, or always offer one after the boss (question 21).

**The four bonus levels, one per set (drafts):**

- **I: The Offering Hall.** A treasure run against a candle clock: rows of candles burning down. No
  enemies. Gold kept.
- **II: The Flooded Nave.** Soul lanterns drift on the water and sink one by one. Reach them before they
  go under; a few Drowned get in the way.
- **III: The Warrens.** A maze homage to the old secret level in Wolfenstein: collect soul orbs while
  four wraiths hunt you. Pick up the wisp and they flee from you for a while.
- **IV: The Cat's Dream.** The merchant's dream: a room of pedestals where one ware is free, or a
  gauntlet of waves for souls (question 22).

**Rules.** Bonus levels are optional and can't cost the run: dying ends the bonus and you go on to the
next level with what you had. The tally gets a BONUS row. Whether the autopilot ever plays them is
question 23.

## 8. Building it: code, order and checks

- **Where:**
  - `crawl_art_level` takes the set, step and loop: per-set ramp tables, texture generators per role,
    banner tatter, the loop mood.
  - `theme_tex` and the room themes become per set.
  - `LEVEL_NAMES` becomes 20 entries, with numerals on loops.
  - New `T_*` roles: riches, danger, glow frames.
  - New `MOB_*` types and sprites.
  - The boss level flow lives in `start_level`, `place_exit` and the tally; bonus levels are a new
    level kind.
  - Palette cycling goes in `crawl_render` (on `f->pal`); glow lights via `light_static`.
- **Saves:** boss kills, relics and bonus unlocks need a version 2 save. Keep loading version 1, and
  remember `Gear` is copied raw.
- **Sound:** a boss music program, and a few effects per new enemy.
- **Budgets:** about 2 ms a frame now, so keep it under 3. Glowing walls raise the picture level on
  OLED screens; measure with `build/host apl` (the crypt averages 14% now, 10 minutes of autopilot).
- **Checks per phase:**
  - screenshots of each set at steps 0 and 4;
  - ASan/UBSan runs;
  - eight-seed 50-minute autopilot simulations against the previous build;
  - `CRAWL_FOLLOWWISP` on boss and bonus levels;
  - a Wine smoke test;
  - then the owner plays it.
- **Suggested order, each its own beta:**
  1. Sets, names, decay schedule and palettes, using today's textures. Small; lets you feel the rhythm.
  2. Wall tiles per set: the brick rework, banners with tatter, riches and danger tiles, glowing stones
     with palette cycling. The biggest art job; iterate on screenshots with the owner.
  3. New enemies, one set at a time.
  4. Boss levels and the four bosses.
  5. Bonus levels.
  6. Loops: moods and the difficulty curve.

## 9. Questions for the owner

**The sets and their look**

1. Should decay reset at the start of each set and run to the boss (proposed), or keep climbing across
   all twenty levels with each set starting more worn than the last?
2. Is the story right: built crypts, then cut catacombs, then natural caves and a mine, then the deep?
   Or should all four sets feel like natural rock?
3. The set names and the twenty level names in section 2 are drafts. Want to name any yourself?
4. Brick: replace it with rough-hewn stone blocks in set I and let it disappear deeper (proposed), or
   keep some brick everywhere? Should the merchant's red-brick room stay as it is?
5. Should floors and ceilings change per set too (puddles in II, gravel in III, glassy cracks in IV)?
   And any flooded floors you have to wade through, slowing you?
6. Whose banners hang in the caves and the deep? The order's, followed down? A cult's? Earlier
   expeditions'? One design throughout, or one per set? How far should tatter go: a bare pole by level
   20?
7. Glowing stones: which colours? Blue to match the magic, gold, and fire are free; amethyst or emerald
   cost an audit and a reused ramp. Pulse slowly or stay steady?
8. Is it fine to keep spells and the wisp blue throughout, or may the magic colour shift per set?
9. "Riches on the walls": decoration only, or can you pry gems and gold out of a seam with E for gold?
   If so, does it count towards the level's treasure, and can the autopilot do it?
10. "Danger on the walls": only the look of it, or real hazards (magma seams that burn if you brush
    them, ceilings that drop stones)?

**Enemies**

11. How many new enemies are right: one per set from set II (three), or the six above?
12. Should older enemies fade out deeper (no bats in the deep), or always be in the mix?
13. Do you like an enemy that is only visible in light (the Shade), which makes the wisp and torches
    matter in a fight?
14. Ranged enemies proved deadly in the wide halls. Keep new ones mostly close-range and ambushers, as
    proposed?

**Bosses**

15. A boss on levels 5, 10, 15 and 20: yes? Does the merchant appear before a boss as usual?
16. Health bar, name card and boss music: all three? Where should the bar sit? Over the top of the view
    is proposed, so the status bar stays as it is.
17. Should the arena be a sealed room inside a normal (smaller) level, as proposed, or a short level
    that is mostly the arena?
18. Rewards: a relic per boss that can't be bought (what kinds?), a hoard, souls? Does the boss hoard
    count towards treasure?
19. Dying in a boss fight: restart at the arena door with the boss back at full health (proposed), or
    restart the whole level?
20. Looped bosses: the same four, stronger with one new move each, or new bosses for later loops?

**Bonus levels**

21. How do you get in: a marked second exit in a secret closet on levels 4, 9, 14 and 19 (proposed),
    finding every secret in a set, or always after each boss?
22. Which kinds do you want? The candle-clock treasure run, the sinking lanterns, the wraith maze
    homage, the cat's free ware or a soul gauntlet? Mix and match per set?
23. Should the autopilot ever play bonus levels in screensaver mode? It only presses about a third of
    the secrets it passes, so with a secret entrance it would rarely get in.

**Loops and progression**

24. After level 20: should difficulty keep rising, plateau, or step up once per loop (proposed: damage
    and throw speed capped at their level-20 values, plus a step per loop)?
25. Loop moods: blood, frost, ash, void. Keep, change, or one mood for all loops?
26. Should the merchant change per set (follow you down, different wares deeper), or stay the same?
27. Should treasure be worth more deeper, so the riches on the walls are matched by the gold?

**Process**

28. Order: is the suggested order (sets and palettes first, then tiles, enemies, bosses, bonus levels,
    loops) right? One beta per step, or fewer, bigger releases?
29. Is a new save version fine? Old saves would still load and carry on.
30. For the art, should the next agent send you screenshot sheets of each set and wait for a yes before
    moving on (as with the title cards)?
