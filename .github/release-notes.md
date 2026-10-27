The seventh beta of **HAUNTED.EXE**, a Halloween screensaver for Windows in real VGA modes: a possessed 486 PC (HAUNT.EXE) and a Wolfenstein-style crypt crawl (CATACOMB) that you can take over and play by holding **H** for three seconds.

### New in this beta

- **Runs of ten, and four bosses.** Forty named levels in four runs: the Crypts, the Catacombs, the Hollows and the Deep. The tenth of each ends in an arena: **the Ill-Fated Skeleton Knight**, **the Wandering Hunting Spider**, **the Black Deacon** and **the Wraith Unbound**, each with its own runes, its own ways and a wave of adds at every quarter of its health. Their hazards (falling rocks, bile, hellfire, the void) always warn first, and come into the halls as omens on the levels before. Each has a waltz of its own.
- **Death costs**, once you have taken over. Your gold, everything you wear, your spells, souls and coins go; a stone is cut in the new grave plot with what you carried and what took you; you wake at the merchant's before level 1. The runes, the Rune Shrine and the merchant's hall stay. The screensaver, playing by itself, still only dreams.
- **The merchant's ledger.** Three pages, and only she signs them: at her menu, by your death, or by her claim. Fleeing keeps only what was signed. Load a page from Esc while you are in danger, or close the window with a creature at your back, and she takes the dead's claim: a share of your gold and one thing you wear, whatever you meant by it.
- **Her hall.** Pedestals bought from her, two to seven. **The Hexed Totem** teaches each beaten boss's ability for its scrip: Swift Dodge on a double tap, and Tangled Web, Grave Call and Wraithwalk on the right mouse. **The Accursed Portal** opens each run's start once its boss has fallen, and the bosses again as challenges, a step harder each win. **The Corpus Compendium** sells back what you have worn. **The Wretched Repository** keeps pieces, gold and coins through death. **C** opens **the Codex of Affixations**, a page for everything you have known.

This is still a beta, and none of what is new has been played by hand yet, nor run under Wine: it has been tested in the harness, and what that showed is in the README. The autopilot finds the first knight hard (it beat him in 2 runs of 8) and gets less far than it did; that will be tuned. If something goes wrong on your machine, please [open an issue](https://github.com/ASPDesignLabs/HAUNTED.EXE/issues) with your Windows version and graphics card.

### Before you upgrade

Your settings and saves carry over: the sixth beta's graves open as pages of the ledger, and its last progress as a run not yet signed (sign it at the merchant's to keep it on a page). This beta writes a new kind of save, and **earlier betas can't read it**: they start again from level 1, and fleeing there overwrites the save. If you might go back, keep a copy first, from a command prompt:

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
