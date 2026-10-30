The eleventh beta of **HAUNTED.EXE**, a Halloween screensaver for Windows in real VGA modes: a possessed 486 PC (HAUNT.EXE) and a Wolfenstein-style crypt crawl (CATACOMB) that you can take over and play by holding **H**, or any button on a game controller, for three seconds.

### Before you upgrade: back up your saves

**This beta changes saves one way.** It writes a new kind of save that the tenth beta and earlier can't read: go back after playing it and they start again from level 1, and fleeing there writes over what you had. The first time it opens a page from before it, that page's spells are forgotten for good (you are given an Alignment Point for each Level of Atonement past the first instead) and the merchant asks what you were. Everything else on the page is kept. So keep a copy first, from a command prompt:

```
reg export HKCU\Software\HalloweenVGASaver haunted-backup.reg
```

Double-clicking `haunted-backup.reg` puts it back. Your settings carry over either way.

### New in this beta

**The caster grows by element now, not by what the merchant has on her pedestals.**

- **Six elements.** Fire, Frost, Lightning, Poison, Spirit and Physical. Every creature takes more or less from each, and does its harm in one or two of them. The target's readout shows it as glyphs: what it takes worst on the left, what it shrugs off on the right, its harm underneath, each as far as you have learnt it.
- **Spells are lines**, one an element, bought with **Alignment Points** (one a Level of Atonement) at **the Lampstand**, in a new room north of the merchant's. Three points open a line (Fire comes open), every point after is a rank, and a spell's strength follows your Level of Atonement. Each line has Efficiency, Insight, an ability, and two paths that close each other off, each ending in an **ultimate**: Flamewalker or Firewall, Winter's Lance or Shatter, Arc or Amperage, Plague Pit or Contagion, Soul Tether or Wisp Host, Phalanx or Ballista. Two Cursed Coins unbind a line for half its points back. Tab's fourth and fifth pages show the tree and the elements.
- **Wind-ups and charging.** Every spell gathers for a moment before it goes (shorter with Insight; you can walk meanwhile), and a blow that lands then may break it. Hold the button to charge an ultimate: a light gathers by your hand in the element's colour and lights the crypt round you.
- **What you were.** At a fresh start, and after every death that costs, she asks *WHAT WERE YOU?* Six classes, each a trade that ended many lives: the Witchfinder, the Anatomist, the Galvanist, the Plague Doctor, the Sin-Eater and the Headsman, each with a line, a gift and a story. From Level of Atonement 20 she hears another past for five Cursed Coins.
- **Gear rolls the elements**: their Power, Efficiency and Insight, leaning towards the lines you have ranked, and two new stats, Resist Spirit and the Vessel.
- **Souls can run dry.** Spells spend souls by rank, and with too few a spell goes out at rank 1, weak. A **vessel** holds twenty and two a Level of Atonement; what comes in over it slips away. The dead leave **motes** where they fell, to be walked through. In the new room, **the Dead Bell** corrupts your next level for souls (more of the dead and their gold, less experience; on a level cleared before, more souls show its secrets), and **the Weeping Font** sweetens the next level's experience. She takes an **offering** of ten souls for experience. **The Accursed Portal** replays any level you have cleared. **Spectres** come on every level by your Luck, and Spirit Mastery slays them for their souls.
- **Soul Sap.** Bats of rank II and up latch onto your face, feeding on health and souls and slowing you, until you shake them off (turn hard one way and back, or strafe) or throw them with Blink or Swift Dodge.
- **The map at a walk.** The crypt goes on while the map is open: walk, turn and cast with it up. The parchment is torn and burnt through, and the ink goes on over the holes in a ghostly violet. Walked cells are washed darker, and the level's numbers are across its foot.
- **The runes' sockets.** When a rune fires its mark comes up at the lower right, filling until it is ready again.
- The merchant no longer sells spells (the BFS9000 still turns up now and then). The Codex has pages for the lines, the classes and the souls, and the controls card has the new keys.

**Balance.** @BALANCE@

This is still a beta. Everything new has been tested only in the harness, which draws exactly what the game draws: none of it has been played by hand, run on Windows or under Wine, or seen on a real screen. The four runs' looks from the tenth beta, the map and Levels of Atonement from the ninth, the controller from the eighth and everything in the seventh haven't been played by hand yet either. If something goes wrong on your machine, please [open an issue](https://github.com/ASPDesignLabs/HAUNTED.EXE/issues) with your Windows version and graphics card.

### Files

| File | What it is |
|---|---|
| `HAUNTED.EXE` | The program on its own. Double-click it to open it in a window: hold H, or a controller's button, to play, F11 for full screen, Esc to quit. |
| `HAUNTED.scr` | The same program as a screensaver. Right-click it and choose **Install**, then pick HAUNTED.EXE in Screen Saver Settings. |
| `HAUNTED-<version>-win64.zip` | Both, with `install.cmd`, `uninstall.cmd`, `capture.cmd` for recording, the README, previews and the full source. |
| `SHA256SUMS.txt` | Checksums of the three files above. |

The files aren't code-signed, so SmartScreen may stop them the first time: choose *More info*, then *Run anyway*.

The project page: https://aspdesignlabs.github.io/HAUNTED.EXE/
