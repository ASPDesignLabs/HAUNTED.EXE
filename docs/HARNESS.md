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
`HOST_SAVE=file` (the progress, each time the game writes it), `HOST_SLOTS=dir` (the ledger's pages as
`slot1.bin`..`slot3.bin`, the dead's mark as `mark.bin`, the pad's layout as `prefs.bin`), `BOTH=1` (scene rotation), `STAY=n`. A game
controller in `HOST_INPUT`: buttons `+PAD_A` ... `-PAD_RT` (names as `PAD_*` in `core.h`), sticks `aLX0.8`,
`aLY1`, `aRX-0.5`, `aRY0` (y up); any of them makes a pad plugged in. The harness prints `RUMBLE lo hi`
and `RUMBLE off` as the motors start and stop. `M` is the map's key (1.0.0b9).

Debug build switches (`CRAWL_DEBUG`): `CRAWL_SEED=n` (seeds are OR-ed with 1, so 2 and 3 are the
same), `CRAWL_LEVEL=n` (start deeper; you start at the merchant), `CRAWL_GEAR=t` (every spell at
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
`CRAWL_PEDS=n` (the merchant's pedestals), `CRAWL_TPCAT=1` (before the merchant herself), `CRAWL_TPSTONE=1` (before the plot's newest stone), `CRAWL_STONE=n` (n stones in the plot, from what is held), `CRAWL_TPSHRINE=1`
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
and into each pocket).
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
