The fourth beta of **HAUNTED.EXE**, a Halloween screensaver for Windows in real VGA modes: a possessed 486 PC (HAUNT.EXE) and a Wolfenstein-style crypt crawl (CATACOMB) that you can take over and play by holding **H** for three seconds.

### New in this beta

- **Things to wear, rolled like loot.** Seven places: a bracer on each wrist, boots, an amulet, a hood, robes and a ring. Every piece comes in a quality you can see by its colour, Common (white) to Exquisite (gold), and the better the quality, the more stats it rolls and the stronger they are: armour, dodge, health, wards against poison, burning and slowing, movement, quiet footsteps, spell haste, spirit cost, damage, fire, frost or lightning, and spell effect. About one in ten comes Anointed, a little stronger.
- **Abilities.** Hoods always carry one, other pieces sometimes: Swift, Arcane, Treasure Hunter (a whisper near secret walls), Keymaker (keys shine through the stone, and **L** sends the wisp to each in turn), Communing (creatures as red shapes through the walls), Thorned and Mirrored.
- **Spirit and bones.** Your souls are now spent on spells, to the tenth. The Fire Bolt you start with and any spell at tier-I strength are free; stronger casts cost a little, and with too little spirit a spell goes out weak. Every Bone Spear throws a bone, which skeletons, shield-bearers, knights and hounds drop now and then.
- **The merchant's wider room.** Five to seven pedestals, one to three purchases a visit by depth, and **Q** holds a ware for your next visit for a quarter of its price. Bags, from a Burlap Sack to a Bone Reliquary, keep spare pieces; without one, what a new piece replaces is lost, and the card warns you first.
- **Black Altars.** From level 6, an altar stands deep in each level. Ten spirit wakes it, and its violet wisp leads you to every secret on the level; find them all and it anoints something you wear.
- **The wardrobe and the caster's page** (Tab): what you wear and carry, every total, what your abilities add, and what has reached its cap.

This is still a beta, and none of what is new has been played by hand yet: it has been tested by the harness, and the autopilot's runs are in the README. The deepest levels are still the hardest, and will be tuned as they are played. If something goes wrong on your machine, please [open an issue](https://github.com/ASPDesignLabs/HAUNTED.EXE/issues) with your Windows version and graphics card.

### Before you upgrade

Your settings, progress and graves from earlier betas carry over: spells, runes, gold and souls as they were, with your robe, hood and best ring turned into Common pieces. This beta saves in a new format, and **1.0.0b3 and earlier can't read it**: they start again from level 1, and fleeing there overwrites the save. If you might go back, keep a copy first, from a command prompt:

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
