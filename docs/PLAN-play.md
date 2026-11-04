# 1.0.0b16: one line to begin with, things struck by the right spell, warded creatures, the Mirror by hand

**The first round is answered** (4 November 2026): the owner's answers are in section 10, with what they change. Q1 went
further than asked: no class starts with the fireball any more (section 3a). That opens a short second round (section 11),
each with a default. It is **1.0.0b16**, ahead of the minds' showing and the rest of the art pass (now 1.0.0b17) and the
lower storey (now 1.0.0b18). The second round was answered the same day (section 11: every default, and N2 as splinters,
an ammunition of their own). Steps 1 and 2 are built (harness only).

*The brief, as first drafted (ideas only). Written on `claude/modest-knuth-gd9u51` from main at
1.0.0b15. The owner's brief: early levels have long stretches where the caster isn't doing much; fill them with things in
the crypt that want a particular spell, and only spells the caster holds, so nothing locks; each marked by motes in its
element's glow; timers, moving things, several hits and sequences are all fair. To make room for that, a second line is
given free at the Lampstand after level 4 and another after level 12. With more lines in hand, creatures may be warded
against one of them (60% early, 90% by the end of run III), shown plainly, on top of the weaknesses they have now. And the
Mirror rune goes into a person's hands: the right mouse (the pad's LT) raises it, its cooldown as now; the dream keeps it
as it is; the shrine's first curse gives a Common Mirror free. The questions are at the end.*

## 1. Where the caster stands today

On run I a caster has the fireball and, unless a Witchfinder, their class's line: two spells at most, one for a
Witchfinder. Between rooms there is walking: halls a lane wide, a skeleton or two, a door, now and then a secret's crack
or a key. Puzzles come from level 6 (even levels) and trials behind skull doors; both live in one room of the level, so
the halls between are where the lull is. What the halls hold is dressing (chains, hooks, gibbets, chandeliers, roots,
torches), all of it already drawn and none of it answering a spell.

Three facts from the code that shape everything below:

- **Spirit never locks a spell.** With too little spirit a cast goes weak, at tier 1 and free (`cast_spell`, `Shot.tier`;
  "LOW ON SPIRIT: SPELLS CAST WEAK"). So an object that wants Frost can always be answered by a caster with Frost open,
  souls or no souls. (Only the fireball's first rank costs nothing outright; every other line's first rank costs 0.2.)
- **The spear is the exception.** It spends a bone a throw (`Gear.bones`), and bones run out. Anything that wants
  Physical needs bones beside it, or another way.
- **Fire is always open** (`align_open`). Anything that wants fire is safe for everyone. *No longer, by Q1: see 3a.*

## 2. The rules every idea keeps

- **R1. Only what is held.** A thing's element is chosen when the level is made (`start_level`), from the lines open at
  that moment. On levels 1-4 that is the class's line alone (3a), so the first things are each class's own. Lines change only in her hall (the Lampstand, a vow), never mid-level, so it holds for the level.
- **R2. Never between the caster and the way down** until three lines are held, and after that only where there is a way
  round (a shortcut, not the only door; Q3 b). Keys, the exit and a boss's arch never wait on one.
- **R3. Physical brings its own bones** (Q10 a). A spear-marked thing has two or three bones on the floor within a few
  cells, and takes only one throw per hit wanted.
- **R4. Nothing in a pocket, a trial or a boss's arena** to begin with: those have their own rules. (Pockets could have
  their own later: section 4.8.)
- **R5. Seen before it is needed.** The motes are visible through fog, as the art pass's eyes are, so a thing is noticed
  from down the hall, not walked past.
- **R6. The dream can do all of it** (section 8), so the screensaver shows it off and the harness can test it.

## 3. The second and third lines (the Lampstand's gifts)

**When.** A gift is *due* after level 4 is cleared (her room before level 5) and after level 12 (before level 13). Check
it in `begin_shop` as "gifts due by the level about to be entered", not "just cleared 4": then a Portal run that starts
at 11, an old save loaded at 20 or a replay all get what they are owed on the next visit.

**How.** As her class question does (`shop_human` opens `UI_CLASS` while `RUN.cls` is 0): on entering her room with a
gift due, the Lampstand is lit and the way on is barred (the door holds, a caption: *THE LAMPSTAND IS LIT FOR YOU*). At it,
`UI_ALIGN` opens in a gift mode: only the closed lines' openings can be chosen, the opening costs nothing (`RUN.al_gift`
already marks a line opened free; a count of gifts taken, `RUN.gifts`, beside it), and Esc doesn't close it until one is
chosen. A short line on her screen says what it is: *THE DEAD ASK YOU TO CARRY ANOTHER*.

**Through death.** Death already takes the lines and the class (`death_fall`); the gifts would go with them and come again
at levels 4 and 12 on the next way down. That fits; it needs only `RUN.gifts` cleared with the rest.

**The Witchfinder** (Q1, settled otherwise than asked). Nobody starts with the fireball now, so every class begins with
one line, its own, and the gifts bring everyone to two lines by level 5 and three by level 13. The Witchfinder is no longer
a line behind.

**A line already bought** (Q2 a). A person may have opened a second line with Alignment Points before level 4. The gift then
opens another (a third), and the one after (a fourth). Only when every line is open does a gift pay the opening's points back.

**The dream.** `auto_align` already picks a second line; it takes the gift the same way, at once, at its greeting.

**As built** (step 2). `gifts_owed(level)` is one from level 5 and two from 13, by the level her room leads to; `gift_due()`
while she is owed more than `RUN.gifts`. The dream's are taken in `begin_shop` (`gift_settle`: Fire if it isn't open, then
the second line `auto_align` would buy, then any), never on a page it may not choose for. For a person the Lampstand
breathes brighter, a caption says *THE LAMPSTAND IS LIT FOR YOU: A GIFT WAITS*, and the way on holds at the door (*NOT YET*)
till it is taken. Its screen opens on the first closed line; a closed line's opening reads *A GIFT* and Enter twice takes it
(`gift_take`, `AL_GIFT`); Physical may be taken as the first gift. A gifted line is marked (`RUN.gifted`, saved after
`RUN.gifts` at the end of `LG_ALIGN`): its opening was never paid, so unbinding keeps it open and pays nothing back for it,
and if a later vow makes it the class's line the gift is owed again. With every line open a gift comes as three points.
Esc may close the screen; the door still holds. Debug: `EVENT gift`, `CRAWL_NOGIFT=1` (the dream leaves the gift for a
person, to test it).

## 3a. One line to begin with (Q1)

The owner: no class gets fire by default; a caster specs into other lines if they choose, and the class's gifts apply
mostly to its own line, which evens the six out. Today's gifts already do, but for two general ones (the Sin-Eater's twenty
more in the vessel, the Headsman's ten bones and tenth of armour), which can stay. What changes in the code:

- **`align_open`** loses `e == EL_FIRE`; Fire's opening costs 3 like the others (`ALIGN[EL_FIRE].n[ND_OPEN].cost`, today
  0). The Witchfinder keeps their three Fire points (`RUN.cls_pts`).
- **The free first rank.** Today only the fireball's first rank costs nothing (`spell_cost`); the others cost 0.2 spirit.
  To keep the classes even, the class's own line costs nothing at rank 1, whichever it is (N1).
- **A class before the first step down.** With no fireball, a caster without a class has no spell. Her question already
  opens at once while `RUN.cls` is 0 (`shop_human`), and the dream takes one at random in `begin_shop`; the way on must
  also stay barred until it is answered, for a person, after a death and for an old page with no class.
- **Death** (`death_fall`) takes the lines and the class as now, but no longer leaves Fire open at rank 1: the next life
  is asked what it was, and starts with that line. Her claim's spell "a tier down, the Fire Bolt to the fireball, never
  below" becomes the class's line, never below rank 1.
- **Old pages.** A page whose Fire nodes were bought on top of the free opening keeps Fire open, marked as a gift opening
  (`RUN.al_gift`, as `class_take` does for a class given up), so nothing bought is lost. A page with no class is asked.
- **Splinters** (N2, the owner's). A Headsman, or anyone with only Physical, out of bones had nothing to cast ("NO BONES
  FOR THE SPEAR"); before, the fireball was always there. Splinters are the line's second ammunition: thrown when the bones
  are gone, eight at most, one growing back every six seconds (`SPLINTER_MAX`, `SPLINTER_T`), thrown at 1.6 times the
  spear's interval (`cast_cycle`), striking at two fifths of the spear's blow at its rank (`SPLINTER_DMG`), through nothing
  and stopped by a shield, with no Impale or Ossified (`SHOT_SPLINTER`). Bones stay the way to the line's strength. The
  spear's place on the status bar shows the splinters left, under a sliver, while the bones are out. The Phalanx and the
  Ballista without bones throw a splinter. Not saved: a load starts with all eight.
- **The Wraith** (N3 a). Only fire, Soul Drain, Hellfire and wisps drove the ordinary Wraith back (`damage_mob`,
  `wraith_back`). Now any spell drives it back for 3 s (`wraith_back_for`); fire, spirit and wisps for 5 s as before. The
  dream turns fire on it if it has fire, else Soul Drain, else its own line.
- **Physical shut till the first gift** (the owner, 4 November, after the first runs: the Headsman, with only the spear,
  6 bones and splinters, cleared 6 levels and died 13 times on four seeds where 1.0.0b15's cleared 11 with none; 40 bones at
  the start still left 3 deaths). Her question offers five pasts, the Headsman's socket shut with *SHUT TILL THE
  LAMPSTAND'S FIRST GIFT* (`class_offered`); the Lampstand won't sell Physical's opening before it (`phys_locked` in
  `align_ready`); the dream and a lending choose among the five (`class_random`). A Headsman already sworn on an old page
  keeps their past. The gifts taken are `RUN.gifts`, a byte at the end of `LG_ALIGN` (`LG_ALIGN_BYTES`; an older page reads
  as none), cleared by a death.
- **The Plague Doctor's poison** (the owner, 4 November, after step 1's runs: the class's flask alone, a slow lob that hurts
  over time, died 35 times on eight seeds where 1.0.0b15's died 6). Their poison now bites half again as hard each tick
  (`PLAGUE_BITE` in `mob_poison`), and still lasts a fifth longer; everyone else's is as it was. The class's gift says so.
- **A line lent** (N4, N5). Her question already reopens every frame in her room while `RUN.cls` is 0, so a person can't
  walk on without a class. The dream may not choose for a page it reads from the ledger (`LEDGER.kept`), and a run met in
  the crypt with no class has no spell at all; for those `line_lend` lends a line at random (`LEDGER.lent`, never saved)
  till a class is taken, and the Lampstand marks it *LENT, TILL YOU SAY WHAT YOU WERE*.
- **The dream's fallbacks.** It reaches for the fireball in six places: `choose_spell`'s default and its Wraith rule, tearing
  at the Wandering Spider's web, the probe through an ambush's door, and the spell it falls back to when one is closed or
  the BFS9000 can't be paid for (`G.spell`). Each becomes "the caster's own line" (a small `own_spell()`).
- **Fire's own things stay fire's.** Explosions and splashes, Firestarter, the double harm to the spider's web, firedamp set
  off, the knight's breastplate, the imps' and the Wraith Unbound's weaknesses: all keep wanting fire. A caster without it
  meets them as the other elements do today.
- **The free fireball in the words.** The status bar, the controls card, the Codex, the class pages and the release notes
  say the fireball is everyone's in a few places; they change with it.
- **Balance.** The dream will be weaker on levels 1-4 with one line, and the spear's class weakest of all. Sixteen seeds from
  level 1 against main, by class (`CRAWL_CLASS`), before anything else in this plan is built on it.

## 4. Things that want a spell

### 4.1 How they look

Each carries the element's glyph (the six the target's readout uses: a flame, a flake, a bolt, a drop, a wisp, a bone),
carved or hung on it, and **motes in the element's ramp** (`R_FLAME`, `R_GLOW`, `R_GOLD`, `R_MOSS`, `R_PURPLE`, `R_BONE`)
drawn *in* towards it, slowly, as if it were breathing them: it wants that. Lit through fog and dark, as the eyes are.

- **Hit by the right spell:** a flare of its colour, a chime, a pip lit on a little ring round the glyph when it wants
  several hits.
- **Hit by the wrong one:** the dull ring a boss's shell gives (`G.bring`), the shot's sparks thrown back, nothing lit. The
  first time a level, a caption: *IT WANTS FROST*.
- **A timer:** the ring of pips drains back one by one if left; an hourglass glyph as the trials have when it is a race.
- **Done:** the motes stop, the glyph goes dark, the thing does what it does.

The same colours mark warded creatures (section 6), so the two languages must not cross: a thing *draws motes in*; a
warded creature *wears a hard rim* of the colour and its glyph is barred.

### 4.2 Single spell, standing still (levels 1-4: the class's line)

The first ones should be quick and generous: one or two hits, something in it every time. With one line to begin with
(3a), the carriers (the coffer, the cage, the chandelier) wear the class's glyph, and the class's own (8-11, and the sconces
and the rimed door for fire) come where they can.

1. **Cold sconces.** Empty torch brackets along a long hall, marked with fire. Lit, they stay lit: the hall is lighter (the
   shade and the Lightless care, deeper down), and lighting every one on a level opens a niche with gold. Only for a caster
   with fire, now (3a); the Witchfinder's first.
2. **The bound coffer.** A chest wrapped in chains, a glyph on the lock. Its element's spell breaks a chain a hit; one to
   three chains by what is inside.
3. **The hanging cage.** A gibbet (already dressing) on a chain over the hall, marked with the bolt or the bone: strike the
   chain and it falls and bursts open (bones, coin, now and then a potion).
4. **The chandelier.** Already in rooms. Marked, its rope can be cut: it falls on whatever is under it. The first thing
   that turns a fight: draw them under it, then cut. (A frost mark: the rope freezes and snaps.)
5. **Webbed passages.** A side passage or alcove sheeted in web, marked with fire. Burnt, it opens on a small find. Run II
   already has webs from its sixth level; these are the friendly kind.
6. **Votive racks.** A rack of dead candles in a niche; fire lights them, all at once or a candle a hit. A lit rack is a
   small blessing for the level (a tenth more experience while it burns, a minute).
7. **The rimed door.** A side door frozen shut, frost in the cracks, motes of fire drawn to it: melt it. (Frost's mirror:
   **the smoking door**, a door left hot by something behind, wants frost.)
8. **A bone barricade.** A heap of the Order's dead across a short cut, marked with the bone: the spear brings it down (R3).
   The Headsman's first one.
9. **The withered vine.** Roots grown over a niche (roots are already dressing), marked with the drop: poison withers them
   in a few seconds. The Plague Doctor's first.
10. **The bound soul.** A lantern hung in a niche with something moving in it, marked with the wisp: Soul Drain draws it
    out; it thanks you with a few souls, or (later) goes as a wisp to the nearest shut secret. The Sin-Eater's first.
11. **The galvanic coil.** An iron winding on a dead lamp or a stuck gate winch, marked with the bolt: lightning wakes it,
    and the lamp or the winch runs. The Galvanist's first.

Each class's first thing comes on level 2 or 3, so every caster sees early that their own line has a use outside a fight.

### 4.3 Several hits, timers, and moving things (from level 5: two or three lines)

12. **The portcullis winch.** Lightning charges it; the gate rises and holds for eight seconds, a short cut or a vault of
    gold behind. Charge it again from the far side to come back.
13. **The frozen bridge.** Run II's water (a cistern, a canal) with a small island or the far side's alcove: frost freezes a
    path across for twenty seconds, the ice cracking audibly before it goes. Fall in and you wade back, no harm.
14. **The steam vent.** A hall filled with scalding steam from a cracked pipe or a vent (a fixed hazard): frost seals it for
    fifteen seconds. Or go the long way.
15. **The dying hearth.** A hearth in a room whose door opens slowly while it burns; a draught puts it out every ten
    seconds. Keep it fed with fire until the door is up.
16. **Wandering corpse-lights.** A pale light drifting about a room or along a hall, marked with an element; hit it and it
    breaks into motes (souls, a little experience). It shies off if you come close. Lightning, which strikes at once, is
    the easy answer; the fireball has to lead it.
17. **The swinging censer.** A censer on a long chain, swinging across a hall; marked, it can be struck only at the bottom
    of its swing, where the glyph turns to face you. Struck three times, it falls and its coals spill (a short fire on the
    floor: good against what follows you).
18. **Rolling skulls.** A skull rolling down a sloping hall towards a pit, marked; hit it before it drops. A little gold in
    each.
19. **The heat-and-shatter seal.** A stone seal over a niche: fire heats it (it glows, the glow fading over four seconds),
    and frost while it glows shatters it. Two spells, one thing, and a reason to switch fast. It also teaches Frost's
    Shatter.
20. **Bile in the cistern.** A grate (`T_GRATE`) with something glinting under the water beyond it: the poison flask lobs
    over the grate (it arcs; nothing else does), and the bile eats the lock. Teaches the flask's arc.
21. **Three bells in a row.** Three small bells hung one behind another down a hall: the spear pierces all three in one
    throw, and they ring together. Any spell rings them one at a time, and they must all ring within two seconds: easy for
    the spear and lightning's Arc, a race for the rest.

### 4.4 Sequences (from level 13: three lines or more)

22. **The tablet's order.** A carved tablet shows three glyphs; three things down the hall each want their own element,
    and must be struck in the tablet's order. A wrong one resets them, with a groan from the stone.
23. **The turning reliquary.** A reliquary that turns slowly between three faces, each with its element's glyph; strike the
    face towards you with that element. Three right in a row. Switching spells in time is the trick.
24. **The changing ward.** A sealed door whose glyph changes after each right hit, at random among the caster's lines:
    five hits to open, switching every time.
25. **The relay.** A lamp struck with fire passes a flame down a line of lamps to a gate; a frozen one in the line stops it
    until thawed; a dead coil in the line wants lightning. Read the line, then answer each break.

### 4.5 Things for fights

Not puzzles at all: ways the crypt helps the one who looks about. These are the best answer to the lull, because they
make the next fight a different fight.

26. **Lamp oil.** Casks or a spilt pool, marked with fire: lit, a burning floor for eight seconds. Draw them across it.
27. **Standing water.** Run II's water, marked with the bolt when creatures stand in it: lightning into it stuns everything
    wet for a second.
28. **Firedamp.** The stope's gas (`HZ_DAMP`) already waits for fire; mark it so a person knows, and let a blast set it off
    under creatures.
29. **Loose masonry.** A cracked pillar or lintel, marked with the bone: the spear brings it down, rubble crushing what is
    under it and blocking the hall behind you (a pursuer has to go round).
30. **The ice floor.** A stretch of floor frost can rime; creatures crossing it slide and slow. Lasts twenty seconds.
31. **The poison font.** A dry font that a flask fills with bile; what comes near it sickens (the dead included: D3's
    hazards already hurt the dead).
32. **The Order's drum.** A war drum in an ossuary, marked with the wisp: Soul Drain silences it, and the file (PLAN-ai's
    packs) loses its order for half a minute.

### 4.6 By run

Each run's look can carry its own:

- **Run I, the Order's crypts:** sconces, votive racks, bound coffers, chandeliers, censers, the drum, bone barricades.
- **Run II, the drowned catacombs:** frozen bridges, the canal shocked, flasks over grates, webbed niches (burnt), sluice
  winches charged.
- **Run III, the charnel mine:** timber props burnt to drop a short cut's rubble, firedamp, lodestones charged to lift ore
  carts, ore carts on rails as moving targets (a cart with gold in it rolls past: break its wheel).
- **Run IV, the black glass:** prisms that split one bolt into three (a sequence with one cast), magma crust frosted to walk
  on, obsidian shattered by heat-and-frost, geodes cracked by the spear for gems.

### 4.7 What they give

Small and steady, so they are worth stopping for but never the reason to stay: a little gold, bones (the spear's
caster loves these), souls as motes, a potion now and then, a few points of experience, a short way round. Kept out of the
level's treasure count (as `Item.secret` is), so the dream's "most of the treasure" and the reckoning don't change. A
level's last thing done could give a Cursed Coin one level in five, as every secret found does.

### 4.8 Later, in pockets

The Backrooms and Severance could have their own: a fuse box lightning wakes (the lights come on), a vending machine
frozen for a can (a potion), a fire alarm set off by fire that brings every creature in the pocket to it. Kept out of the
first steps (R4).

## 5. Where they go: filling the stretches, not the rooms

The lull is in the walking, so placement should follow the walking.

- **An interest budget along the way.** At the end of `gen_level`, walk the lanes from the start room to the exit (the
  trunk, as `mz_widths` already finds it) and to each key. Count steps since the last thing of interest: a room with
  creatures in it, a door, a secret's crack, a puzzle's or trial's door, a key. Wherever a stretch passes twenty-five steps
  or so with nothing, put a thing from 4.2-4.5 in it, in a hall's side or a niche off it, facing the way the caster comes.
  A level holds three to six. Most in halls; few in rooms.
- **Early levels lean on it most.** Levels 1-4 have the fewest creatures and the longest walks; they get the most things,
  and the simplest.
- **One per class early.** Of the first few things on levels 2-4, at least one is the class's own line (4.2's 8-11).
- **The lull itself.** A simple watch in the game: if a person has walked thirty seconds without a fight, a find, a thing
  struck or a creature seen, the level offers something: a corpse-light drifts into the hall ahead, a cold sconce lights
  itself for a moment further on and goes out (*it wants fire*), a far-off bell. PLAN-ai's budget could keep a creature or
  two back for this, to come by as a wanderer when a lull runs long. Never twice in a minute; never the dream's.

## 6. Warded creatures

**What.** Some creatures are warded against one line the caster has open: that element does a small share of its damage.
On top of the kinds' weaknesses (`KINDS[].weak[]`), which stay as they are.

**How strongly.** 60% resisted on level 5, rising evenly to 90% on level 30 (the end of run III) and staying there in run
IV: `ward = 0.6 + 0.3 x clamp((level - 5) / 25, 0, 1)`. In `weak_el` that is the kind's weakness times (1 - ward) for the
warded element, so `hit_mult`, `choose_spell` and the target's readout all see it without change.

**Which element.** From the lines the caster has open, the one the kind is already least weak to (its nature: a gargoyle
warded against fire, an imp against fire, a shade against frost), ties broken at random, and:

- never the line it is weakest to (a shambler is never warded against fire);
- never every line the caster holds: at least one held line is not warded, always;
- one element only until the caster has three lines (level 13); from then a rank IV or black creature may carry two;
- never the class's own line while the caster has only two (levels 5-12): the second line is the one a ward teaches.
  From three lines, any.

**How often.** None before level 5 (one line and fire is too little to switch between). Then about one creature in six on
run I's levels 5-9, one in four on run II, one in three on III and IV. Packs share one ward (PLAN-ai's packs): a whole file
warded against fire is a lesson in switching. Leaders, rank III and up and the gilded more often.

**How it shows.** A hard rim of the element's colour round the creature's outline (as `DR_EDGE` draws a boss's shell), the
element's glyph barred over its head as it comes near, a dull ring and thrown-back sparks when the warded spell hits it (the
boss's `G.bring`), and the target's readout gives that element a barred glyph on the strong side. The first time a level, a
caption: *WARDED AGAINST FROST*. On the bestiary's page, a line: *SOME COME WARDED*.

**Breaking it** (Q7 a). A ward cracks after a fifth of the creature's health in damage of its own element, with a sound
of breaking glass and the rim gone, dropping it to the kind's own weakness: then nothing is ever locked, only slower.

**What it doesn't touch.** Bosses (their own rules), their adds (Q8 a), spectres and spirits, allies, a trial's creatures. Nothing saved: a ward belongs to the creature for its level.

**Balance.** The dream sees wards through `hit_mult`, so it switches spells as it should; the cost is time, which shows in
the runs. Eight seeds is too few here (CLAUDE.md's step-2 note); compare sixteen against `CRAWL_WARD=0`.

## 7. The Mirror by hand

**Now.** `def_trigger` fires it when two missiles will land within 1.5 s or a wall skull charges; the veil
(`G.mirror_t`, 3 + half a second a tier, by Spell Effect) sends back missiles within 0.7 of the caster; its cooldown is
`def_cooldown` (about 40 s Common, 16 s Exquisite).

**For a person.** It never fires by itself. The right mouse (the pad's LT, which is already the right mouse) raises it;
the cooldown and the socket's filling line are as now; pressed early it says *THE MIRROR IN 12 S* and knocks, as the boss
abilities do. The hand comes up palm out, silvered, while it holds. Three ways it could feel (Q4; the owner: **(b)**, the parry):

- **(a) The veil by hand.** The same veil, the same length, raised when you choose. Simplest; the skill is in the timing.
- **(b) A parry.** A short true mirror (0.6 s) that sends back at twice the strength and homing (as the Exquisite one
  does), then the ordinary veil for the rest. Rewards a good eye; a whiff costs the whole cooldown.
- **(c) Held.** Up while the button is held, draining a store that fills over the cooldown.

**The right mouse is taken.** It already uses the readied boss ability (`use_readied`; Q readies the next). The Mirror
joins them (Q5 a): it is readied by default until a boss's gift is won, and Q turns between it and the gifts
(`ready_next`); its socket sits first among theirs.

**The dream** keeps `def_trigger` as it is (`G.player` decides which).

**The shrine's first curse.** `shrine_curse` gives a Common Mirror free with the shrine's first rank, with a pop-up (*THE
SHRINE GIVES YOU A MIRROR*) and a hint (*RIGHT MOUSE RAISES IT*, through `pk`, so the pad's says LT). The shrine stays
cursed through death and the runes stay, so it comes once a run of the ledger. If a Mirror is already held: nothing more (Q6 a). Skeletons throw from level 1 and wall skulls charge from 3, so it has work at once.

**The other runes** stay automatic. Blink, the Ward and Time Slip by hand are for later (Q9), once the Mirror is played.

## 8. The dream

- It strikes things in its way (4.2-4.5) as it strikes secrets: from `use_spot`, when the thing is within a few cells of
  its lane and it has the element; a few seconds each, never in a fight; timers and sequences only where they are short.
  Section 4.5's are for a person: the dream doesn't plan fights.
- It takes the gifts at once (section 3), keeps its Mirror automatic, and switches spells for wards through `hit_mult`.
- Debug: `CRAWL_THINGS2=k` (a thing of kind k ahead on each level), `CRAWL_WARD=0/1/2` (none, as made, every creature
  warded), `EVENT struck`, `thing_done`, `warded`, `ward_cracked`, `mirror_raised`, `mirror_turned`, `gift`.

## 9. The order

1. One line to begin with (3a), with the bare spear (N2) and the Wraith (N3), and its balance runs by class.
2. The gifts at the Lampstand (section 3).
3. The Mirror by hand (the parry, then the veil), its place among the right mouse's gifts, and the shrine's free Mirror.
4. The look of a marked thing (motes, glyph, pips, the dull ring), on a sheet first, as the art has gone.
5. Section 4.2's things and the stretch budget (section 5), run I only; frames from levels 1-4 for the owner.
6. Section 4.3's, and runs II-IV's flavours.
7. Warded creatures, with sixteen-seed runs against `CRAWL_WARD=0`.
8. Sequences (4.4) and the fight things (4.5).
9. The lull's watch.
10. The checks: balance against 1.0.0b15, ASan, the README's and the page's words.

## 10. Questions (the first round, answered 4 November)

The owner's answers follow each, in bold.

- **Q1. The Witchfinder.** (a) two gifts at level 4, so everyone has three lines by level 5; (b) one, a line behind the
  others. *Recommended: (a).*
  **The owner: neither. No class starts with fire; each begins with its own line, and its gifts apply mostly to it.**
  Section 3a.
- **Q2. A gift with a line already bought.** (a) it opens another line; (b) the points back. *Recommended: (a); (b) only
  when every line is open.*
  **The owner: (a).**
- **Q3. Do things ever stand between the caster and the way down?** (a) never; (b) only from level 13, with a way round.
  *Recommended: (b).*
  **The owner: (b).**
- **Q4. The Mirror's feel.** (a) the veil by hand; (b) a parry and then the veil; (c) held. *Recommended: (b), with (a) as
  the first step.*
  **The owner: (b).**
- **Q5. The right mouse.** (a) the Mirror joins the boss gifts, Q turns between them; (b) the Mirror alone on the right
  mouse, the gifts on another key. *Recommended: (a).*
  **The owner: (a).**
- **Q6. The free Mirror when one is held.** (a) nothing; (b) the next pull a coin cheaper. *Recommended: (a).*
  **The owner: (a).**
- **Q7. Can a ward crack?** (a) yes, after a fifth of its health in its own element; (b) never. *Recommended: (a).*
  **The owner: (a).**
- **Q8. Wards on a boss's adds?** (a) no; (b) from run II. *Recommended: (a) for now.*
  **The owner: (a).**
- **Q9. The other runes by hand, later?** Blink, the Ward, Time Slip. *Recommended: ask again after the Mirror is played.*
  **The owner: later.**
- **Q10. The spear's things without bones.** (a) bones beside each; (b) the fireball works too, at three times the hits.
  *Recommended: both.*
  **The owner: (a).** (The fireball's fallback went with Q1 anyway.)
- **Q11. How many things a level, and do they count?** Three to six, more on levels 1-4; outside the treasure count, a
  coin one level in five for doing them all. *Recommended as written.*
  **The owner: as recommended.**
- **Q12. Which release.** After 1.0.0b16 (the minds' showing and the rest of the art pass), or the gifts and the Mirror
  first as a small one.
  **The owner: first; the others move on a number.** This is 1.0.0b16, the minds' showing and the rest of the art
  pass 1.0.0b17, the lower storey 1.0.0b18.

## 11. The second round (from Q1), answered 4 November

The owner took every default but N2, where splinters became an ammunition of their own (3a).

- **N1. The class's first rank free.** (a\*) The class's own line costs nothing at rank 1, as the fireball did; (b) every
  first rank costs 0.2, the fireball's too.
  **The owner: (a).**
- **N2. The spear with no bones.** (a\*) It goes weak, as a spell without spirit does: a splinter at tier 1, no pierce, no
  bone spent; (b) bones grow back, one every six seconds under three; (c) as now, nothing.
  **The owner: splinters, but as their own ammunition: thrown moderately slowly, light damage, growing back as (b) says,
  eight at most; bones the way to the line's full strength.**
- **N3. What drives the Wraith back.** (a\*) Any spell, 3 s; fire, spirit and wisps 5 s as now; (b) as now (fire, spirit,
  wisps, Dread and the blessing only).
  **The owner: (a).**
- **N4. The way on before a class is chosen.** (a\*) Barred until her question is answered, for anyone (it can be closed and
  come back to, but not walked past).
  **The owner: (a).**
- **N5. The fireball's first rank for old pages.** (a\*) A page that only ever had the free fireball loses it and is asked
  for a class if it has none; what was bought in Fire stays open as a gift.
  **The owner: (a).**
