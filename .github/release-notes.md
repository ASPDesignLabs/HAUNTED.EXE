First Steps: the fifth point release of the twenty-first beta of **HAUNTED.EXE**, a Halloween screensaver for Windows in real VGA modes: a possessed 486 PC (HAUNT.EXE) and a Wolfenstein-style crypt crawl (CATACOMB) that you can take over and play by holding **H**, or any button on a game controller, for three seconds.

It lays out the first run's first two levels by hand, and teaches on them as you go.

### Before you upgrade

Your saves and settings carry over. Nothing in the saves has changed since 1.0.0b21.4; the settings gain a byte for the lessons seen, which earlier betas ignore. If you have played before, the lessons count as given and won't stop you; ask for them under the title's **HELP**.

### New in this release

**The first two levels are drawn.** The same every time, for everyone, the screensaver included.
- **Level 1**: a door opens on lamp oil burning across the hall. Walk through it; it hurts enough to want mending, and a place to mend waits at the hall's end (the dead hear you mend). Then a hall of skeletons for your spell, a gold key in a side room, a vault with two gold doors and one lock between them, and the way out past it.
- **Level 2**: more of the dead (skeletons, shamblers under turned earth, bats), ten things that want a spell, and a room at the back behind an **ice field** that holds what stands on it. A **rimed seal** in that room's far corner takes the ice down after three strikes of any spell, and can be struck from the ice's edge. **The map** lies by the ice: it is yours for good, and as you take it the ice comes back. The way out of that room is a **gate that opens only from its side**, by the seal on it. Two keys, gold and crimson, each spent on its own door, and a **secret** by the gold key whose finding pays the run's first Cursed Coin.

**The lessons.** On your first run on the computer, these levels teach as they go.
- A page cut in stone stops the crypt at a moment (the first door, the fire, mending, your class's spell, the first soul, a key and a lock, the ice, the map, a secret's cracks) and says what is happening, in your keys, your controller's buttons or touch's stones, whichever you are using. There is no skip.
- Now and then the view is turned for you: towards the place to mend once you are through the fire, where a free wisp kindles to lead you; towards the hall your spell is for; back from a door you have no key for yet; off the ice before the switch is struck; towards the ice as it comes back behind the map, then the gate. Nothing can hurt you while the view is held.
- While the lessons run, a death costs nothing (the level starts again), the merchant keeps the map off her wares until level 2 is behind you, and level 2's way out stays shut until the map is yours.
- **HELP** on the title screen turns *THE LESSONS ON A NEW RUN* on or off, once you have had them.

### What has been tested

In the test harness only: not yet played by hand, run on Windows or seen on a real screen. A scripted player went through every lesson on both levels, each from its own place, with the keyboard; the lessons in a controller's buttons and touch's stones were checked on their sheet, not in play. The screensaver cleared both levels on every seed tried, with no deaths or stalls there. Its runs from level 1 were not measured against 1.0.0b21.4 over many seeds; the first two levels are new, so its early numbers will differ. Clean under AddressSanitizer. The Windows and browser builds were compiled and smoke-tested by GitHub Actions.
