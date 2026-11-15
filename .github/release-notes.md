Small Life: the sixth point release of the twenty-first beta of **HAUNTED.EXE**, a Halloween screensaver for Windows in real VGA modes: a possessed 486 PC (HAUNT.EXE) and a Wolfenstein-style crypt crawl (CATACOMB) that you can take over and play by holding **H**, or any button on a game controller, for three seconds.

It fills the crypt with small things that live in it, and notice you.

### Before you upgrade

Your saves carry over, and nothing in them has changed. The settings gain two entries (below), which earlier betas ignore. A copy of your saves costs a moment, and this is as good a time as any; from a Command Prompt:

```
reg export "HKCU\Software\HalloweenVGASaver" "%USERPROFILE%\Desktop\haunted-saves-backup.reg"
```

### New in this release

**Mice.** They live in the walls of every level but her hall, the Backrooms and Severance: a few at a run's start and more by its end. They wander, cannot open doors, and go through holes in the walls, which is quicker than walking. They gather and preen by warm lights, and run for the nearest hole when you frighten them. They grow every two levels, most of all at the third level of the second run, and their chirps deepen as they do. Their eyes are green and bright from anywhere. They cannot hurt you and you cannot hurt them, until later.

**They tell.** A mouse that sees you carries word of you to the next through the holes, and a mark shows over each that carries it.

**The charnel rat.** From the fifth level of the second run the grown ones are rats the size of a hound. A mouse that reaches one tells it where you are, and it comes. It is the twenty-fourth kind in the bestiary, which is now full.

**Spiders on the walls.** Small ones crawl the stone of every level, never leaving it, round the corners and past the doors, open or shut, and round whatever is painted on the walls. They hurt no one. When you come near they run to tell the nearest web or hunting spider, or hide in a web in a corner.

**The crypt's small life.** Twelve things that notice you, and ask nothing of you:
- Moths circle the warm lights, and flies where the level is foul; they scatter as you pass and drift back.
- Torches gutter as you pass, and harder when a door swings near them.
- Bats roost in the corners of the first ten levels. Some drop as you come near, circle the room and settle somewhere else. (From the eleventh level a roost of the dead's own is a warning, and these keep out of its way.)
- Beetles and woodlice scuttle from you into cracks in the stone, and crunch if you tread on one.
- Water drips into puddles and rings spread; step in one and your boots leave wet prints that dry.
- Toads sit by the water from the sixth level and hop off with a croak.
- The saints of the second run follow you with their eyes.
- Eyes open in the dark of some burial niches, and shut when your light finds them.
- A hand on the sill of a niche settles as you pass.
- Chains, lamps and the rest swing after you.
- Webs shiver as you pass, tear if you walk through, and grow back.
- Glowing fungus breathes as you near, and sends up spores.

**Two new settings.** *Small life: movement* and *Small life: sound*, each *On*, *Reduced* (only what keeps still) or *Off*, under Settings. Off draws and sounds exactly as before. The browser has no menu for them yet and plays with both on.

**A smaller download.** The program's source was tidied this release, and the download is much smaller for it. Nothing in the game changed with it.

### What has been tested

The mice and the spiders' first build were played by hand by the author on their own Windows PC, and mended after: the mice at first span on the spot when frightened, and the spiders floated off the walls. Everything since is tested in the test harness only: the spiders kept to the stone and round the pictures, the charnel rat's growth, and all the small life. None of it has been played by hand, none of the new sounds has been heard (the drip, the crunch, the croak and the rest borrow other effects), nothing has been run under Wine, and the Windows settings dialog and the browser build were compiled and smoke-tested by GitHub Actions and by nothing else.

In the harness the screensaver's runs from level 1 to 10 log line for line as the build before, and eight seeds of 3,000 seconds from levels 11 and 15 died within the noise of it (154 deaths against 155, and 271 against 250, of which the extra are the charnel rats'). The small life moves nothing of the game: its runs match the build before line for line, and with it switched off the frames match to the byte. Clean under AddressSanitizer.
