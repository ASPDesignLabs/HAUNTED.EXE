/* Tutorial level 2 (from drawn.c): level 2, drawn in the map editor (docs/PLAN-mapedit.md) */
static const char *const DRAWN2_MAP[35] = {
    "##S++.###################",    /*  0 */
    "####+.####...############",    /*  1 */
    "###.+.####...############",    /*  2 */
    "##..+..###...############",    /*  3 */
    "#+++++++###B#############",    /*  4 */
    "#.+.#.+.......###########",    /*  5 */
    "#.+###++++++++###########",    /*  6 */
    "#.+.#.+......+###########",    /*  7 */
    "#.++#.+.####.+###########",    /*  8 */
    "##.+#++#####.+###########",    /*  9 */
    "###+#+##X####=###########",    /* 10 */
    "++G+#+#.+.#.++.##########",    /* 11 */
    ".+###+#.+.#.++.##########",    /* 12 */
    ".+#+++#.+.#.++.##########",    /* 13 */
    ".+#+#+#.+.#.++.##########",    /* 14 */
    ".+#+#+#.+.#.++.##########",    /* 15 */
    ".+#+++#.+.#IiiI##########",    /* 16 */
    ".+#++##.+.#IiiI##########",    /* 17 */
    ".+#+h##.+.#IiiI##qq#qq#q#",    /* 18 */
    ".+#+#+C+++#IiiIcqqqqqqqqq",    /* 19 */
    ".+#+#+######ii###########",    /* 20 */
    "##.+#+.#####ii###########",    /* 21 */
    "#..+#+..####ii###########",    /* 22 */
    "#+++#+++++++++###########",    /* 23 */
    "#.+###...+....###########",    /* 24 */
    "#.+###...+....###########",    /* 25 */
    "#+++#+++++++++###########",    /* 26 */
    "#+++++++#####D###########",    /* 27 */
    "##..+..##....+###########",    /* 28 */
    "###.+.##++++++###########",    /* 29 */
    "###.+.#+++++++###########",    /* 30 */
    "###.+.#+.....+###########",    /* 31 */
    "#######c#################",    /* 32 */
    "#######QQQ###############",    /* 33 */
    "#######qqq###############",    /* 34 */
};
static const DrawnThing DRAWN2_THING[] = {
    { DT_GOLD, IT_COINS, 5.30f, 2.50f }, { DT_GOLD, IT_CHALICE, 2.50f, 3.50f }, { DT_GOLD, IT_COINS, 1.40f, 7.00f },
    { DT_GOLD, IT_GEMS, 1.30f, 7.90f }, { DT_GOLD, IT_COINS, 11.10f, 6.40f }, { DT_GOLD, IT_CHALICE, 7.40f, 8.70f },
    { DT_GOLD, IT_COINS, 5.60f, 12.50f }, { DT_GOLD, IT_COINS, 3.30f, 13.60f }, { DT_GOLD, IT_GEMS, 5.70f, 15.50f },
    { DT_GOLD, IT_CHALICE, 11.30f, 13.90f }, { DT_GOLD, IT_GEMS, 11.30f, 17.40f }, { DT_GOLD, IT_COINS, 7.60f, 11.30f },
    { DT_GOLD, IT_CHALICE, 9.30f, 11.40f }, { DT_GOLD, IT_CROWN, 9.60f, 18.60f }, { DT_GOLD, IT_COINS, 1.60f, 14.90f },
    { DT_GOLD, IT_CHEST, 0.40f, 20.60f }, { DT_GOLD, IT_GEMS, 1.60f, 20.60f }, { DT_GOLD, IT_COINS, 9.60f, 24.10f },
    { DT_GOLD, IT_CHALICE, 12.20f, 25.40f }, { DT_GOLD, IT_COINS, 1.30f, 27.50f }, { DT_GOLD, IT_COINS, 7.70f, 27.70f },
    { DT_GOLD, IT_CHALICE, 2.80f, 28.80f }, { DT_GOLD, IT_COINS, 4.00f, 30.50f }, { DT_GOLD, IT_GEMS, 4.90f, 31.70f },
    { DT_GOLD, IT_COINS, 9.40f, 28.50f }, { DT_GOLD, IT_CHALICE, 8.60f, 29.30f }, { DT_SGOLD, IT_CHEST, 9.40f, 33.50f },
    { DT_SGOLD, IT_GEMS, 8.50f, 33.60f }, { DT_SGOLD, IT_CROWN, 8.50f, 34.50f }, { DT_SGOLD, IT_GEMS, 9.50f, 34.50f },
    { DT_KEY, 0, 7.60f, 30.80f }, { DT_KEY, 2, 1.00f, 19.60f }, { DT_MAP, 0, 12.80f, 15.40f },
    { DT_DEAD, MOB_SKEL, 2.30f, 5.00f }, { DT_DEAD, MOB_BAT, 13.00f, 5.70f }, { DT_DEAD, MOB_SKEL, 3.40f, 9.40f },
    { DT_DEAD, MOB_SKEL, 11.70f, 12.60f }, { DT_DEAD, MOB_SHAMBLER, 7.90f, 13.10f }, { DT_DEAD, MOB_SKEL, 3.50f, 15.00f },
    { DT_DEAD, MOB_SKEL, 9.50f, 16.40f }, { DT_DEAD, MOB_BAT, 14.30f, 17.60f }, { DT_DEAD, MOB_SHAMBLER, 1.40f, 18.20f },
    { DT_DEAD, MOB_SKEL, 7.10f, 22.60f }, { DT_DEAD, MOB_SHAMBLER, 3.10f, 22.90f }, { DT_DEAD, MOB_SKEL, 11.60f, 26.30f },
    { DT_DEAD, MOB_BAT, 12.90f, 28.80f }, { DT_DEAD, MOB_SKEL, 9.30f, 30.80f }, { DT_DEAD, MOB_SHAMBLER, 3.60f, 31.30f },
    { DT_MARK, 0, 5.50f, 3.50f }, { DT_MARK, 1, 11.80f, 7.60f }, { DT_MARK, 2, 2.40f, 9.50f },
    { DT_MARK, 2, 7.30f, 14.80f }, { DT_MARK, 2, 0.30f, 16.20f }, { DT_MARK, 2, 3.20f, 16.50f },
    { DT_MARK, 0, 11.70f, 23.20f }, { DT_MARK, 2, 3.70f, 26.20f }, { DT_MARK, 2, 5.50f, 31.40f },
    { DT_MARK, 1, 11.10f, 31.60f }, { DT_SWITCH, 0, 13.13f, 18.88f }, { DT_GSEAL, 0, 13.50f, 11.06f },
    { DT_SIDEDOOR, 16, 23.38f, 17.63f }, { DT_SIDEDOOR, 17, 11.50f, 0.50f }, { DT_PLATE, 0, 11.63f, 3.63f },
    { DT_SGOLD, IT_CHEST, 23.38f, 19.38f }, { DT_SGOLD, IT_GEMS, 20.63f, 18.63f }, { DT_SGOLD, IT_CROWN, 18.63f, 18.63f },
    { DT_SGOLD, IT_CHALICE, 17.38f, 18.38f },
};
static const DrawnRoom DRAWN2_ROOM[] = {
    { 0, 11, 2, 10, RT_OSSUARY },
    { 11, 11, 4, 5, RT_KNIGHTS },
    { 7, 11, 3, 9, RT_CHAPTER },
    { 7, 28, 7, 4, RT_BURIAL },
    { 5, 23, 9, 4, RT_UNDER },
    { 1, 27, 5, 5, RT_UNDER },
};
/* triggers: the kind; once, every time, a switch or timed; how long it holds, s (0 till touched again); whether its torches and doors go
 * back with it; its thing (an index in DRAWN2_THING) or -1; its box (x0, y0, x1, y1); what it waits on, a bit a trigger; its actions;
 * the actions as it goes back */
static const DrawnTrig DRAWN2_TRIG[] = {
    { TG_STEP, TT_EVERY, 0.00f, 1, 62, 0, 0, -1, -1, 0x0u, 0, 1, 1, 0 },    /* Open Back Room Exit */
};
/* actions: the kind; the group or the sound; a door's cell in the drawing; after how long, s; the line */
static const DrawnAct DRAWN2_ACT[] = {
    { TA_DOOR_OPEN, 0, 11, 4, 0.00f, NULL },
};
/* lights (docs/PLAN-lights.md): the style; its cell in the drawing. With any, the generator hangs and stands none of its own */
static const DrawnLight DRAWN2_LIGHT[] = {
    { DL_CHANDELIER, 14, 12 }, { DL_CHANDELIER, 8, 18 }, { DL_CHANDELIER, 12, 30 }, { DL_LANTERN, 0, 15 }, { DL_LANTERN, 4, 28 }, { DL_CHANDELIER, 11, 17 },
    { DL_CHANDELIER, 9, 6 }, { DL_CHANDELIER, 8, 25 },
};
/* its row in DRAWN[] */
    { DRAWN2_MAP, 25, 35, 15, 5, 0, DRAWN2_ROOM, (int)(sizeof DRAWN2_ROOM / sizeof DRAWN2_ROOM[0]), { 0, -1, 2 },
      DRAWN2_THING, (int)(sizeof DRAWN2_THING / sizeof DRAWN2_THING[0]), 0x7D1A0002u, 2, NULL, 0, NULL, 0, DRAWN2_TRIG, 1, DRAWN2_ACT, 1, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, DRAWN2_LIGHT, 8 },
