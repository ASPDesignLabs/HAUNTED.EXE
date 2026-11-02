# The owner's words (2 November 2026)

Status: the editor is published and empty, waiting for the owner's first pass. Nothing in the game has changed.

The owner finds the game's wording "a bit too Claude-ish" and wants to put their own words in place of mine, with a way
to manage about two thousand strings. This plan treats their voice as a translation of mine: every string a player sees
is gathered, shown with my wording and a box for theirs, and later moved out of the C into a catalogue that the build
reads.

## What the owner settled (2 November)

1. **CATACOMB first.** HAUNT.EXE and the settings window wait. The project will be renamed throughout once this pass is
   done.
2. **An editor page, as an artifact.** Not text files: a page where they write, see their words in the game's font, and
   keep their place.
3. **Grammar.** Plurals and word order stay with the code for the first pass; after it, we look at how the code should
   handle them.
4. **Capitals.** CATACOMB stays in capitals.
5. **Habits, and the big things.** The strings that lean on my habits are marked. The creatures, bosses, classes,
   spells, runes and gifts have profiles of their own, with fields for traits the owner wants to become mechanics
   (damage types, what they throw, how they come at you). Those traits were never asked for when the work was handed to
   me, and the game's behaviour should follow them once they are written.

## The editor

https://claude.ai/artifact/RadNycWu96Z4reghCcoPqa (private to the owner; source `tools/words/editor.html`).

- **Strings.** 1,961 distinct strings, about 10,300 words, in 21 sections. Each card shows my wording and its preview
  in the game's own font (`font_bios8`, `font_vga16`), where it is drawn and how wide its space is, what fills each blank,
  the habits it leans on, and the places it is used. The owner writes underneath; the preview and the checks (blanks,
  width, characters outside the font, habits in their own text) follow as they type. *Keep mine*, *Flag* and a note.
- **Profiles.** 20 creatures, 4 bosses, 6 classes, 6 spell lines, 9 spells, 9 runes, 16 gifts and abilities. Each has
  facts read from the tables (`KINDS`, `BOSSES`, `CLASSES`, `ALIGN`, `ATK_*`, `DEF_*`, `BA_NAME`, `CS_*`, `ABIL_NAME`),
  a line on what it does now (written by hand in `tools/words/profiles.py`; keep it true), fields for the owner's
  design, and its names and descriptions as editable strings. For creatures, an element choice that differs from the
  game's turns the line under it amber.
- **Where it is saved.** The page's database. `words/<id>`: `{text, keep, flag, note, mine, section, at}`, `mine` being
  my wording when they wrote. `traits/<profile key>` (`creature.skel`, `boss.deacon`, `class.headsman`, `line.fire`,
  `spell.fire_bolt`, `rune.blink`, `gift.grave_call`, `corner.premonition`, `ability.keymaker`): the profile's fields,
  elements as arrays of `FIRE` ... `PHYSICAL`. Only the owner and editors write (`{path: "", read: "view", write:
  "admin"}`). Read it from a session with `ArtifactData` `list` on each collection.

### Refreshing it

`make words` (or `python3 tools/words/extract.py`) writes `build/words/catalogue.json` and `profiles.json` from the
sources. Republish with the Artifact tool: `file_path` `tools/words/editor.html`, `url` the editor's, `files`
`{"catalogue.json": "build/words/catalogue.json", "profiles.json": "build/words/profiles.json"}`, capabilities left out
so they carry over. Do it whenever strings are added or changed while the pass is open, so the owner sees them.

### Ids

An entry's id is a slug of my wording and four hex digits of its SHA-1 (`the_web_tears_away_from_it-1a2b`), so it
survives code moving about but not my wording changing. A row whose id no longer exists still holds `mine`, so it can be
matched to the new entry by hand. Don't reword strings while the pass is open without saying so to the owner.

### What the extractor counts

Every literal in `src/scene_crawl.c` and `src/crawl_gear.c`, less: anything in `#ifdef CRAWL_DEBUG`, `fprintf`,
`getenv` and the like, the console (`con_*`), pixel art and map legends (`is_art`), save tags, and lower case (CATACOMB
writes in capitals). Identical strings are one entry with several places. A printf blank becomes `{1}`, `{2}`; its fill
is the argument's code, and a ternary of two short literals is a plural ending. A face button's `\1` shows as `^`
(`^A`). Widths are from the call: a caption 40 letters (centred, one line), a pop-up 33, a hint or text from its x,
`text_wrap` its columns and lines (and for a table drawn by `text_wrap`, the table's: `table_wraps`). Most names and
assembled text have no width the code states; the editor wraps long ones at 36 softly and says so.

### Habits (pattern matches, `HABITS` in `extract.py` and `HABIT_RX` in the page)

Paired clauses on a comma or semicolon; aphorisms (short lines beginning NOTHING, NONE, ALL..., or ending ALREADY,
AGAIN...); a label then the point after a colon; THE three times or a name beginning with it; clauses hung on AS; the
place or the dead as the one who acts; X OF THE Y names; old-fashioned words (LEST, TILL, ROUND, A WHILE); more than 25
words. 556 of the 1,961 are tagged.

## Found on the way

Eight captions are wider than the screen's 40 letters and are cut off at both edges (`say` centres them; two are also
past its 64-character buffer): the scales' `LEVEL, BUT THE JUDGED IS NOT IN THE LEFT PAN` (line 9952), `THE GLASS
CRACKS, AND THE FLOOR GOES DARK` (10448), `THE CART CLANGS INTO A DEAD END AND ROLLS BACK` (10729), `THE CRYSTALS FALL
SILENT, THEN SING AGAIN` (11068), `SHE SETS OUT THE SEVENTH PEDESTAL, THE LAST` (13755), the trial's boon and curse
(11793, 11799, 67 letters) and `IT ANSWERS ONLY WHILE THE GREAT BELL TOLLS` (11832). Left as they are: the owner is
rewording them, and the editor flags the width.

## After the first pass

1. Read `words` and `traits`. Report what was written, what was kept, what was flagged, and every check that fails.
2. **The catalogue in the code** (the approach proposed on 2 November): `text/*.txt`, one file per section, each entry
   its id, where, width and text; a small generator built by `make` into a header; the code calls `TX(ID)`. Done area by
   area, each a commit, each leaving the harness's frames identical for every string kept as mine. Debug output and the
   console's codes stay out of it.
3. Put the owner's words in, with the build-time checks the editor already makes.
4. Grammar: decide with the owner how plurals and word order are written in the catalogue, from what the first pass
   needed.
5. **The traits.** The profiles' fields are a brief for mechanics. Turn them into a plan of their own (questions first,
   as with every big feature), compare with what each creature does now, and change behaviour only at the owner's word.
6. HAUNT.EXE and the settings window; then the project's new name.
