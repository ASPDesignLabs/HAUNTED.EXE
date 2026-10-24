The fifth beta of **HAUNTED.EXE**, a Halloween screensaver for Windows in real VGA modes: a possessed 486 PC (HAUNT.EXE) and a Wolfenstein-style crypt crawl (CATACOMB) that you can take over and play by holding **H** for three seconds.

### New in this beta

- **More of the crypt to look at.** The status bar slides away five seconds after anything it has to show, and comes back when you are hurt, choose a spell, tap **H**, or open the wardrobe or the bestiary; a small readout shows gold, keys and souls as they come and go meanwhile. Your hand rises only to cast or to reach for something.
- **Wisps in the crypt.** **L** lists the wisps you can summon and their price. A wisp to the way out costs a soul: it flies off down the halls, fetching the key first if the exit is locked away, lights the dark as it goes, drives back the Wraith, and leaves a trail of motes that hangs for twelve seconds so you can follow it round corners. With a Keymaker piece on, there is a wisp for each key still lying. Set **L** to cast your choice at a tap; holding **L** brings the list back.
- **The Black Altar's gifts.** **E** at an altar offers its three: wake its violet wisp to the secrets (10 souls), bless your exit wisps for the level (25 souls), so that the undead flee from them, and, once every secret is found, anoint something you wear.
- **The autopilot uses them too:** an exit wisp as it heads for the way out, and the altars when it has souls to spare.
- **The Wraith now comes once a level**, as it was always meant to; it had been coming only once a session.
- **Secrets were added.**

This is still a beta, and none of what is new has been played by hand yet: it has been tested by the harness, and the autopilot's runs are in the README. If something goes wrong on your machine, please [open an issue](https://github.com/ASPDesignLabs/HAUNTED.EXE/issues) with your Windows version and graphics card.

### Before you upgrade

Your settings, progress and graves carry over from earlier betas. This beta adds four bytes to the save, and **earlier betas can't read it**: they start again from level 1, and fleeing there overwrites the save. If you might go back, keep a copy first, from a command prompt:

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
