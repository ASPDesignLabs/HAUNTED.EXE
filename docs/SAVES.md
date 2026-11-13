# The save's format

From `CLAUDE.md`'s *Building and testing*, moved here word for word when that file was slimmed (docs/PLAN-tidy.md, step 6).
How the ledger uses it (pages, the progress, the mark, signing and opening) is under *Saves: the merchant's ledger* in
`docs/catacomb/hall.md`. The code is `src/crawl/game/save.c`.

A save blob (for `HOST_LOAD`) is `"HVS1"` (u32 0x31535648 LE), version 7 (1.0.0b11; the same shape as version 6,
its changes in ledger sections), then little-endian int32
level, hp, gold, souls (in tenths), shop-first flag, then the gear packed field by field (`gear_pack`:
attack tiers and rune qualities, the seven worn pieces, bag size and quality, five bag places, bones, a held ware;
a piece is 24 bytes, `PIECE_BYTES`), then the run's `Record` (`rec_pack`: u16 kills by kind and rank,
u8 kills by kind and spell, u8 ranks seen by kind, u16 secrets, u16 descents, u32 levels cleared, 32
bytes of achievement bits; `REC_BYTES` in all), then four bytes for the L menu (the wisp kind, the
key's colour, the toggle, a spare), then 32 for the Rune Shrine (`shrine_pack`: u16 Cursed Coins, its
rank, u16 attunements, nine slots of rune kind + 1 and quality, u16 the level whose coins were last
taken and a byte of which (`RUN.coin_level/coin_bits`), six spare), then (version 6, runs of ten; it grows
as docs/PLAN-b7.md is built) the rest of the record (`rec_pack_extra`: u32 levels 33-64 cleared, 32 more
bytes of achievement bits, and for each of eight bosses its met flag, u16 slain, tries, best time and
scrip won), then the hall as far as it is built (`hall_pack`: u16 runs completed, u16 scrip for each of
eight bosses, a byte each for their challenge steps, and 24 bytes of the highest rank of each kind met
since the last death, a byte of omens named, a byte of each boss ability's rank and the one readied, u16 the furthest run opened): 1,145
bytes; then the ledger's sections behind the marker `"LDG1"` (`page_section`; a tag, a u16 length and the
bytes, to a tag of 0): `LG_PAGE` (u32 the run's id, u32 its signatures, u64 when written, a byte of why:
`PG_SIGNED`, `PG_DEATH`, `PG_CLAIM`) and, signed in the merchant's room, `LG_VISIT` (her visit: its
level, pedestals, wares left and bought, the shrine's takes, the ware held this visit, then each
pedestal's ware, `offer_pack`, and whether it is gone or back), and `LG_STONES` (the grave plot: a
count, then each stone, `STONE_BYTES`, 331: when, the level, what took it as a `Blame`, gold, souls,
coins, spells and runes, the worn pieces and the bag), `LG_PEDS` (the pedestals bought, a byte) and
`LG_COMP` (the Compendium: a count for each kind, bracer to bag, and quality, those pieces, then each
spell's tier), `LG_REPO` (the Repository: places, u32 gold, u16 coins, then each place's piece) and
`LG_CODEX` (four u32 of bits: abilities, runes, runes at Exquisite, stats; 1.0.0b11 a fifth, the stats past the 32nd), `LG_HALL2` (1.0.0b11: the bell and the font cursed, the levels they wait for, the secrets shown, a replay's
level of origin), `LG_ELEM` (version 7: the record by
element, a byte each by kind and element for kills, hits landed and harms felt; version 7 leaves the fixed part's kills
by spell empty, and an older record's go into their elements, a spell learnt staying learnt) and `LG_CASTER` (1.0.0b9,
`LG_CASTER_BYTES`: a byte of flags (1 the map, 2 the lantern shuttered), u16 the Level of Atonement, u32 the experience since the last
death, u16 points unspent, a u16 of steps bought in each of the lines (twelve, thirteen from 1.0.0b11 with Resist
Spirit's; a reader counts them from the length), a byte of overlevels in each, a byte of cornerstones, a byte for Second Wind spent, u16 the run it last came back on; an older page reads as
Level of Atonement 1 with no map), and `LG_PORTRAIT` (the portraits of the slain, docs/PLAN-portraits.md, `LG_PORTRAIT_BYTES`
277: a byte of how her room pays this life, `POR_*`, u32 the tapestries drawn back (a bit a kind, the bosses' from bit 24), the run each
kind's gem was unlocked in, a byte each, eight a kind, then each kind's gems taken this life and each boss's, a byte each, then each
kind's own count, u16 (`RUN.por_feat`; a 229-byte section, `LG_PORTRAIT_B4`, reads without them); an older page reads with its ranks
shown where they are common, tapestries down where it has slain and all its gold waiting): about
2,000 bytes with a little in the Compendium and the Repository and no stones, 16,153 at the most
(`PAGE_BYTES`, checked against `SLOT_MAX` as it compiles). A
reader steps over sections it doesn't know, so the hall's parts can come as sections of their own. The
progress (`SLOT_PROGRESS`) is either a whole run (unsigned: 1.0.0b6's, or a fresh one's) or `"HVP1"`, the
page to open (a byte, 1-3), the run's id and signatures. Version 5 is the same without version 6's part; its
descents of twenty become descents of forty on loading (`rec_from_old`). Version 4 is
the same without the shrine and with runes by tier (`rune_from_tier`: 1-3 Common, 4-6 Uncommon, 7-10
Rare, 11 up Ultra-Rare; a rune held at the merchant's is dropped and its hold fee paid back); version 3
is version 4 without the L menu's four bytes. Versions 1
and 2 (the old 46-byte raw `Gear`; version 1 without a record) still load: their worn robe, hood and best ring become Common
pieces of their tier (`gear_from_old`). A few lines of Python writing version 1 are still the quickest
way to start a test at a given level with given spells.
