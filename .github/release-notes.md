The twenty-first beta of **HAUNTED.EXE**, a Halloween screensaver for Windows in real VGA modes: a possessed 486 PC (HAUNT.EXE) and a Wolfenstein-style crypt crawl (CATACOMB) that you can take over and play by holding **H**, or any button on a game controller, for three seconds.

It gives every spell line six augments, a pair at each of its great steps, earned by using the line; and the merchant's wares now lean to the lines you hold, with your class's own abilities among them.

### Before you upgrade

Your saves and settings carry over. A page signed on this beta keeps the augments you have chosen and their credits.

Going back to 1.0.0b20 is not quite safe: it reads a page without the augments (and a page it signs loses them), and it reads any piece you wear or carry that has one of this beta's new stats (a mark's chance, Steadiness) or a class's own ability as nothing, so such pieces would be lost. If you might go back, keep a copy of your saves first, from a command prompt: `reg export HKCU\Software\HalloweenVGASaver haunted-backup.reg` (double-click the file to restore it).

### New in this release

**The spells' augments.** Each line earns a pair of augments at its opening, its ability and its ultimate. At the Lampstand, Space (Y on a controller) on that step's node shows the pair; A and D choose, Enter takes one, asked twice, for good until the line is unbound. Unbinding a line is now Back (X on a controller), and its augments go with it.

They grow by use: a kill by the line's spell is a credit, a thing that wants the spell struck with it two, a boss ten. An augment's ranks come at 20, 50 and 100 credits of its own, the line's newest first; rank I gives half of what it does whole, rank II three quarters. Credits a line can't spend yet wait in its bank, so a pair chosen late can rank up at once. The Codex has a page for each one chosen.

| Line | Its opening | Its ability | Its ultimate |
|---|---|---|---|
| Fire | Burning Intensity (hold the bolt and it grows, to a quarter stronger and two and a half times the size; tapped, a fifth weaker; with the Firewall, a hold past three seconds calls it) or Firecracker (a spark, a quarter of the damage, all but sure to set its mark alight) | Multiply (three bolts in an arc) or Swift Flight (twice as fast) | Volatility (what dies of your burning bursts, setting the rest alight) or Pyre (burning passed on to one more) |
| Frost | Hoarbite or Splinterfrost | Rime Trail or Glacial Weight | Cold Snap or Brittle |
| Lightning | Forked or Overcharge | Static Build or Grounding | Thunderclap or Conductor |
| Poison | Thick Brew or Straight Throw | Cluster or Miasma | Pestilence or Rot |
| Spirit | Siphon or Hollowing | Lingering Hunger or Soul Echo | Exorcism or Shepherd |
| Physical | Barbed or Recovery | Twin Throw or Heavy Haft | Skewer or Bone Rain |

The Fire node that was called Burning Intensity, which makes burning last longer, is now Slow Embers.

**The merchant, by what you hold.** An element's stats now roll for the lines you hold, more for those ranked higher; only one piece in six carries one for a line you don't, and anything Rare or better carries at least one for yours. New stats: a chance of each element's mark on the dead (Ignition, Rime, Static, Blight, Hush) on rings and bracers, and Steadiness (Knocked Down, Stunned and Frozen shorter and rarer) on boots and amulets. The element added to every spell now rolls among all six lines you hold. From Uncommon up, one piece in three may carry your class's own ability: Pyrebrand, the Slab, Live Wire, Fever, Swallowed Sin or the Block.

**Harder again at the start, for the screensaver.** Weighed against 1.0.0b20, the screensaver's caster died a third more often in its first fifty minutes from level 1 (158 deaths against 117 over eighty runs), on levels 3 to 7. It chooses its augments at random, and two openings cost it most: Firecracker and Siphon, each trading part of the one spell it has early for something that comes later. That is kept on purpose: a person chooses their own. From level 31 nothing moved.

This is still a beta. All of it has been tested in the harness, by the screensaver's own play, by scripted play and on sheets of the art: nobody has played it by hand yet. The Windows and browser builds are compiled and smoke-tested by GitHub Actions. If something goes wrong on your machine, please [open an issue](https://github.com/ASPDesignLabs/HAUNTED.EXE/issues) with your Windows version and graphics card.

### Files

| File | What it is |
|---|---|
| `HAUNTED.EXE` | The program on its own. Double-click it to open it in a window: hold H, or a controller's button, to play, F11 for full screen, Esc to quit. |
| `HAUNTED.scr` | The same program as a screensaver. Right-click it and choose **Install**, then pick HAUNTED.EXE in Screen Saver Settings. |
| `HAUNTED-<version>-win64.zip` | Both, with `install.cmd`, `uninstall.cmd`, `capture.cmd` for recording, the README, previews and the full source. |
| `HAUNTED-<version>-web.zip` | The same game built for the browser, the page that plays it, to serve yourself. |
| `SHA256SUMS.txt` | Checksums of the files above. |

The files aren't code-signed, so SmartScreen may stop them the first time: choose *More info*, then *Run anyway*.

The project page: https://aspdesignlabs.github.io/HAUNTED.EXE/

Play it in the browser, with nothing to download: https://aspdesignlabs.github.io/HAUNTED.EXE/play/ (Space casts there, since Ctrl is the browser's; the ledger is kept in the browser).
