Her Wares, Weighed: the third point release of the twenty-first beta of **HAUNTED.EXE**, a Halloween screensaver for Windows in real VGA modes: a possessed 486 PC (HAUNT.EXE) and a Wolfenstein-style crypt crawl (CATACOMB) that you can take over and play by holding **H**, or any button on a game controller, for three seconds.

It mends her rooms after a first look at the portraits: the candles say where to look, and her wares no longer hide the room.

### Before you upgrade

Your saves and settings carry over. Nothing in the saves has changed since 1.0.0b21.2.

### New in this release

**The portraits' candles stay cold until there is something to see.** A portrait's candles are unlit until one of its kind is slain and its tapestry can be drawn back, and burn for good after. A lit candle beside a tapestry still hung means there is one to draw back. The urn that stood in front of the skeletons' portrait, hiding its candle, now stands by the way on.

**A slim prompt in place of the cards in her rooms.** Whatever you face in her rooms shows only as one line at the foot of the view: the key, what it is, and a word more where one helps (a ware's price, gold waiting on a portrait, a gift at the Lampstand). Its card or menu comes up on the key, so the thing itself stays in sight.

**A ware's buying, on a screen of its own.** **E** at a pedestal, or at the map, opens it:
- the ware and its price, and what you wear in its place now;
- each of your numbers it would change, now and after, green where it rises and red where it falls, with the abilities gained and lost first;
- what becomes of the old piece: into your bag, lost if you have no bag (then it asks twice), or, with a full bag, which spare to let go.

**E** buys, **Q** holds it for your next visit, **A** and **D** go round her other wares and the map, and **Esc** closes it. On a controller: **A** buys, **Y** holds, the bumpers go round, **B** closes.

**Two smaller fixes.** The Rune Shrine's, the Totem's and the Compendium's passing words could come up blank. On a controller, the hint for going back read *B X*; it reads *B BACK*.

### What has been tested

All of it in the test harness: the prompts, the buying by keys and by a scripted controller (bought with a bag and without one, held, a bag bought, too little gold), and the candles cold and lit; the screensaver's own runs unchanged, event for event, against 1.0.0b21.2. Checked with AddressSanitizer. The Windows and browser builds were compiled and smoke-tested by GitHub Actions. None of it has been played by hand or run on Windows yet.
