/* The Compass (the owner's Map 3, drawn in the World Editor as "Storage"): level 3 (docs/PLAN-mapedit.md) */
static const char *const DRAWN3_MAP[43] = {
    "########+++#+++#+++#############",    /*  0 */
    "######+##D###D###D##+###########",    /*  1 */
    "######+++++++++++++++###########",    /*  2 */
    "####+#+#############+#+#########",    /*  3 */
    "####+D+#+++#+++#+++#+D+cqqqqqqq#",    /*  4 */
    "####+#B##D###D###D##B#+#qqqqqqq#",    /*  5 */
    "######+.+++++++++++++###########",    /*  6 */
    "######+.+##D###D##+..###########",    /*  7 */
    "#####.+++#+++#+++#+...####++++++",    /*  8 */
    "####....+#########+....###++##++",    /*  9 */
    "#####D##+++++++++++##D###+++##++",    /* 10 */
    "####...######D######...#+++###++",    /* 11 */
    "#####c######.+.########+++##+#++",    /* 12 */
    "####qqqq##h#.+.########++##++#++",    /* 13 */
    "####qqqq##...+...####++++#+++#++",    /* 14 */
    "####qqqq##+++++++####+####++##++",    /* 15 */
    "########..+#...#+..##+++++++##++",    /* 16 */
    "########..+#...#++.###+++++.##++",    /* 17 */
    "S#####.++++#####.+++.#########++",    /* 18 */
    "+#####.+####...####+.######..+++",    /* 19 */
    "+###...+..#.....#..+.###++++++++",    /* 20 */
    "+###.+++..#..+..#..+++++++..##c#",    /* 21 */
    "+++D++.+..#..+..#..+.###..###qqq",    /* 22 */
    "######.+####.+.####+.########qqq",    /* 23 */
    "######.+++.##B##.+++.########qqq",    /* 24 */
    "########.+.#++.#.+.###########q#",    /* 25 */
    "########.++#.+.#++.#############",    /* 26 */
    "##########++.+..+###############",    /* 27 */
    "##########.++++++###############",    /* 28 */
    "############.+.#################",    /* 29 */
    "############.+.#################",    /* 30 */
    "#############D##################",    /* 31 */
    "############+++#################",    /* 32 */
    "############+#+#################",    /* 33 */
    "##########+++++++###############",    /* 34 */
    "##########G#...#C###############",    /* 35 */
    "########+++.###.+++#####h#######",    /* 36 */
    "########+#+..#..+#+#####qqq#####",    /* 37 */
    "########+.+#####fFf#####qqq#####",    /* 38 */
    "#####qq#+.+#####fFf#qqq##q######",    /* 39 */
    "####qqqc+.+#####fFfcqqqqqqqq####",    /* 40 */
    "#####qq#+.+#####fFf#qqq#########",    /* 41 */
    "########fff####Xfff#############",    /* 42 */
};
static const DrawnThing DRAWN3_THING[] = {
    { DT_KEY, 0, 28.63f, 12.38f }, { DT_KEY, 2, 4.38f, 3.38f }, { DT_COMPASS, 0, 13.38f, 21.63f },
    { DT_SWITCH, 0, 27.38f, 13.38f }, { DT_DEAD, MOB_BAT, 8.13f, 20.63f }, { DT_DEAD, MOB_SPIDER, 8.13f, 26.63f },
    { DT_DEAD, MOB_WSKULL, 12.13f, 25.63f }, { DT_DEAD, MOB_WSKULL, 14.88f, 25.63f }, { DT_RELIC, 0, 18.63f, 0.38f },
    { DT_SOCKET, -1, 12.88f, 25.13f }, { DT_LEVER, 0, 6.13f, 1.38f }, { DT_PLATE, 0, 20.38f, 6.38f },
    { DT_PLATE, 0, 20.38f, 4.38f }, { DT_LEVER, 0, 9.38f, 41.63f }, { DT_SPOT, 0, 18.63f, 4.38f },
    { DT_SPOT, 0, 12.38f, 4.13f }, { DT_SPOT, 0, 14.88f, 0.13f }, { DT_SPOT, 0, 16.38f, 0.63f },
    { DT_SPOT, 0, 10.63f, 0.13f }, { DT_SPOT, 0, 4.13f, 5.63f }, { DT_SPOT, 0, 12.63f, 8.63f },
    { DT_SPOT, 0, 14.38f, 8.63f },
};
static const DrawnRoom DRAWN3_ROOM[] = {
    { 6, 14, 15, 15, RT_KNIGHTS },
    { 11, 19, 5, 5, RT_BURIAL },
    { 8, 4, 11, 1, RT_CISTERN },
    { 8, 0, 11, 1, RT_CISTERN },
    { 10, 8, 7, 1, RT_CISTERN },
    { 8, 36, 5, 7, RT_OSSUARY },
    { 14, 36, 5, 7, RT_OSSUARY },
    { 4, 3, 1, 3, RT_CISTERN },
    { 22, 3, 1, 3, RT_CISTERN },
};
/* spawn points: x, y; the kind or -1 the budget's choice; the chance %; the element's chance % or -1 by rank; the element or -1;
 * the rank or -1 the level's usual; 1 + its wake group (0: filled as the level is laid); 1 + the tag of the spots it is laid at (0: here) */
static const DrawnSpawn DRAWN3_SPAWN[] = {
    { 8.88f, 21.63f, -1, 100, -1, -1, -1 }, { 13.38f, 20.13f, MOB_SHIELD, 100, -1, -1, RK_GILDED },
    { 13.38f, 34.63f, MOB_SHIELD, 100, -1, -1, RK_II, 1 },
};
/* torches: x, y of the wall in the drawing; the face (0 east, 1 south, 2 west, 3 north); the group (0 path A); lit at the start */
static const DrawnTorch DRAWN3_TORCH[] = {
    { -1, 20, 0, 0, 1 }, { 1, 23, 3, 0, 1 }, { 4, 23, 3, 0, 1 }, { 4, 19, 1, 0, 1 },
    { 6, 17, 1, 0, 1 }, { 6, 25, 3, 0, 1 }, { 8, 27, 3, 0, 1 }, { 8, 15, 1, 0, 1 },
    { 9, 14, 0, 0, 1 }, { 9, 28, 0, 0, 1 }, { 17, 14, 2, 0, 1 }, { 18, 15, 1, 0, 1 },
    { 19, 17, 2, 0, 1 }, { 7, 17, 0, 0, 1 }, { 11, 18, 2, 0, 1 }, { 10, 23, 1, 0, 1 },
    { 15, 24, 0, 0, 1 }, { 16, 19, 3, 0, 1 }, { 21, 20, 2, 0, 1 }, { 21, 22, 2, 0, 1 },
    { 19, 25, 3, 0, 1 }, { 18, 27, 3, 0, 1 }, { 17, 28, 2, 0, 1 }, { 12, 11, 3, 0, 1 },
    { 14, 11, 3, 0, 1 }, { 10, 9, 1, 0, 1 }, { 16, 9, 1, 0, 1 }, { 8, 11, 3, 0, 1 },
    { 18, 11, 3, 0, 1 }, { 21, 7, 2, 0, 1 }, { 5, 7, 0, 0, 1 }, { 8, 23, 2, 0, 1 },
    { 18, 23, 0, 0, 1 }, { 10, 20, 0, 0, 1 }, { 10, 22, 0, 0, 1 }, { 16, 20, 2, 0, 1 },
    { 16, 22, 2, 0, 1 }, { 14, 18, 1, 0, 1 }, { 12, 18, 1, 0, 1 }, { 14, 24, 3, 0, 1 },
    { 12, 24, 3, 0, 1 }, { 11, 9, 3, 0, 1 }, { 15, 9, 3, 0, 1 }, { 8, 3, 1, 0, 1 },
    { 13, 3, 1, 0, 1 }, { 18, 3, 1, 0, 1 }, { 17, -1, 1, 0, 1 }, { 13, -1, 1, 0, 1 },
    { 9, -1, 1, 0, 1 }, { 3, 4, 0, 0, 1 }, { 23, 5, 2, 0, 1 }, { 14, 31, 3, 0, 1 },
    { 12, 31, 3, 0, 1 }, { 21, 38, 1, 0, 1 }, { 21, 42, 3, 0, 1 }, { 24, 41, 3, 0, 1 },
    { 26, 39, 1, 0, 1 }, { 25, 36, 1, 0, 1 }, { 7, 39, 2, 0, 1 }, { 4, 41, 0, 0, 1 },
    { 3, 14, 0, 0, 1 }, { 6, 12, 1, 0, 1 }, { 7, 16, 3, 0, 1 }, { 25, 6, 3, 0, 1 },
    { 27, 3, 1, 0, 1 }, { 29, 6, 3, 0, 1 }, { 28, 23, 0, 0, 1 }, { 29, 25, 3, 0, 1 },
    { 32, 22, 2, 0, 1 }, { 17, 8, 0, 0, 1 }, { 9, 8, 2, 0, 1 }, { 13, 7, 3, 0, 1 },
    { 11, 5, 1, 0, 1 }, { 15, 5, 1, 0, 1 }, { 19, 5, 1, 0, 1 }, { 7, 5, 1, 0, 1 },
    { 11, 1, 1, 0, 1 }, { 15, 1, 1, 0, 1 }, { 19, 1, 1, 0, 1 }, { 7, 1, 1, 0, 1 },
    { 21, 2, 2, 0, 1 }, { 5, 2, 0, 0, 1 }, { 5, 5, 0, 0, 1 }, { 21, 5, 2, 0, 1 },
    { 13, 18, 3, 0, 1 }, { 15, 16, 3, 0, 1 }, { 11, 16, 3, 0, 1 }, { 3, 11, 0, 0, 1 },
    { 28, 11, 1, 0, 1 }, { 29, 17, 0, 0, 1 }, { 23, 15, 1, 0, 1 }, { 22, 10, 1, 0, 1 },
    { 11, 39, 2, 0, 1 }, { 7, 39, 0, 0, 1 }, { 15, 39, 0, 0, 1 }, { 15, 41, 0, 0, 1 },
    { 19, 39, 2, 0, 1 }, { 19, 41, 2, 0, 1 },
};
static const char *const DRAWN3_RELIC[] = { "A RELIC" };
/* triggers: the kind; once, every time, a switch or timed; how long it holds, s (0 till touched again); whether its torches and doors go
 * back with it; its thing (an index in DRAWN3_THING) or -1; its box (x0, y0, x1, y1); what it waits on, a bit a trigger; its actions;
 * the actions as it goes back */
static const DrawnTrig DRAWN3_TRIG[] = {
    { TG_STEP, TT_EVERY, 0.00f, 1, 11, 0, 0, -1, -1, 0x0u, 0, 1, 1, 0 },    /* Enter Back Area Switch */
    { TG_PLACE, TT_TOGGLE, 0.00f, 1, 9, 0, 0, -1, -1, 0x0u, 1, 1, 2, 0 },    /* Open Compass Room */
    { TG_STEP, TT_EVERY, 0.00f, 1, 12, 0, 0, -1, -1, 0x1u, 2, 1, 3, 0 },    /* Close Backroom Entrance */
    { TG_USE, TT_ONCE, 0.00f, 1, 13, 0, 0, -1, -1, 0x0u, 3, 1, 4, 0 },    /* Miniboss Spawn */
    { TG_USE, TT_TOGGLE, 5.00f, 1, 10, 0, 0, -1, -1, 0x1u, 4, 1, 5, 0 },    /* Trigger 5 */
};
/* actions: the kind; the group or the sound; a door's cell in the drawing; after how long, s; the line */
static const DrawnAct DRAWN3_ACT[] = {
    { TA_DOOR_OPEN, 0, 20, 5, 0.00f, NULL },
    { TA_DOOR_OPEN, 0, 13, 24, 0.00f, NULL },
    { TA_DOOR_SHUT, 0, 20, 5, 0.00f, NULL },
    { TA_WAKE, 0, 0, 0, 2.00f, NULL },
    { TA_DOOR_OPEN, 0, 6, 5, 0.00f, NULL },
};
/* things the run's dice lay: the thing (an index in DRAWN3_THING); the rooms it may appear in (a bit a room in DRAWN3_ROOM: a spot standing
 * there, else the floor nearest the middle); the tag of the spots it may appear at, or -1 */
static const DrawnPick DRAWN3_PICK[] = {
    { 1, 0x19cu, 0 },    /* Crimson key */
};
/* fire and ice: the kind; a cell of it in the drawing (the field is every cell of its kind joined to it); live at the start; its harm,
 * % of health each half second; the chance of its status (On Fire, a Chilled stack), %; whether it may kill */
static const DrawnField DRAWN3_FIELD[] = {
    { DF_FIRE, 16, 38, 0, 35, 100, 1 },    /* Fire 2 */
    { DF_FIRE, 8, 42, 1, 35, 100, 1 },    /* Fire 1 */
};
/* hazards (docs/PLAN-events.md, 2c): the kind; its anchor; its cells (a run in DRAWN3_HZCELL); a zone (one place at a time) or placed;
 * what sets it off; every, give, first, s; the box walked into; live at the start; harm %, its status %, may kill; warning, s (-1: the game's own) */
static const DrawnHaz DRAWN3_HAZ[] = {
    { HZ_RELIQ, 12, 20, 0, 8, 0, HH_CLOCK | HH_ENTER, 8.00f, 2.00f, 1.00f, 11, 19, 13, 21, 1, -1, -1, 0, -1.00f },    /* The hungry reliquary 1 */
    { HZ_RELIQ, 14, 22, 8, 8, 0, HH_CLOCK | HH_ENTER, 8.00f, 2.00f, 1.00f, 13, 21, 13, 21, 1, -1, -1, 0, -1.00f },    /* The hungry reliquary 2 */
    { HZ_CENSER, 6, 19, 16, 2, 0, HH_CLOCK, 8.00f, 2.00f, 3.00f, 0, 0, -1, -1, 1, -1, -1, 0, -1.00f },    /* The swinging censer 1 */
    { HZ_CENSER, 19, 19, 18, 2, 0, HH_CLOCK, 8.00f, 2.00f, 3.00f, 0, 0, -1, -1, 1, -1, -1, 0, -1.00f },    /* The swinging censer 2 */
    { HZ_COLD, 11, 26, 20, 6, 0, HH_CLOCK, 8.00f, 2.00f, 3.00f, 0, 0, -1, -1, 1, -1, -1, 0, -1.00f },    /* The cold vault 1 */
    { HZ_COLD, 15, 26, 26, 6, 0, HH_CLOCK, 8.00f, 2.00f, 3.00f, 0, 0, -1, -1, 1, -1, -1, 0, -1.00f },    /* The cold vault 2 */
    { HZ_ROCK, 13, 12, 32, 3, 0, HH_ENTER, 8.00f, 2.00f, 3.00f, 13, 13, 13, 13, 1, -1, -1, 1, -1.00f },    /* Falling masonry 1 */
};
static const DrawnCell DRAWN3_HZCELL[] = {
    { 12, 19 }, { 13, 19 }, { 13, 20 }, { 13, 21 }, { 12, 21 }, { 11, 21 }, { 11, 20 }, { 12, 20 },
    { 14, 22 }, { 14, 21 }, { 15, 21 }, { 15, 22 }, { 14, 23 }, { 13, 23 }, { 13, 22 }, { 13, 21 },
    { 6, 19 }, { 7, 19 }, { 19, 19 }, { 20, 19 }, { 12, 26 }, { 12, 25 }, { 12, 27 }, { 11, 27 },
    { 10, 27 }, { 10, 26 }, { 16, 26 }, { 14, 25 }, { 14, 26 }, { 14, 27 }, { 15, 27 }, { 16, 27 },
    { 13, 12 }, { 12, 12 }, { 14, 12 },
};
/* its row in DRAWN[] */
    { DRAWN3_MAP, 32, 43, 1, 1, 1, DRAWN3_ROOM, (int)(sizeof DRAWN3_ROOM / sizeof DRAWN3_ROOM[0]), { 5, -1, 6 },
      DRAWN3_THING, (int)(sizeof DRAWN3_THING / sizeof DRAWN3_THING[0]), 0x7D1A0003u, 3, DRAWN3_SPAWN, (int)(sizeof DRAWN3_SPAWN / sizeof DRAWN3_SPAWN[0]), DRAWN3_TORCH, 98, DRAWN3_TRIG, 5, DRAWN3_ACT, 5, DRAWN3_RELIC, 1, DRAWN3_PICK, 1, DRAWN3_FIELD, 2, DRAWN3_HAZ, 7, DRAWN3_HZCELL, 35 },
