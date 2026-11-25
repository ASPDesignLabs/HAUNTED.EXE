/* Every part of the events (a test level, not for play): level 3, drawn in the map editor (docs/PLAN-mapedit.md) */
static const char *const DRAWN3_MAP[35] = {
    "############.+.+.+.+.+.#############",    /*  0 */
    "############+++++++++++#############",    /*  1 */
    "############.+.+.+.+.+.#############",    /*  2 */
    "############+++++++++++#############",    /*  3 */
    "############.+.+.+.+.+.#############",    /*  4 */
    "############+++++++++++#############",    /*  5 */
    "############.+.+.+.+.+.#############",    /*  6 */
    "#################+##################",    /*  7 */
    "#################+##################",    /*  8 */
    "#################+##################",    /*  9 */
    "#################+##################",    /* 10 */
    "#################+##################",    /* 11 */
    ".+.+.+.#####.+.+.+.+.+.#####.+.+.+.#",    /* 12 */
    "+++++++#####+++++++++++#####+++++++#",    /* 13 */
    ".+.+.+.#####.+.+.+.+.+.#####.+.+.+.#",    /* 14 */
    "+++++++#####+++++++++++#####+++++++#",    /* 15 */
    ".+.+.+.#####.+.+.+.+.+.#####.+.+.+.#",    /* 16 */
    "+S+++++D+++++++++++++++++++B+++++++X",    /* 17 */
    ".+.+.+.#####.+.+.+.+.+.#####.+.+.+.#",    /* 18 */
    "+++++++#####+++++++++++#####+++++++#",    /* 19 */
    ".+.+.+.#####.+.+.+.+.+.#####.+.+.+.#",    /* 20 */
    "+++++++#####+++++++++++#####+++++++#",    /* 21 */
    ".+.+.+.#####.+.+.+.+.+.#####.+.+.+.#",    /* 22 */
    "#################+##################",    /* 23 */
    "#################+##################",    /* 24 */
    "#################+##################",    /* 25 */
    "#################+##################",    /* 26 */
    "#################+##################",    /* 27 */
    "############.+.+.+FfF+.#############",    /* 28 */
    "############++++++fff++#############",    /* 29 */
    "############.+.+.+FfF+.#############",    /* 30 */
    "############+++++++++++#############",    /* 31 */
    "############.+.+.+.+.+.#############",    /* 32 */
    "############+++++++++++#############",    /* 33 */
    "############.+.+.+.+.+.#############",    /* 34 */
};
static const DrawnThing DRAWN3_THING[] = {
    { DT_COMPASS, 0, 17.50f, 17.50f }, { DT_RELIC, 0, 17.50f, 2.50f }, { DT_SOCKET, 0, 13.63f, 13.63f },
    { DT_GOLD, IT_CHALICE, 13.38f, 1.38f }, { DT_GOLD, IT_GEMS, 21.63f, 5.63f }, { DT_GOLD, IT_CHEST, 17.50f, 32.50f },
    { DT_GOLD, IT_COINS, 13.38f, 29.38f }, { DT_GOLD, IT_CROWN, 33.38f, 21.38f }, { DT_SPOT, 0, 15.50f, 15.50f },
    { DT_SPOT, 0, 19.50f, 19.50f }, { DT_SPOT, 0, 3.50f, 15.50f }, { DT_PLATE, 0, 17.50f, 9.50f },
    { DT_LEVER, 0, 21.50f, 21.50f },
};
static const DrawnRoom DRAWN3_ROOM[] = {
    { 0, 12, 7, 11, RT_OSSUARY },
    { 12, 12, 11, 11, RT_CHAPTER },
    { 12, 0, 11, 7, RT_BURIAL },
    { 28, 12, 7, 11, RT_KNIGHTS },
    { 12, 28, 11, 7, RT_UNDER },
};
/* spawn points: x, y; the kind or -1 the budget's choice; the chance %; the element's chance % or -1 by rank; the element or -1;
 * the rank or -1 the level's usual; 1 + its wake group (0: filled as the level is laid); 1 + the tag of the spots it is laid at (0: here) */
static const DrawnSpawn DRAWN3_SPAWN[] = {
    { 20.50f, 1.50f, MOB_SKEL, 100, -1, -1, -1 }, { 21.50f, 32.50f, -1, 75, -1, -1, -1 },
    { 31.50f, 13.50f, MOB_SKEL, 100, -1, -1, RK_II, 1 }, { -4.50f, -4.50f, -1, 100, -1, -1, -1, 0, 1 },
};
/* torches: x, y of the wall in the drawing; the face (0 east, 1 south, 2 west, 3 north); the group (0 path A, 1 path B, 2 path C, 3 the hub); lit at the start */
static const DrawnTorch DRAWN3_TORCH[] = {
    { 16, 8, 0, 0, 0 }, { 18, 10, 2, 0, 0 }, { 24, 16, 1, 1, 0 }, { 26, 18, 3, 1, 0 },
    { 16, 25, 0, 2, 0 }, { 18, 27, 2, 2, 0 }, { 11, 16, 1, 3, 1 }, { 11, 18, 3, 3, 1 },
};
static const char *const DRAWN3_RELIC[] = { "A BONE KEY" };
/* triggers: the kind; once, every time, a switch or timed; how long it holds, s (0 till touched again); whether its torches and doors go
 * back with it; its thing (an index in DRAWN3_THING) or -1; its box (x0, y0, x1, y1); what it waits on, a bit a trigger; its actions;
 * the actions as it goes back */
static const DrawnTrig DRAWN3_TRIG[] = {
    { TG_PICKUP, TT_ONCE, 0.00f, 1, 0, 0, 0, -1, -1, 0x0u, 0, 3, 3, 0 },    /* The compass taken */
    { TG_PLACE, TT_ONCE, 0.00f, 1, 2, 0, 0, -1, -1, 0x1u, 3, 5, 8, 0 },    /* The bone set */
    { TG_ENTER, TT_ONCE, 0.00f, 1, -1, 12, 28, 22, 34, 0x0u, 8, 1, 9, 0 },    /* The south room found */
    { TG_STEP, TT_TIMED, 6.00f, 1, 11, 0, 0, -1, -1, 0x0u, 9, 1, 10, 1 },    /* The plate */
    { TG_USE, TT_TOGGLE, 0.00f, 1, 12, 0, 0, -1, -1, 0x0u, 11, 1, 12, 0 },    /* The lever */
};
/* actions: the kind; the group or the sound; a door's cell in the drawing; after how long, s; the line */
static const DrawnAct DRAWN3_ACT[] = {
    { TA_FLICKER_ALL, 0, 0, 0, 0.00f, NULL },
    { TA_SOUND, SFX_DEEPCHIME, 0, 0, 0.00f, NULL },
    { TA_FLICKER_ON, 0, 0, 0, 0.80f, NULL },
    { TA_FLICKER_OFF, 0, 0, 0, 0.00f, NULL },
    { TA_FLICKER_ON, 1, 0, 0, 1.00f, NULL },
    { TA_DOOR_OPEN, 0, 27, 17, 1.50f, NULL },
    { TA_WAKE, 0, 0, 0, 1.50f, NULL },
    { TA_LINE, 0, 0, 0, 1.00f, "SOMETHING STIRS TO THE EAST" },
    { TA_LIGHT_ON, 2, 0, 0, 0.00f, NULL },
    { TA_LIGHT_ON, 2, 0, 0, 0.00f, NULL },
    { TA_SOUND, SFX_KNOCK, 0, 0, 0.00f, NULL },
    { TA_FIELD_OFF, 0, 0, 0, 0.00f, NULL },
};
/* things the run's dice lay: the thing (an index in DRAWN3_THING); the rooms it may appear in (a bit a room in DRAWN3_ROOM: a spot standing
 * there, else the floor nearest the middle); the tag of the spots it may appear at, or -1 */
static const DrawnPick DRAWN3_PICK[] = {
    { 0, 0x0u, 0 },    /* The compass */
};
/* fire and ice: the kind; a cell of it in the drawing (the field is every cell of its kind joined to it); live at the start; its harm,
 * % of health each half second; the chance of its status (On Fire, a Chilled stack), %; whether it may kill */
static const DrawnField DRAWN3_FIELD[] = {
    { DF_FIRE, 18, 28, 1, 10, 50, 0 },    /* The south embers */
};
/* its row in DRAWN[] */
    { DRAWN3_MAP, 36, 35, 5, 5, 0, DRAWN3_ROOM, (int)(sizeof DRAWN3_ROOM / sizeof DRAWN3_ROOM[0]), { -1, -1, -1 },
      DRAWN3_THING, (int)(sizeof DRAWN3_THING / sizeof DRAWN3_THING[0]), 0x7D1A0003u, 3, DRAWN3_SPAWN, (int)(sizeof DRAWN3_SPAWN / sizeof DRAWN3_SPAWN[0]), DRAWN3_TORCH, 8, DRAWN3_TRIG, 5, DRAWN3_ACT, 12, DRAWN3_RELIC, 1, DRAWN3_PICK, 1, DRAWN3_FIELD, 1 },
