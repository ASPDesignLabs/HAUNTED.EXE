The Portraits of the Slain: the second point release of the twenty-first beta of **HAUNTED.EXE**, a Halloween screensaver for Windows in real VGA modes: a possessed 486 PC (HAUNT.EXE) and a Wolfenstein-style crypt crawl (CATACOMB) that you can take over and play by holding **H**, or any button on a game controller, for three seconds.

It hangs a portrait of every creature you have slain on the merchant's walls, and pays you for them.

### Before you upgrade

Your saves and settings carry over. This release adds the portraits to each page of the ledger: which tapestries are drawn back, where each rank was earned, the gold taken this life, and the feats' counts. 1.0.0b21.1 steps over them, and a page it signs loses them. The achievements for kills now ask fewer (below); any your record already reaches are earned as a page is read, under one pop-up, and achievements earned stay earned.

### New in this release

**The portraits of the slain.** Her walls carry a portrait of every creature in the bestiary: the first two thirds in her room, the last in the back room with the Totem.
- Each hangs under an oxblood tapestry until one of its kind is slain. Then **E** draws the tapestry back, to be tied at the side for good, and behind it the creature's shadow walks on the spot.
- Each rank whose achievement you earn then walks there in that rank's colours, on the stone of the run where you earned it, the ranks taking turns. The frame takes the colour of the highest: bone, green, blue, red, gold for the gilded, black for rank V. A gem in its foot lights for each rank, and a candle lights each portrait, set seven different ways.
- **Every rank unlocked pays its gold once a life**, at any visit to her room once a level has been cleared in that life, and again on waking after a death that costs. A gem whose gold waits twinkles. **E** at a portrait opens its card: each rank, slain against what it asks, its gold, and whether it is taken, waits or is still to come. **E** takes all that waits, and **A** and **D** go through the rest. A rank pays from 100 to 1,200 gold, by the creature, its rank and its strength.
- **The Spectre and the Wraith**, which can't be slain, open theirs too: the Spectre laid to rest by Spirit 5, 15 and 40 times, the Wraith driven back 10, 40 and 100 times.
- **A feat for each creature that can be slain**, an achievement of its own, hung as a gold medallion under its frame and paying as its rank III does. Some of them: skeletons slain before they ever throw, wall skulls silenced before their scream goes out, web spiders still in their webs, knights still in full plate, ghouls at their meal, gargoyles as they wake, the Sexton with his bell unrung, the Lightless with your lantern shuttered. In the bestiary, **W** and **S** go past a creature's ranks to its feat.
- **The bosses' four** hang in the grave plot, two faces wide, either side of the Accursed Portal, each in its own arena under two tapestries until it has fallen. They pay for the fall and for each step of its challenge, from 1,500 for the knight's fall to 9,000 for the Wraith Unbound's fifth step.

Everything on the walls together comes to 175,240 gold a life. The screensaver draws a tapestry back now and then, but takes no gold.

**Fewer kills for each rank's achievement.** 20 of a creature's lowest rank, then 25, 20, 15 and 10, and 5 gilded (it was 60, 40, 25, 15, 8 and 3).

### What has been tested

All of it in the test harness: every portrait's gold taken in turn, matching the numbers settled for it row for row; gold taken, a death, and the gold owed again on waking; the feats counted in the screensaver's own play, eleven of the twenty-two seen there; and the screensaver's runs unchanged, event for event, against 1.0.0b21.1. Checked with AddressSanitizer. The Windows and browser builds were compiled and smoke-tested by GitHub Actions. None of it has been played by hand or run on Windows yet. With the portraits in view, a frame in her room takes about a millisecond longer to draw on the test machine.
