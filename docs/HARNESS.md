# The harness, its debug switches and the long runs

From `CLAUDE.md`'s *Building and testing*, moved here word for word when that file was slimmed (docs/PLAN-tidy.md, step 6).
`CLAUDE.md` keeps the commands that build things. The save's format, which stood between the long runs and the
sanitizers, is `docs/SAVES.md`.

The harness (`tools/host.c`):

```sh
build/host shot  <haunt|crawl> <t1,t2,...> <prefix> [scale]   # PNGs; default scale 3 for 320x200, 1 for 720x400
build/host wav   <scene> <seconds> out.wav [sfx_id@time ...]   # sfx_id: the SFX_* number (SFX_SPLASH is 61)
build/host dump  <scene> <seconds> <prefix> <every_n_s>       # frames and audio together
build/host bench <scene> <seconds>                            # ms per frame, every frame drawn
build/host music <piece> <seconds> out.wav [event@t ...]    # a piece alone (crypts, catacombs, hollows, deep, merchant,
                                                              # perfect, title, special, ending, lamp, office, ossuary, menu1-4)
                                                              # through fight0-3, frail/well, menu/title/lamp/back,
                                                              # trial/untrial, place0-2, run1-4 (docs/PLAN-music.md); prints
                                                              # any score faults; HOST_DRIPS=0, HOST_MUTE=mask (PLAN-music-work)
build/host apl   <scene> <seconds>                            # average picture level; fast (draws twice a second), good for long sims;
                                                              # plays HOST_INPUT (it didn't before step 6 of PLAN-ui), but writes no slots
```

Environment for the harness: `HOST_INPUT="1:+H;4.3:-H;5:+W;7:-W;8:m400,0;9:+MOUSE1;9.2:-MOUSE1"`
(press `+`, release `-`, mouse counts `m`, wheel `w`, typed letters and digits `t`, as `6:tWORD`; key names
as in `core.h` without `KEY_`, so `CONSOLE`, `BACK`, `PGUP`, `PGDN` too),
`HOST_LOAD=file` (the progress to start from: a run, or the page it names; stands in for the registry),
`HOST_DRAWN=file` (a drawn level from the world editor's export, read as its PLAY reads it: it takes the place of its level's drawing and a new run begins on that level; a text it cannot read stops the harness with the reason and its line; docs/PLAN-mapedit.md, part 3), `HOST_SAVE=file` (the progress, each time the game writes it), `HOST_STAGE=WxH` (with `shot`: touch's stage of that size too, each frame's written beside it as `_stage.png`; no fingers, so the stones as they stand), `HOST_SLOTS=dir` (the ledger's pages as
`slot1.bin`..`slot3.bin`, the dead's mark as `mark.bin`, the pad's layout as `prefs.bin`), `BOTH=1` (scene rotation), `STAY=n`. A game
controller in `HOST_INPUT`: buttons `+PAD_A` ... `-PAD_RT` (names as `PAD_*` in `core.h`), sticks `aLX0.8`,
`aLY1`, `aRX-0.5`, `aRY0` (y up); any of them makes a pad plugged in. The harness prints `RUMBLE lo hi`
and `RUMBLE off` as the motors start and stop. `build/host_debug drawncheck <export> <n>` reads an export and compares it with `DRAWN[n - 1]` field for field ("the same, field for field", or the first part that differs). `build/host_debug drawnlights <export>` prints the light props the generator hangs or stands on that drawing, one a line (`DL_CHANDELIER 14 3`: the style and its cell in the drawing; `crawl_drawn_lights`, docs/PLAN-lights.md); the debug build's `CRAWL_LIGHTLIST=1` prints the same as each drawn level is laid in play (`LIGHTLIST level n: style x y`), and `CRAWL_LIGHTLISTAT=s` asks for the listing once, at scene time s, in the middle of a game (nothing after it may move). `CRAWL_THREELEFT=1` shows three left (the cold flames and the breath) on every level (`-1` never, for frames of the run's own flames), `CRAWL_THREECOL=k` the k-th colour on its sheet (0 the ghost ramp, the default; 1 glow, 2 bone, 3 violet); `CRAWL_TPPROP=-2` (with `CRAWL_STAGE`) stands you before a torch on a wall, `CRAWL_TPPROPN=n` the n-th. `CRAWL_LIGHTLOOK=0` draws the cage lantern's light plain, without its spots, and the sanctuary lamp's without its red (the lights' looks, drawing only: `lk_*` in `draw/floor.c`); `python3 -I tools/dapple_sheet.py` makes the dapple's sheet from the harness. `M` is the map's key (1.0.0b9).

Debug build switches (`CRAWL_DEBUG`): `CRAWL_SEED=n` (seeds are OR-ed with 1, so 2 and 3 are the
same), `CRAWL_HASCOMPASS=1` (the compass, already bought), `CRAWL_COMPASSTEST=1` (at a level's start, the bearing from eight places round the exit and each key lying: `EVENT compass_selftest cases n bad n`), `CRAWL_COMPASSLOG=1` (with the compass, what its readout says every five seconds: `EVENT compass LABEL | NUM bear eased`) and `CRAWL_SAVETEST=1` (at a level's start, a page packed and read back for each compass it can hold; prints `EVENT save_selftest cases n bad n`), `CRAWL_LEVEL=n` (start deeper; you start at the merchant), `CRAWL_GEAR=t` (every spell at
tier t, every rune at the quality an old save's tier t becomes, and an Uncommon piece of level 2t in every
slot), `CRAWL_RUNES=q` (every rune at quality q, 1 Common to 5 Exquisite), `CRAWL_COINS=n` (Cursed Coins),
`CRAWL_SHRINE=r` (the Rune Shrine's rank, 0 sanctified to 5), `CRAWL_BAG=q` (a bag of quality q, all
but one place filled with spares), `CRAWL_ABIL=a` (every worn piece with ability a, the `AB_*` number), `CRAWL_GOLD`, `CRAWL_SOULS`, `CRAWL_MAP=1` (prints the map, the plan and a lane
reachability check), `CRAWL_MAPAT=t`, `CRAWL_TPSECRET=n` / `CRAWL_TPEXIT=1` / `CRAWL_TPALTAR=1` (stand in front of a secret,
the exit or the Black Altar once you take control; an altar in a locked vault has the vault's door
unlocked for you), `CRAWL_TPDOOR=k` (open the first chained room and
stand before its doorway; 2 on: into that link of the chain, facing the way on; with `CRAWL_TPVIS=1`, in
the last link, facing its visportal; with `CRAWL_TPTHING=k`, a few steps from the k-th thing out of
place), `CRAWL_TAKEALL=1`
(everything in a pocket picked up as you enter), `CRAWL_KILLAT=s` (struck down at scene time s), `CRAWL_BLESS=1` (the altar's blessing on every level), `CRAWL_WISP=1` (a free exit wisp as each level opens; 2 to 4: to the
gold, silver or crimson key), `CRAWL_FOLLOWWISP=1` (a player that walks the way the wisp went, from
wherever it stands, and presses E at the exit and at secrets: a test that it leads out, and that an
altar's wisp finds the secrets; with the altar's wisp it goes into each secret and out, presses E at the
wisp, follows it home and sends it on from the altar's menu; it can't be hurt), `CRAWL_SHOWHAND`, `CRAWL_BFSANY`,
`CRAWL_TPBOSS=1` (on a boss level, in its antechamber facing the arch once you take control; 2: the autopilot
put there as the level opens, already leaving), `CRAWL_BOSSHP=f` (a boss's health times f), `CRAWL_OMEN=s` (the
level's omens on any level, every s seconds, the first at once), `CRAWL_TOTEM=dddd` (each boss ability's rank, a digit
each in boss order, and that boss counted beaten), `CRAWL_SCRIP=n` (n scrip of each boss, all four counted beaten),
`CRAWL_PEDS=n` (the merchant's pedestals), `CRAWL_TPCAT=1` (before the merchant herself), `CRAWL_TPCOMPASS=1` (before the compass, once you take control), `CRAWL_TPSTONE=1` (before the plot's newest stone), `CRAWL_STONE=n` (n stones in the plot, from what is held), `CRAWL_TPSHRINE=1`
(before the Rune Shrine), `CRAWL_LETGO=1` (the full bag's question as you take over in her room, with a bag), `CRAWL_HOLDCARD=1` (a level's card and a boss's held
up once shown), `CRAWL_CLAIM=1` (her claim's card as you take over, its words only, nothing taken), `CRAWL_TOAST=1` (a pop-up as you take over), `CRAWL_SEENALL=1` (every kind met and some slain, for the bestiary), `CRAWL_TPLECTERN=1` (before the Compendium), `CRAWL_TPREPO=1` (before the Repository), `CRAWL_TPTOTEM=1` (in the merchant's back room facing the Hexed Totem once you take control), `CRAWL_TPPORTAL=1` (in the
grave plot facing the Accursed Portal), `CRAWL_RUNTOP=n` (the runs it opens: 0 only run I), `CRAWL_BSTEP=s` (every
challenge at step s), `CRAWL_GOALS=1`
(the autopilot's goal at every step, as `GOAL` lines),
`CRAWL_HASMAP=1` (the map already bought), `CRAWL_LEARNT=1` (every spell's effect learnt on every kind, for the target's
readout), `CRAWL_XP=n` (n experience banked as a person takes over: the autopilot never ascends a person's run),
`CRAWL_LUCK=x` (Luck at x%), `CRAWL_CORNER=b` (the cornerstones held, a bit each: 1 Second Wind, 2 Premonition, 4
Cartographer, 8 Lantern of the Dead, 16 Grave Sense),
`CRAWL_MEETN=n` (with `CRAWL_MEET`, n of them spread about the line of sight), `CRAWL_ALIGN=s` (1.0.0b11: a letter
for each line, Fire, Frost, Lightning, Poison, Spirit, Physical: 0 closed, 1 open, 2 to its Insight, 3 to its ability,
a-c path A's nodes in order, x-z path B's, so `c` is A's ultimate and `z` B's), `CRAWL_ALPTS=n` (Alignment Points),
`CRAWL_TPLAMP=1` (before the Lampstand), `CRAWL_SPECTRES=n` (n spectres a level), `CRAWL_CLASS=k` (the class, 1-6 by
element, 0 to be asked), `CRAWL_ATONE=n` (the Level of Atonement, nothing bought with it), `CRAWL_CORRUPT=1` (every level
corrupted; 2 its secrets shown), `CRAWL_TPBELL=1` / `CRAWL_TPFONT=1` (before the Dead Bell or the Weeping Font),
`CRAWL_CLEARED=n` (levels 1-n counted cleared, for replays), `CRAWL_XPBANK=n` (experience banked at a fresh start; the
dream ascends and aligns on it at its first greeting, for deep starts), `CRAWL_LATCH=1` (a rank II bat latched five seconds into each
level); `CRAWL_ALIGN`, `CRAWL_SOULS`, `CRAWL_CLASS`, `CRAWL_HASMAP` and `CRAWL_LEARNT`
also apply over a run loaded with `HOST_LOAD`,
`CRAWL_TPROOM=k` (in a corner of the first room of type k, looking across it: run I's 1 ossuary, 2 burial
vault, 3 chapter house, 4 knights' vault, 5 undercroft; run II's 8 saints' catacomb, 9 cistern, 10 silted nave,
11 canal, 12 spinners' gallery; run III's 13 timbered gallery, 14 stope, 15 charnel pit, 16 fossil hall, 17 the
chapel; run IV's 18 geode, 19 glass hall, 20 cinder field, 21 magma vent, 22 the throne), `CRAWL_RANK=r` (every creature at rank r), `CRAWL_WRAITH=s`
(the Wraith at s seconds), `CRAWL_MEET=k`
(one creature of kind k, the `MOB_*` number, a few cells ahead of the caster five seconds into each level,
and into each pocket). The mice (docs/PLAN-mice.md): `CRAWL_MICE=0` (none), `CRAWL_MICESTAGE=n` (the small ones at size n, 0-6,
whatever the level, their holes too), `CRAWL_TPMICE=1` (three of them preening by the nearest warm light, you three steps off facing
it, once you take control; 2: two cells before a hole, a mouse looking out of it ten seconds later); `EVENT mice level n stage s mice
m holes h lights l`, `EVENT mouse_tell i at x,y to mouse|rat j via h1,h2` (the holes it goes through, -1 on foot), `EVENT
mouse_told_rat`, `EVENT mouse_slain`, `EVENT mouse_gaveup`, `EVENT mouse_stall` (never, since it was fixed), `EVENT mice_hide boss`.
The crypt'"'"'s small life (docs/PLAN-ambience.md): `CRAWL_LIFE=0` (none), `CRAWL_TPLIFE=1` (three cells from the first puddle, facing it, a few
crawlers set about it; once you take control, the controls card put away), `CRAWL_LIFEMOVE=n`, `CRAWL_LIFESOUND=n` (the two settings, 0 off, 1 reduced, 2 on), `CRAWL_LIFELIT=1` (a light of the harness'"'"'s own on you, to see
them by: they are a dark crypt'"'"'s); `EVENT life level n puddles p beetles b lice l toads t`, `life drip`, `life print wet n` (a wet
print left), `life flee beetle|louse d` (it ran from you), `life crack` (it slipped into the stone), `life crunch` (stepped on), `life hop
toad d`, `life bat drops d`, `life torch gutters`; `CRAWL_TPLIFE=3` (before a light's moths), `4` (before a bat roost), `5` (before eyes in a niche, a little off them), `6` (before a sleeper), `7` (before a run II saint); `life niche eyes open|shut`, `life sleeper settles`, `life hanging swings`, `life saint watches`; the level line also counts moths, flies, bats, roosts, torches. The settings: `g_life_move`, `g_life_sound` (0 off, 1 reduced, 2 on), from the config and the registry; the harness runs both on.
The charnel rat is `CRAWL_MEET=23`. The wall spiders (its section 8): `CRAWL_SKIT=0` (none), `CRAWL_TPSKIT=1` (two cells from one on
its wall, once you take control; `CRAWL_SKITGO=1` it crawls off at once), `CRAWL_TPSKIT=2` (one put a little short of a wall's edge,
crawling slowly round it and back, you where both faces show; it minds nothing, and the controls card is put away), `CRAWL_TPSKIT=3`
(the same into a corner), `CRAWL_TPSKIT=4` (one crawling at a picture on the wall, at its height, and round it; `5` at three pictures
together), `CRAWL_PICS=1` (every picture on the stone and its keeping, each time it is traced); `EVENT skitters level n count c faces
f pictures p`, `skitter_noway` (a mark with no way to it round the pictures), `skitter_run`, `skitter_told`, `skitter_web`, `skitter_slain`, `skitter_ways` (the stone's ways
traced again, a wall having moved), `skitter_jump` (one moved more than 0.3 of a cell in a frame), `skitter_ways_broken` (a loop that
did not close) and `skitter_onpic` (one within half its legs' reach of a picture), the last three never yet. `CRAWL_STAGE=1` with `CRAWL_LEVEL` starts in the level, not
her hall, so the teleports apply.
`CRAWL_BUDGET=0` gives a level with next to no creatures, for looking at them.
`HAUNT_DEBUG` adds `HAUNT_PLAN=10,11,...` to force HAUNT.EXE acts (numbers in the README).

The debug build prints the autopilot's state once a second (`lvl=... t=... hp=... items=a/b val=a/b
...` plus a second line of hit counters) and `EVENT` lines: `cleared level N t=..`, `died level N
t=..`, `secret i of n`, `key..`, `leaving`, `bought`, `shop_pick`, `cast`, `defense`, `exit_seen`,
`control`, and for bosses `boss_begin`, `boss_wave`, `boss_adds`, `boss_rise`, `boss_fall`, `font`, `scrip`,
`hazard k` (`omen` when it is one), `hz_hit`, `omen set n`. Long simulations are just `CRAWL_SEED=s timeout 3000 build/host_debug apl crawl 3000 >
log` per seed, four at a time, then a short script over the logs: levels reached, median level time
(from `cleared`), treasure share (`val=` on the last state line before `cleared`), deaths, purchases,
secrets. Compare against a build of the previous commit (a `git worktree`) on the same seeds; one run
can spiral, so use at least eight seeds before trusting a balance change.

Memory safety: `make asan` builds `build/host_asan` (`-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer`
with the debug switches); run long autopilot and scripted player sessions on it
with `ASAN_OPTIONS=detect_leaks=0`. Every round so far has been clean; keep it that way.

Windows under Wine (in a container with `wine` and `xvfb-run`): set `WINEPREFIX`, run `wineserver -w`
before `xvfb-run -a -s "-screen 0 1920x1080x24"` (a wineserver started outside the display gives black
screenshots), run `HAUNTED.scr /s`, wait about 25 s for it to come up, then drive it with XTest from
Python `ctypes` (`libXtst.so.6`: `XTestFakeKeyEvent`, `XTestFakeRelativeMotionEvent`) and take
screenshots with ImageMagick `import -window root`. Settings, progress, the ledger and the mark live under
`HKCU\Software\HalloweenVGASaver` (`wine reg query` to check).

**Stamps** (docs/PLAN-stamps.md; harness only until the owner has played them). The generator's secret rooms, joiners and places come from
drawn stamps (`src/crawl/crawl_stamp_data.c`); the debug build's switches: `CRAWL_NOSTAMPS=1` (today's generator, byte for byte: the dream's
logs match 1.0.0b21.6 line for line, which is how a change is proved to move nothing), `CRAWL_NOJOINERS=1` (stamps for secret rooms only: no
joiners or places), `CRAWL_STAMP=name` (this stamp is tried first, wherever it fits), `CRAWL_STAMPLOG=1` (what each port opens on, the stamps
left out and why, the cells reached before and after the stone, and `MAPR`, the reach map when floor is cut off), `CRAWL_TPSTAMP=n` (stand
on the hall's cell before secret n, facing it, once you take control), `CRAWL_TPMARK=letter` (two to four cells from the first stamp mark
of that letter, facing it: `a b c s t`). Lines: `STAMPS level n: k rooms: [name xT plan (x,y) WxH] ...` (T the orientation, 0 to 7, as
`Stamp.xf`), `STAMPS level n: ...; made again without them` (a safety net fired: the level is made again with no stamps), `STAMPS level n
marks: ...` (what the marks became), and the `SECRETS` line names the stamp of each room. The sheet and its check:
`make build/stampsheet`, `build/stampsheet <dir> [secrets|joiners|places|extras|all|check]` (`check` runs the parser's rules on every stamp in
all eight orientations and exits 1 on a failure; `make sheets` runs it). Two proofs, in `tools/`:

```sh
make build/host_debug && python3 tools/stampsweep.py [--seeds 1-200] [--levels 1,5,10,11,20,21,30,31,40] [--nostamps] [--list]
python3 tools/stampbal.py --old build/bal/b216/build/host_debug [--new build/host_debug] [--seeds 1,3,5,...] [--secs 3000] [--reuse]
```

`stampsweep.py` generates each (seed, level) with `CRAWL_STAGE=1 CRAWL_MAP=1` and reads the printed map: it rebuilds reachability from it
(locked doors passable, secret walls shut, then open) and fails a level whose exit, a key, a locked door, a secret entrance, a secret room or a
doorway cannot be reached, or whose doorway touches no floor of its room; it counts hangs, crashes and the levels made again without stamps
(the exit code is 1 on any failure). It also takes `--host build/host_asan` (with `ASAN_OPTIONS=detect_leaks=0`) to run the same under the
sanitizers. **The harness ORs the seed with 1, so seeds 1-200 are 100 layouts, and a seed gives the same layout at every level.** A stamp
overlapping a vault, a wing, a trial's room or the arena is not tested directly (a vault door or key it covered would show as unreached).
`stampbal.py` runs the dream on the same seeds in this build and in an older one (a `git worktree` at the commit to weigh against, with
`make build/host_debug` in it; keep it under `build/bal/`, which is not tracked) and prints levels cleared, deaths, the median seconds a
level took, secret rooms found and gold, per seed and in all; 16 seeds of 3,000 s take about 17 minutes on four cores. Eight seeds carry
noise of about a quarter either way: read the means.


**Side passages** (docs/PLAN-side.md; harness only until the owner has played them). The debug build's switches: `CRAWL_SIDE=n` (straight
into built-in side passage n, `SIDE_TEXT[n]` in `game/side_levels.c`, half a second into each level, once a level; its doorways out all lead
back before the cell you faced), `CRAWL_TPSIDE=n` (a side doorway to passage n, every link of it, in the first wall ahead of the start: walk
into it or press E), `CRAWL_SIDETRACE=1` (twice a second, `SIDE at x,y tile tx,ty v vx,vy ground plat crouch in L R J D C hp t`, and each of
the dream's plans: `SIDE plan from a to b: n reached, the furthest x,y, a way|no way`). Events: `side_in`, `side_out` (its link, whether the
goal), `side_take`, `side_switch`, `side_cast`, `side_hurt`, `side_slain`, `side_pit`, `side_life`, `side_over`, `side_dream gives up`, `side_works` (a thing a trigger turned on or
off), `side_trap` (a trap going off), `life_bought` (a life for the passages bought in her room); `CRAWL_TPLIFE=1` (before that life, in her
room), `CRAWL_SIDECAM=x,y` (a passage's view held at that tile; with `CRAWL_SIDETRACE`, `SIDE fx` counts of its particles by kind),
`CRAWL_SIDEFIRE=1` (the dream in a passage casts the fire bolt, to light silt gas), `CRAWL_SIDEMOBS=1` (each of the dead, twice a second: `SIDE mob i KIND at x,y act a next n knows|- hp h`), `EVENT side_dead KIND does what` (a creature's act: strikes, leaps, dives, drops, rings, winds his horn, raises the fallen ...), `EVENT side_bone from x,y to x,y (n off) in s s, rising n px` (a skeleton's throw), `EVENT side_death_sound k` (a life lost, the sound TUNING's death_sound chose), `CRAWL_SIDESND=1` (each sound a passage places: `SND id at d px: heard|unheard gain of gain`), `CRAWL_SIDEMETRO=1` (a timed switch's metronome, each beat: `METRO i beat n chord A|B up n`), `EVENT side_power kind for secs`, `EVENT side_flask aimed n px ahead (n below), held s` (a flask thrown), `EVENT side_switch i back, its time out`, `EVENT side_trap HZ_SNARE holds` and `CRAWL_SDLOST=n` (the passages' lives lost at a fresh start); `SIDE
door` when a doorway is laid and `SIDE n unreadable: line n: why` when a text is refused. The draft art's sheet: `make build/sidemock`,
`build/sidemock out.png` (kept as `media/sheets/side/side_draft.png`). The World Editor's PLAY, in the harness (any build):
`HOST_SIDE=file:n[,file:n]` (passages read as PLAY hands them over, each into slot n, 0 to 7, in place of what is built in; a text it
cannot read stops the harness with the reason and its line) and `HOST_SIDE_NOW=n[:level]` (a new run on that level, the merchant passed
by, straight into passage n). `node tools/mapedit/test.js` writes `tools/mapedit/fixtures/side_*.txt` and `side_crypt.c` for them.
