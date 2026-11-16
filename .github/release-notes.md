A Needle of Bone: the ninth point release of the twenty-first beta of **HAUNTED.EXE**, a Halloween screensaver for Windows in real VGA modes: a possessed 486 PC (HAUNT.EXE) and a Wolfenstein-style crypt crawl (CATACOMB) that you can take over and play by holding **H**, or any button on a game controller, for three seconds.

It sells you a compass, and takes the list of controls off the screen for now.

### Before you upgrade

Your saves carry over. A page now remembers a compass if you hold one; an earlier build reads a page from this one as having none, and this one reads an earlier page the same way. The settings are as they were. A copy of your saves costs a moment, and this is as good a time as any; from a Command Prompt:

```
reg export "HKCU\Software\HalloweenVGASaver" "%USERPROFILE%\Desktop\haunted-saves-backup.reg"
```

### New in this release

**The compass.** Once the map is yours, the merchant sells a compass for 2,500 gold: a disc of old stone, chipped and mossed, with a needle of bone, hanging at her right. It leans toward whatever you have asked it to find, and says how far: `GOLD KEY  36 S`. It knows the direction and the distance, not the route, so it will point through a wall and leave the way round to you. The disc turns as you do, and the gilt stud on its rim is always north.

You choose what it leans toward in the L list, where the wisps are: **Back** (Backspace; X on a controller; the BACK stone by touch) aims it at the row chosen, and a small dial marks the row it is aimed at. It begins on the way out. The keys are there for Keymaker, as they are for the wisps, and the altar's secret while its wisp is out. When what it leans toward is gone, a key taken or a secret found, it turns back to the way out and says so. An exit behind a locked vault leans toward the key that vault wants first, as the exit wisp goes.

It sits in a plate at the top right of the view, below the news. By touch it is a tablet under MAP, worn blank until you have bought it; tap it to open the list.

A death that wakes you in her room takes it, with the rest; the map stays. The card says so. The screensaver does not use it.

**The controls list is gone, for now.** The list of keys that stood on the screen for six seconds when you took over, or opened a page, no longer appears. It will be back.

### What has been tested

Nobody has played this by hand: not the compass, not a finger on its tablet, not the list's going. Everything is tested in the test harness only; nothing has been run under Wine, and the Windows and browser builds were compiled and smoke-tested by GitHub Actions and by nothing else.

In the harness, the bearing was checked from eight places round the exit and each key on levels 1, 4, 5, 12 and 23, and the saved page read back for every compass it can hold: none failed. The screensaver played eight seeds for 3,000 seconds of its time, against the build before, and logged the same lines to the line, about 11,000 of them a run; it cannot buy the compass, so it could not have moved. Scripted players bought one, aimed it, died and woke, and the lot ran clean under AddressSanitizer. What 2,500 is worth is not known by hand. The screensaver gathers about a thousand gold a level at first and about three and a half thousand from the fourth, and takes nearly everything it sees, so a person will find it dearer.
