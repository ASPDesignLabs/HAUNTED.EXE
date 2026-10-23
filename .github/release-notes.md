The third beta of **HAUNTED.EXE**, a Halloween screensaver for Windows in real VGA modes: a possessed 486 PC (HAUNT.EXE) and a Wolfenstein-style crypt crawl (CATACOMB) that you can take over and play by holding **H** for three seconds.

### New in this beta

- **Who lives down there.** Twenty kinds of creature, brought in about one a level as you go down, each a level or two after its sign and alone the first time. The Crypts add shamblers climbing out of turned graves; the Catacombs the Drowned, bone hounds in pairs and the Wraith, which nothing kills; the Hollows hunting spiders on the ceiling, ghouls, skeleton knights and brood mothers with their egg sacs; the Deep shades you see only in light, ember imps, deacons that raise the dead and gargoyles that are statues until you pass.
- **Ranks you can see.** Every creature comes in its own colour or in green, blue, red or black, each tougher, quicker and worth more souls; the deeper you go, the higher the mix. Now and then one is gilded and carries treasure. A caption names each kind and rank the first time you meet it on a level.
- **They hunt.** A creature that sees you, is hit or hears a spell follows you round corners, and the dead pull doors open. Each kind has its weaknesses, and the autopilot plays to them.
- **Four sets of five.** The twenty levels have names and come in sets: the Crypts, the Catacombs, the Hollows and the Deep, each with its own stone and light, rotting further towards its last level. Then round again.
- **Achievements**, for kills of each creature in each rank, for secrets, for each of the first twenty levels and each time down after. They slide in at the upper right, the autopilot's too, and they are kept with your saved run and with each grave.
- **The bestiary.** Press **B** while you play: a page for each creature, turning on the spot in each rank you have met, with your kills and what hurts it, learnt from three kills with each spell. The last page lists the achievements.

This is still a beta. The author has run it on their own Windows PC and found it looks and runs well, but most of what is new has been tested by the harness, not yet played through by hand. The deeper levels are much harder than the first ten, and will be tuned as they are played. If something goes wrong on your machine, please [open an issue](https://github.com/ASPDesignLabs/HAUNTED.EXE/issues) with your Windows version and graphics card.

### Before you upgrade

Your settings, progress and graves from earlier betas carry over. This beta saves in a new format to keep the record, and **1.0.0b2 and earlier can't read it**: they start again from level 1, and fleeing there overwrites the save. If you might go back, keep a copy first, from a command prompt:

```
reg export HKCU\Software\HalloweenVGASaver haunted-backup.reg
```

Double-clicking `haunted-backup.reg` puts it back.

### Files

| File | What it is |
|---|---|
| `HAUNTED.EXE` | The program on its own. Double-click it to open it in a window: hold H to play, F11 for full screen, Esc to quit. |
| `HAUNTED.scr` | The same program as a screensaver. Right-click it and choose **Install**, then pick HAUNTED.EXE in Screen Saver Settings. |
| `HAUNTED-<version>-win64.zip` | Both, with `install.cmd`, `uninstall.cmd`, `capture.cmd` for recording, the README, previews and the full source. |
| `SHA256SUMS.txt` | Checksums of the three files above. |

The files aren't code-signed, so SmartScreen may stop them the first time: choose *More info*, then *Run anyway*.

The project page: https://aspdesignlabs.github.io/HAUNTED.EXE/
