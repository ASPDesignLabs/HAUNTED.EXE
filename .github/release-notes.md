The sixth beta of **HAUNTED.EXE**, a Halloween screensaver for Windows in real VGA modes: a possessed 486 PC (HAUNT.EXE) and a Wolfenstein-style crypt crawl (CATACOMB) that you can take over and play by holding **H** for three seconds.

### New in this beta

- **Secrets that go deeper.** Three to six on every level, behind cracked walls, in rooms carved out of the rock. Some rooms have a doorway at the back, and a chain of places beyond it: on the first ten levels **the Backrooms** (yellow wallpaper, damp carpet, failing tubes, things from the crypt where they shouldn't be, creatures glitching, and no music), on the next ten **Severance** (white halls, green carpet, offices, too much light, and muzak). The level waits while you are away. Each link is bigger and its creatures stronger; at the end, a visportal or a water cooler restores you, with a **Cursed Coin**.
- **The Rune Shrine.** Runes no longer come from the merchant. Across the room a shrine waits to be cursed with a Cursed Coin, and then gives runes for coins, in five qualities from Common to Exquisite; three alike become one better. Every rune at Exquisite does a second thing. Coins come at the end of each chain, and one more for finding every secret on a level.
- **The Black Altar's wisp** now goes to one secret at a time and waits at its door while you are inside; once you are out again, **E** at it sends it home, and from the altar you send it on, for a little more each time.
- **The autopilot does all of it too**, within reason: it presses every secret it passes while it has time, goes into chains while it is healthy, and plays the shrine with the coins it finds.
- **Your hand** rises once to its resting height to cast while the status bar is away.

This is still a beta, and none of what is new has been played by hand yet: it has been tested by the harness, and the autopilot's runs are in the README. If something goes wrong on your machine, please [open an issue](https://github.com/ASPDesignLabs/HAUNTED.EXE/issues) with your Windows version and graphics card.

### Before you upgrade

Your settings, progress and graves carry over from earlier betas: runes come across by their old tier as a quality, and a rune held at the merchant's is let go and its fee given back. This beta adds 32 bytes to the save for the shrine, and **earlier betas can't read it**: they start again from level 1, and fleeing there overwrites the save. If you might go back, keep a copy first, from a command prompt:

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
