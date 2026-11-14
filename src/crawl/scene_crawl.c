/* CATACOMB - a Catacomb 3-D / Wolfenstein 3-D style crawl through generated
 * crypts, in VGA mode 13h (320x200, 256 colours).
 *
 * A grid raycaster with sliding doors, torch-lit walls (light spreads through
 * open space, not through walls), billboard sprites, and an autopilot that
 * explores with hard 90-degree turns.  Each level opens in the merchant's
 * antechamber (the owner's cat on a chest), where the caster buys one spell,
 * rune or piece of gear with its gold; crawl_gear.c holds the wares.  In the
 * crypt it detours for treasure it has seen or a key it can hear, opens the
 * gold, silver and crimson vaults, and leaves through the exit for a tally.
 * Skeletons throw bones, bats bite, wall skulls gather fire, shield-bearers
 * swing swords and spiders drop from their webs; wraiths only loom.  The hand
 * casts whichever spell suits the target, and its tattooed runes defend it on
 * their own.  HP, gold, souls and the face show only in pop-ups that slide in
 * when something changes, so nothing sits still on an OLED. */
#include "crawl.h"
#include "crawl_gear.h"
#include "gfx.h"
#include "snd.h"
#include "crawl_ui.h"
#include "crawl_title.h"
#include "crawl_portrait.h"
#include "crawl_tutor.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include "build/version.h"

/* From here the file is cut into parts (docs/PLAN-tidy.md), each #included in the place it was written in. It is still one
 * translation unit: a part sees everything in the parts above it, and none is compiled on its own. The game's parts are in
 * game/, the drawing's in draw/. This list is the map: to find a thing, read down it. */

/* Sizes and paces; the kinds of cell, creature, item, missile and harm; cells, doors, lights; the creature (Mob); the contracts between combat's two tracks; packs and ambushes; KINDS and the ranks' tables */
#include "game/defs.c"

/* The bosses' arenas and BOSSES; hazards, things, puzzles and trials as structs; shots, secrets, items; the levels' names; the record, a stone, the Alignment's lines; RUN; the console's and the ledger's state; G */
#include "game/state.c"

/* IN and the pad (PADX), touch's and the wheels' state, the pad's layout (PL) and its saving, rumble, the hints in the pad's words (pk) */
#include "game/input.c"

/* The pockets' state (PK, G_LEVEL); a creature's blow by level; ATTACKS; footprints; what a cell blocks (light, a body, a step) */
#include "game/attacks.c"

/* Level generation: carving, themes and room types, decorations and static lights; the plan (maze, vaults, widths); secrets placed; the sliding wall's box */
#include "game/generate.c"

/* A drawn run's walls by face: materials by the stretch, blends, what stands on the wall (lay_faces) */
#include "game/faces.c"

/* gen_level itself (with con_carve and gen_dress), then the dressing: props and their footprints, crystals, fires, litter (dress_level) */
#include "game/gen_level.c"

/* What a level is given: the arena stamped, treasure, keys and the exit, and the creatures that wait in their places (wall skulls, spiders, the buried, imps, gargoyles, guardians) */
#include "game/place.c"

/* The light map: dynamic lights, the torch's flicker, build_lightmap */
#include "game/light.c"

/* Lines of sight, ranks picked, spawn_mob, what comes on a level (random_type), wanderers, the file and the roost placed, the readout's pop-up */
#include "game/spawn.c"

/* Harm that reaches the caster: its one road (el_resist, armour_keep), the caster's statuses (af_*), hurt_ex, the dead's blows and bones (foe_hit, throw_to, update_bones); then souls: the vessel, motes, spirit spent */
#include "game/harm.c"

/* The record and its achievements, the portraits' part of it, experience, Levels of Atonement */
#include "game/record.c"

/* The caster's own: the Unholy Confessions, the Alignment (align_*, caster_sync), confessing, the classes' sheets and caps */
#include "game/caster.c"

/* The spells' augments: the other five lines' (augd_*), then the frame and Fire's (aug_*, fire_*) */
#include "game/augments.c"

/* Harm that reaches the dead: kill_mob, weaknesses and strengths, elemental creatures, damage_mob, the dead's statuses and their gate (mob_gate), trails, venoms, blasts; the spectre; the hunt field (build_hunt, alert_near) */
#include "game/damage.c"

/* How the dead think: senses and memory, packs (calls, turns, flanks, morale), roles and rounds, hunters on the trail, the dead's ways named and their deeds */
#include "game/minds.c"

/* Lures and ambushes: kill rooms, teeth, bait, the spring */
#include "game/ambush.c"

/* Noise and who hears it, alarms, the Sexton's bell, searching, mind_tick, mind_goal, mind_frame, footsteps */
#include "game/noise.c"

/* Each kind's own ways: the Wraith (and the wards' field it walks by), the buried, pools, wall skulls, spiders and leaps, sacs, shades, gargoyles; then the bosses' own: the knight, the spider, the Wraith Unbound */
#include "game/creatures.c"

/* Cunning by rank (dodges, cover, doorways held, feints), and the new attacks (the Bone Thrower, the charge, the spew, the grapple) */
#include "game/cunning.c"

/* update_mob: one creature's step, every kind's branch */
#include "game/update_mob.c"

/* The caster's spells: shots, lightning, the BFS9000, the flask, cast_spell; charging and the ultimates; update_shots */
#include "game/spells.c"

/* The caster's defences: runes, each waiting for its moment (def_trigger, cast_defense), update_status */
#include "game/runes.c"

/* The balance sheet (crawl_combat_sheet); a creature's blow; the bat's latch; the caster's boss abilities (web, Grave Call and allies, Wraithwalk, Swift Dodge); runes raised by hand; doors (update_doors) */
#include "game/abilities.c"

/* The dream's goals: secrets pushed, keys, the altar, chains, a boss, motes; places to mend */
#include "game/goals.c"

/* The dream (the autopilot): its counters, pick_goal, decide, its steps and turns, find_target, choose_spell, aim, update_player */
#include "game/dream.c"

/* A level's creatures placed: the caster's state reset, the debug map, the Black Altar's place, the old placements, bands, and the encounter budget (place_creatures) */
#include "game/budget.c"

/* Things that want a spell: their kinds, where they go, what a strike does, sequences and gates, things for fights; the lull's watch */
#include "game/marked.c"

/* Drawn levels (the tutorial's two): the maps, gen_drawn, their items, dead and marks; the burning floor; the ice */
#include "game/drawn.c"

/* The mice (docs/PLAN-mice.md): their holes, their days, scurrying, the holes as ways through, the tell; the charnel rats told */
#include "game/mice.c"

/* start_level: a level begun; the debug build's gear and gifts */
#include "game/start_level.c"

/* Pockets: the caster carried across, gen_pocket, the way in and back; crawl_enter; collect (what picking a thing up does) */
#include "game/pockets.c"

/* Hazards: the bosses' six, the runs' own by element, what they mark for the dream, their harm to the caster and to the dead, update_hazards, af_tick */
#include "game/hazards.c"

/* The fixed hazards: where they stand (place_sites), update_sites, the arising ones, their lights */
#include "game/sites.c"

/* Ways to touch the world: things carried, set down and used (thing_use) */
#include "game/things.c"

/* Puzzles: which a level has, where the pieces go, solved and wrong, the shapes they take; the Crypts' (pz_make_run1); pz_make, pz_touched; the dream at a puzzle */
#include "game/puzzles.c"

/* The Catacombs' puzzles, the Hollows', the Deep's, and the two of any run */
#include "game/puzzles_runs.c"

/* The hidden message, Tab's documents, the Black Shrine */
#include "game/message.c"

/* Trials: kinds, waves, tri_begin, tri_run, the reward, tri_end, slow work, where a trial is placed */
#include "game/trials.c"

/* The soul-lamps: laid with the level, lit by souls, ward_tick */
#include "game/lamps.c"

/* The dream at a trial (tri_next, tri_goal); omens; a boss's hazards */
#include "game/omens.c"

/* A boss's fight: fonts, boss_begin, waves and adds, boss_fall, update_boss */
#include "game/boss.c"

/* The Black Altar: its wisp called home, update_altar, waking, sending, blessing, anointing; update_abilities */
#include "game/altar.c"

/* The map's noting (map_note), items and the tally, the hand raised, wisps (targets, routes, summon_wisp, update_wisps), the L list's choices, the altar's rows, undead turned */
#include "game/wisps.c"

/* The debug build's stages (CRAWL_MEET, CRAWL_PACK, CRAWL_STAGE), and crawl_update: the frame's order of things */
#include "game/update.c"

/* The merchant's rooms: her wares' sprites, the pedestals' arc, the portraits hung, gen_shop, the dream's shopping, begin_shop, purchases */
#include "game/merchant.c"

/* Her hall's things: the Rune Shrine, the Hexed Totem, the Accursed Portal (runs, replays, challenges), the bell's and font's errands, update_shop */
#include "game/hall.c"

/* A person at the controls: captions, doors, the exit and the reckoning, E (human_use), the body against walls, the target looked at; Restoration; the wheels; the sprint, human_walk, open_ui, human_cast */
#include "game/human.c"

/* The lessons (TU): their moments, the view turned, the page; update_human */
#include "game/lessons.c"

/* A person in her rooms: the portraits in play, shop_human, the full bag's question, the buying; taking over and letting go (human_begin, human_end); Tab's actions; Esc's question and a new run; the debug build's teleports */
#include "game/her_rooms.c"

/* The console */
#include "game/console.c"

/* The pad into keys (pad_keys), touch into keys (touch_keys), crawl_input, crawl_control */
#include "game/controls.c"

/* Saves: the gear, the record, the hall, the shrine and the stones packed and unpacked, a page's sections, run_pack, run_unpack, the progress, the mark, ledger_first */
#include "game/save.c"

/* The ledger's pages (signing, opening); the title screen's logic (TI); death's cost; her claim and the mark; the ledger's and the cornerstones' menus */
#include "game/ledger.c"

/* The menus' logic: the Alignment and its gifts and augments, the class, the bell and font, confessing and ascending, her menu, a stone, the L list, the altar, the shrine */
#include "game/menus.c"

/* More menus' logic: the Compendium, the Repository, the Codex, the Totem, the Portal; the map's keys; update_ui */
#include "game/menus_hall.c"

/* Rendering begins: shading and fog, the floor and ceiling (render_floor), secrets' cracks, the floor's marks (hazards, omens, wards, ice: fm_pixel) */
#include "draw/floor.c"

/* The walls: the sliding secret's ray, render_walls */
#include "draw/walls.c"

/* Sprites: eyes and vapour, beams, the cat, which sprite a creature shows (mob_sprite, mob_side), render_sprites */
#include "draw/sprites.c"

/* Her wares' tags, the portrait's face, glyphs, the status bar, text wrapped, her prompts and cards, the wards' rings */
#include "draw/hud.c"

/* The hand: built and posed, the charge, Restoration's glow, render_hand; the latched bat, the vessel's slip, wisps */
#include "draw/hand.c"

/* The carved stone's hooks; cards (the level's, a boss's, her claim), the web, the boss's bar, the trial's glass, the tally, the readout, the statuses, sockets, the sprint, the view's tint, what is carried and read, messages */
#include "draw/cards.c"

/* Tab's and her hall's frames; a piece's card, the full bag's question, the buying, Tab's pages (elements, the caster, documents, the wardrobe), the Black Shrine, Esc's question */
#include "draw/tab.c"

/* The controller's screen (its logic too) and the look; the ledger, her menu, a stone's page */
#include "draw/screens.c"

/* Pop-ups (toasts); the bestiary: its logic and its pages */
#include "draw/bestiary.c"

/* Menus drawn: the L list, the altar, the shrine, the Compendium, the Repository, the Codex, the Totem, the Portal */
#include "draw/menus.c"

/* Experience and Second Wind shown; the target's health; the cornerstones, the Alignment, the bell and font, the class; the Confessions and ascending */
#include "draw/caster.c"

/* The map on its scroll */
#include "draw/map.c"

/* render_ui (which screen is drawn); the wheels; Toughness's cracks; the warded; the things that want a spell */
#include "draw/overlays.c"

/* The portraits of the slain, drawn: what each shows, their overlays, their cards */
#include "draw/portraits.c"

/* crawl_render: the frame's order; the music's side (music_frame); touch's stones (crawl_stage); the scene itself */
#include "draw/frame.c"
