/* Untitled level: level 3, drawn in the map editor (docs/PLAN-mapedit.md) */
static const char *const DRAWN3_MAP[2] = {
    "####X",    /*  0 */
    "S++++",    /*  1 */
};
static const DrawnThing DRAWN3_THING[] = {
    { DT_SIDEDOOR, 37, 1.50f, 0.50f },
};
static const DrawnRoom DRAWN3_ROOM[] = {
    { 0, 0, 1, 1, RT_HALL },    /* the array may not be empty: draw a room */
};
/* its row in DRAWN[] */
    { DRAWN3_MAP, 5, 2, 9, 10, 0, DRAWN3_ROOM, (int)(sizeof DRAWN3_ROOM / sizeof DRAWN3_ROOM[0]), { -1, -1, -1 },
      DRAWN3_THING, (int)(sizeof DRAWN3_THING / sizeof DRAWN3_THING[0]), 0x7D1A0003u, 3, NULL, 0 },
