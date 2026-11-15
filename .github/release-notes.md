Drawn Rooms: the seventh point release of the twenty-first beta of **HAUNTED.EXE**, a Halloween screensaver for Windows in real VGA modes: a possessed 486 PC (HAUNT.EXE) and a Wolfenstein-style crypt crawl (CATACOMB) that you can take over and play by holding **H**, or any button on a game controller, for three seconds.

It lays the crypt's rooms by shape, so that what you walk through was put where it is.

### Before you upgrade

Your saves carry over, and nothing in them has changed. The settings are as they were. A copy of your saves costs a moment, and this is as good a time as any; from a Command Prompt:

```
reg export "HKCU\Software\HalloweenVGASaver" "%USERPROFILE%\Desktop\haunted-saves-backup.reg"
```

### New in this release

From the third level, a level is built round rooms of a drawn shape, and the halls are made to reach them. The first two levels, her hall and a boss's arena are as they were.

**A place on every level.** A shrine spur: a way one cell wide that ends at an altar. A cloister: a loop of hall round a solid yard. A gauntlet hall (from the second level of a run): long and narrow, with niches for treasure and the dead. An alcove hall: recesses down both sides. From the sixth level of a run, a sunken ring: stepped rings round a floor, braziers and an altar in the middle. About half the levels get one of the larger four, and the rest a spur. Altars, braziers and torches stand where the drawing puts them, treasure lies in the niches, and some of the niches hold one of the dead, which keeps to its room until it knows you are there.

**Rooms where halls branch.** One big and two or three small a level, no two of a kind: stepped diamonds with a cross, a ring, four pillars or a star in the middle, a transept, a pinwheel, a trident, a horseshoe, a ladder of two halls, an hourglass with a narrow neck, a dogleg stair, a chevron, a gallery three wide, and from the sixth level a great nave with six slabs standing in it.

**New secret rooms.** Ten shapes: boxes and a strip, an L and its mirror, an arrowhead, from the third level a C, a spiral and a J, and from the fifth a long, narrow approach. The cracked wall now slides one cell sideways into the stone beside it, and leaves the room empty to walk.

The screensaver walks all of it, and the map shows it whole once you have drawn it.

### What has been tested

The author played the Windows build GitHub made of this and said it plays well; that is all that has been said of it by hand. Everything else is tested in the test harness only: nothing has been run under Wine, and the Windows and browser builds were compiled and smoke-tested by GitHub Actions and by nothing else.

In the harness, 1,800 levels were made with every exit, key, locked door and secret room tested for being reached, and no floor stranded: none failed. On 22 of them a drawn room had cut the level in two, and it was made again without. The screensaver played sixteen seeds for 3,000 seconds of its time, twice, against the build before: it cleared as many levels (6.6 and 6.9 against 6.6 and 6.2) and died less (32 and 27 against 47 and 47), but the old build's own numbers move between runs, so read that as no worse, not as safer. It reaches level 8 or 9 in that time and no further. With the drawn rooms switched off it logs the build before's line for line. Clean under AddressSanitizer.
