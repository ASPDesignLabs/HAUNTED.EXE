# Her hall: the merchant, the ledger and what death costs

Moved here word for word from `CLAUDE.md`'s *How CATACOMB works* when that file was slimmed (docs/PLAN-tidy.md, step 6); add to it as the game changes.
Where it names a function, search `src/crawl/` and `src/art/` for it: `scene_crawl.c` and `crawl_art.c` list their parts. Where it says
"above" or "below" it meant `CLAUDE.md` as it was: the save's format is `docs/SAVES.md`, the debug switches and the
harness `docs/HARNESS.md`, the known issues `docs/KNOWN-ISSUES.md`, and each release's account `docs/HISTORY.md`.

- **The merchant** (`gen_shop`, `begin_shop`, `update_shop`, `shop_human`). A room 11 cells wide
  with two to seven pedestals in an arc (`G.ped_x/ped_y`, `ped_arc`, `G.ped_deco`): `RUN.peds`, two for a
  fresh run, more bought from her menu (`ped_buy`, `PED_PRICE`: 500, 1,250, 3,125, 7,815, 19,530), kept
  through death; an old save (or a debug start past level 1) keeps what its level gave
  (`gear_shop_pedestals`, five to seven), and `CRAWL_PEDS=n` sets it. The autopilot buys one at its
  greeting when a level's haul (`gear_haul`) is left after. The wares: a spell, the
  rest things to wear (runes come only from the Rune Shrine now), a bag at most once a visit and only one bigger than yours. One to three
  wares a visit (`gear_shop_purchases`, `G.shop_left`); the autopilot buys up to that by value,
  keeping 15% of a level's haul back after the first. A person: E opens the ware's buying (`UI_BUY`, after 1.0.0b21.2: what
  changes, what becomes of the old), where E buys and wears it at once (`gear_take_wear`); with no bag a worn slot is warned
  of in red first (`G.shop_confirm`, a second E buys); with a full bag `UI_LETGO` asks which spare to let go. Q holds
  a ware for a quarter of its price (`Gear.held`, `G.shop_held`); next visit it is back on its
  pedestal at the price it showed (`G.offer_back`), and the hold ends when the caster leaves the room.
  Price tags over the pedestals skip any that would cover a nearer one. A click that buys a ware or
  pats the cat casts nothing until it is let go (`G.click_spent`), and taking control in the room
  leaves no cast half-made (`G.pc_t`). A card's price drops to the top line when the name is long (the dream's cards; a
  person gets a slim prompt, `render_shop_prompt`).
  The debug build logs `EVENT shop_look` as a person's eye moves between wares.

- **The Rune Shrine** (`SHRINE_X/Y` on the merchant's south wall, `RUN.coins`, `RUN.shrine` 0 sanctified to
  5, `RUN.attuned`, `RUN.slot_def/slot_q`; docs/PLAN-b6.md). `shrine_curse`, `shrine_raise` (`SHRINE_COST`
  1, 6, 12, 24, 32), `shrine_pull` (runes rolled into `G.sh_qk/sh_qq`; `shrine_step` brings them into
  the slots one by one and `shrine_combine` makes three alike one better; `G.sh_busy` while that goes
  on, ticked in `shrine_tick` whoever plays), `shrine_take`, `shrine_letgo`, `shrine_attune`. Takes
  (`G.sh_takes`) belong to the visit. The screen is `UI_SHRINE` (`update_shrinemenu`,
  `render_shrinemenu`; modes `SM_*`, rows `SR_*`); looking at it gives a card (`G.sh_look`). The
  autopilot goes there after the wares (`shop_after`, `SH_SHRINE_GO`, `SH_SHRINE`, `shrine_errand`,
  `auto_shrine_act`, `auto_make_room`, `auto_take`). Art: `SP_SHRINE_S/C`, the glow `SP_SHRINE_G0/1`
  (R_PURPLE remapped to the rank's colour), `SP_SRUNE0..8` and `SP_SRUNE_X` going round it, `SP_CCOIN`;
  `make sheets` draws them as `shrine.png`. The debug build logs `EVENT shrine_curse`, `shrine_raise`,
  `shrine_pull`, `shrine_combine`, `shrine_take`, `shrine_attune`.

- **Saves: the merchant's ledger** (docs/PLAN-b7.md sections 2, 3 and 11; `run_pack`/`run_unpack`,
  format above; the gear is packed field by field, so adding to `Gear` means adding to
  `gear_pack`/`gear_unpack` and a new version that still loads the old ones; `REC_KINDS` is fixed at 24
  because the record is packed by it). The app keeps `SLOT_ALL` slots (`core.h`): the three pages, then
  `SLOT_PROGRESS`, `SLOT_MARK` and `SLOT_PREFS` (the pad's layout since 1.0.0b18.1, `PL_BYTES`: "HVQ1", a version, each button's
  job, flags, the turning; the registry's binary `PadMap`, with the DWORD `PadSprint` beside it for the settings dialog; 1.0.0b15's
  single byte, the sprint's button, still reads); the platform loads them all before the first scene and writes any the
  game changed after each step (the registry's `Slot1`-`Slot3`, `Progress` and `Mark`; `SLOT_MAX`
  16 KB). Nothing is written on quitting: fleeing keeps only what is signed. `LEDGER` lasts while the
  program runs: `yours` (a person has taken over: the screensaver's dream is over; sleepwalking doesn't
  end it), `kept` (1.0.0b11: the run was read from the ledger as the program started, so the dream makes
  none of its lasting choices for it: no class, Alignment, ascending, bell or font), `page` (opened from or last signed onto; 0 for the unsigned run), and a page's visit
  (`Visit`, set out again by `begin_shop`). `ledger_first` reads the progress at the first
  `crawl_enter`. E at the cat opens her menu (`UI_MERCHANT`: sign, another pedestal, pat, leave); signing (`UI_LEDGER`, `led_mode` 0: `ledger_sign`) picks the page you came from first and
  asks twice over another run's (`RUN.run_id`, born at a fresh start, kept through death); opening
  (Esc, `led_mode` 1: `ledger_open`) always asks twice. Signing or opening points the progress at the
  page (`progress_point`). 1.0.0b6's graves are pages (shown CARVED, with their carved time after the
  blob) and its last fled progress is the unsigned run. Debug: `EVENT ledger_sign`, `ledger_open`, and
  `LEDGER start from ...`; `CRAWL_TPCAT=1` stands you before her.

- **Death's cost** (docs/PLAN-b7.md section 1; `death_fall`, `death_write`, `death_wake`). Only when the
  run is a person's (`LEDGER.yours`), no code was used and not in a pocket (`death_costs`); otherwise the
  old mercy (the level again, softer each time). At the fall: a `Stone` (newest first in `RUN.stones`,
  ten kept) with what was carried and what took it, then gold, souls, coins, every piece and the bag,
  every spell but the fireball, bones, the held ware and the ranks met (`RUN.top_rank`) go; the runes,
  the shrine, the Totem, scrip, steps, runs and the record stay. The page it came from (or the unsigned
  run's progress) is written at once (`PG_DEATH`, level 1 at the merchant's). A card (`render_card`,
  `G.dcost`, `G.dlost`) holds the haze until E (the autopilot: 6 s), then `death_wake` puts the dweller
  in the merchant's room before level 1. What hurt last is `G.blame` (`Blame`: `BL_MOB` kind and rank,
  `BL_BOSS`, `BL_HAZARD`, `BL_POISON`, `BL_FIRE`, `BL_POOL`, `BL_MIRROR`), set before each `hurt_ex`
  (`hurt_by(m, ...)` for a creature's blow; a missile carries its thrower's in `Bone.by`; poison keeps
  `G.poison_by`); `blame_text` words it. The plot's stones stand at `STONE_AT` (the newest
  `SP_TOMB_FRESH`); E at one opens its page (`UI_STONE`: the epitaph, the date, the spellbook and runes,
  each worn piece's card; A/D the next stone). A death in a pocket puts the dweller at the secret's
  entrance facing in and seals the chain's doorway for the level (`Secret.sealed`, `T_SEALED`; walking
  into it only knocks), with *THE DWELLER FAILED THIS TRIAL* and the tally's *1 TRIAL FAILED*
  (`G.trials_failed`). Debug: `EVENT died ... by what/type/rank`, `death_cost`, `death_written`;
  `CRAWL_TPSTONE=1` stands you before the newest stone. `make sheets` draws `SP_TOMB_FRESH` on `b7.png`
  and `T_SEALED` (with the Portal) on `b7walls.png`, for the owner.

- **Her claim** (docs/PLAN-b7.md section 2; `in_danger`, `update_mark`, `mark_write`, `mark_fits`,
  `claim_take`). While a person's run is in danger (an alerted creature within eight cells, a missile on
  its way, a wall skull charging, health under a quarter, poisoned, slowed or held fast, webbed, a foe's
  fire underfoot, a hazard's warning or harm, a boss arena while its fight is on; never in a pocket or at
  the merchant's) its page's entry in the mark (`SLOT_MARK`, `"HVM1"` and for each page, 0 the unsigned
  run: on, run id, signatures; `MARK_BYTES`) is set at once, and cleared 8 s after the danger ends
  (`LEDGER.calm_t`). A page opened (at the start or from Esc) whose entry still fits pays at once: a
  quarter to a half of the gold, and one worn piece or a spell a tier down (lost at tier 1; the Fire Bolt
  to the fireball, never below), each alike likely; she signs the page (`PG_CLAIM`) and her card
  (`render_claim`, `G.claim_card`) holds a person until E (the autopilot: 9 s). Opening another page
  leaves the marked one marked. A death clears its entry. While marked, a red eye (`dead_eye`) shows
  under the keys on the status bar and after them in the readout, and FLEE and OPEN THE LEDGER carry
  *THE DEAD ARE WATCHING*. An old save's run id is a hash of its bytes, so its mark holds across starts.
  Debug: `EVENT mark on|off`, `EVENT claim`.

- **The Corpus Compendium** (docs/PLAN-b7.md section 7; `comp_note`, `comp_add`, `comp_rows`,
  `comp_price`, `comp_buy`, `comp_auto`, `UI_COMP`). A tome on a lectern (`SP_LECTERN`, at
  `LECTERN_X/Y` in the back room). Every frame `comp_note` puts each worn piece not anointed into
  `RUN.comp[kind][quality]` (alike in kind, quality, ability and stats: kept once; twelve a page,
  `COMP_MAX`, the cheapest going for a better thirteenth), the bag by its quality, and each spell's
  highest tier (`RUN.comp_atk`). Kept through death. Its pages (A/D: bracers, boots, amulets, hoods,
  robes, rings, bags, spells) list them Common first with prices (the shop's x1.2, 0.05 less a run
  completed, never under half); Enter buys and wears it (asking twice when the worn one has nowhere to
  go), outside the wares a visit. The autopilot looks once a visit after the wares and buys the best
  thing worth a tenth or more (`gear_value` over the price's root, as `shop_choose` weighs), never a
  bag. An old save's worn pieces and spells go in as it loads. Debug: `EVENT comp_buy`, `comp_auto`,
  `COMP auto best ...`; `CRAWL_TPLECTERN=1` stands you before it.

- **The Wretched Repository** (docs/PLAN-b7.md section 7; `repo_rows`, `repo_froms`, `repo_put`,
  `repo_take`, `update_repomenu`, `render_repomenu`, `UI_REPO`). Iron doors in niches on the back room's
  west wall (`T_REPOSITORY`, x 3, y 13-16); E facing them opens it. `RUN.repo[]` (places, `RUN.repo_n`,
  two at first, ten more at `PLACE_PRICE`), `RUN.repo_gold`, `RUN.repo_coins`, none of it lost to death or
  the claim. A worn piece, a spare or an empty bag goes in for a tenth of its shop price (the first place
  free) and comes out free, into its slot if bare, else the bag; gold goes in less a quarter (A/D: all,
  half, a quarter, a tenth) and out free; a coin goes in free and comes out for 2,500 gold
  (`COIN_OUT`). The autopilot leaves it alone. Debug: `EVENT repo_put`, `repo row ...`;
  `CRAWL_TPREPO=1` stands you before it.

- **The Codex of Affixations** (docs/PLAN-b7.md section 9; `codex_note`, `codex_entries`, `update_codex`,
  `render_codex`, `UI_CODEX`, `KEY_C`). C opens it, in the crypt (which waits) and at the merchant's. Its
  parts: the abilities worn (`RUN.cx_abil`), runes held (`cx_rune`, and the Exquisite effect once held at
  Exquisite, `cx_exq`), the bosses' gifts once their boss has fallen, the stats worn (`cx_stat`, each with
  its cap and the kinds that roll it: `gear_stat_cap`, `stat_on_kind`), the hazards met (`RUN.omens`), and
  her claim, there from the start. A page each: what it does, its numbers by quality or rank, where it
  comes from, what you have of it now. A/D pages, W/S parts. Kept through death and with the page.

- **The Codex in 1.0.0b11** (step 15): `CX_LINE`, `CX_CLASS`, `CX_SOUL` (`CXS_*` pages; `RUN.cx_more`'s `CXM_SAP`, saved at the
  end of `LG_CODEX`), `CX_MAX`. The controls card's new lines are in `render_hud`'s `more[]`.
