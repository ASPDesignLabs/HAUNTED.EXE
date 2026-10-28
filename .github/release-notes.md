The ninth beta of **HAUNTED.EXE**, a Halloween screensaver for Windows in real VGA modes: a possessed 486 PC (HAUNT.EXE) and a Wolfenstein-style crypt crawl (CATACOMB) that you can take over and play by holding **H**, or any button on a game controller, for three seconds.

### New in this beta

- **A map.** The merchant sells one, once, for 1,000 gold: a small scroll floating at her left. **M** (up on a controller's D-pad) unrolls it across the view while the crypt waits: the level from above, north up, in brown ink on torn parchment. Every wall within about three cells of you is drawn as you go, through the stone too. Move it about with W, A, S and D or the mouse; Space brings it back to you. It outlasts death.
- **The target's health.** Look at a creature and its name, rank and health show at the top of the view, with the spell that hurts it most on the left and the one that hurts it least on the right, once you have learnt them by killing its kind (`?` until then, `--` when no spell differs).
- **Experience.** Every kill, secret, chain's end, boss, level cleared and room first entered is experience, written at the lower left as it comes, with its value. When enough is banked for a level, a choir, a bell and a whisper sound and *LEVEL OF ATONEMENT* crosses the view.
- **Levels of Atonement.** The merchant's menu has *ASCEND*: every level the experience reaches, ten points each, to the fiftieth, spent on the caster's own health, damage, resistances, spell haste, dodge, treasure, movement, Luck (new: purses of gold from the dead) and attunement, added to what you wear under the same caps.
- **Cornerstones.** At each tenth level, one of five, free: **Second Wind** (back at half health from a killing blow, once a run of ten, or again for a Cursed Coin), **Premonition** (a purple edge on the floor before the place does harm), **Cartographer** (the map comes drawn), **Lantern of the Dead** (a light that shows the shades and keeps the Wraith off) and **Grave Sense** (creatures on the map).
- **Overlevels.** Past the fiftieth, at 55 and every fifth level after, one stat goes 5% past its cap (Luck 1%), drawn with an eerie mark wherever it shows.

A death takes the experience, the levels, the points and the cornerstones, as it takes the gold; the map stays, and the merchant's claim leaves your levels alone. The screensaver's caster earns and spends levels too: it dies less than it did, though the first knight still stops it about as often.

This is still a beta. Everything new has been tested only in the harness, with scripted players and the autopilot: none of it has been played by hand, run on Windows or under Wine, and how a person's pace through the fifty levels compares with the autopilot's is unknown. The controller from the eighth beta and everything in the seventh haven't been played by hand yet either. If something goes wrong on your machine, please [open an issue](https://github.com/ASPDesignLabs/HAUNTED.EXE/issues) with your Windows version and graphics card.

### Before you upgrade

Your settings and saves carry over. This beta adds the map and your Levels of Atonement to each page of the ledger as a part of its own; the seventh and eighth betas step over it, so they can still read your pages, without those. If you are coming from the sixth or earlier, its graves open as pages of the merchant's ledger, and the sixth and earlier can't read the saves this one writes. If you might go back that far, keep a copy first, from a command prompt:

```
reg export HKCU\Software\HalloweenVGASaver haunted-backup.reg
```

Double-clicking `haunted-backup.reg` puts it back.

### Files

| File | What it is |
|---|---|
| `HAUNTED.EXE` | The program on its own. Double-click it to open it in a window: hold H, or a controller's button, to play, F11 for full screen, Esc to quit. |
| `HAUNTED.scr` | The same program as a screensaver. Right-click it and choose **Install**, then pick HAUNTED.EXE in Screen Saver Settings. |
| `HAUNTED-<version>-win64.zip` | Both, with `install.cmd`, `uninstall.cmd`, `capture.cmd` for recording, the README, previews and the full source. |
| `SHA256SUMS.txt` | Checksums of the three files above. |

The files aren't code-signed, so SmartScreen may stop them the first time: choose *More info*, then *Run anyway*.

The project page: https://aspdesignlabs.github.io/HAUNTED.EXE/
