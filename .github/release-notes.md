A point release of the nineteenth beta of **HAUNTED.EXE**, a Halloween screensaver for Windows in real VGA modes: a possessed 486 PC (HAUNT.EXE) and a Wolfenstein-style crypt crawl (CATACOMB) that you can take over and play by holding **H**, or any button on a game controller, for three seconds.

It brings the work planned for the twentieth beta early, before that beta's long balance runs: the Unholy Confessions in place of the old lines, and what each class is made of. The twentieth beta comes once these have been weighed against 1.0.0b19.

### Before you upgrade

Your saves and settings carry over. The lines you bought with your points at the merchant are undone as a page is read, and every point comes back to you to confess; she tells you so as she greets you. Overlevels come back the same way, to choose again.

Going back to 1.0.0b19 is safe, but it knows nothing of the Confessions: the points spent on them aren't there, and a page it signs loses them for good.

### New in this release

**The Unholy Confessions.** The merchant's *ascend* now confesses instead of buying lines. There are six, each with five ranks, bought a point at a time (10, 20, 30, 40 and 50 points a rank, 150 to the fifth), every point moving all of a Confession's numbers a step. At the fifth rank, at a class's full share:

- **Restoration**: Restoration, a spell (below), and potions restoring half again as much.
- **Agility**: Knocked Down, Stunned and Frozen 85% less likely and shorter; Dodge +10%, Movement +15%, Quiet +25%.
- **Spirituality**: half again as many souls from the dead; Unbound 85% less likely and shorter; Spell Haste +10%, spells 15% cheaper, Spell Effect +20%.
- **Constitution**: the sprint coming back half again as fast, and 2.5 seconds longer; 100 more health; and a 25% chance, once a level, to live a blow that would kill you, on one health and untouchable for three seconds.
- **Survival**: On Fire, Chilled and Poisoned 60% less likely and shorter, and grabs, webs and bile with them; Treasure +15%, Luck +2.5%.
- **Toughness**: 14 more armour and a quarter of what you wear again; and a shut secret's cracks give off motes you can see from as far as eight cells.

Each class takes each Confession at a share of its own, from half to the whole; which suit it is for you to find. The screen shows every number as you have it in all and as the next rank would leave it, and puts a pale blue lid on anything at its cap; D confesses a point (hold it to go on), A takes back what you confessed since the screen opened. Past the fiftieth Level of Atonement an overlevel takes a Confession past its fifth rank, its caps with it.

**What you were, in numbers.** Each class now starts with its own health and stats, its own element hurting it less and another more, and three caps lifted beyond everyone else's. Her question's second page (Space) shows them, as does the Codex.

| Class | Health | Brings | Strength, weakness | Caps lifted |
|---|---|---|---|---|
| The Witchfinder | 95 | Spell Haste +6%, spells 6% cheaper, Spell Effect +8% | fire 15% less, frost 15% more | Resist Burn 90%, Spell Haste 65%, Spell Effect 65% |
| The Anatomist | 105 | Armour +5, Physic +15% | frost less, fire more | Resist Slow 90%, Physic 520% |
| The Galvanist | 90 | Dodge +6%, Movement +6%, Quiet +15% | lightning less, frost more | Resist Lightning 90%, Dodge 45.5%, Movement 52% |
| The Plague Doctor | 100 | Luck +1%, Treasure +5%, sprint +0.5 s, every mark a tenth less likely | poison less, spirit more | Resist Poison 90%, Luck 32.5%, Treasure 130% |
| The Sin-Eater | 95 | mends 0.1 health a second, a tenth more souls | spirit less, poison more | Resist Spirit 90%, spirit carried 130, spells to 70% cheaper |
| The Headsman | 120 | Armour +4, sprint +1 s | its armour, lightning more | armour keeps off 90%, sprint 9.75 s over the base, Damage 130% |

**Armour has a cap**: it keeps off at most three quarters of a blow (the Headsman nine tenths).

**Restoration, the spell.** Once its first rank is confessed, 8 takes it up (it is last on the mouse wheel and the spells' wheel too). Hold the cast and after a breath it trades souls for health, a glow climbing your arm, while you walk at half pace: slowly at the first rank, a point of a hundred health every four seconds, to a point a second for a tenth of a soul at the fifth. A blow doesn't end it; letting go, a stun, a knock-down, being Unbound, the void or running out of souls do. Tab's caster page lists it with the rest.

This is still a beta. All of it has been tested in the harness, by the screensaver's own play, by scripted play and on sheets of the art: nobody has played it by hand yet, and it hasn't yet been weighed against 1.0.0b19 over the long runs. The Windows and browser builds are compiled and smoke-tested by GitHub Actions. If something goes wrong on your machine, please [open an issue](https://github.com/ASPDesignLabs/HAUNTED.EXE/issues) with your Windows version and graphics card.

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
