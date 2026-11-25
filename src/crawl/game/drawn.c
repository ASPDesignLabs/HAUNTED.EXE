/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Drawn levels (the tutorial's two): the maps, gen_drawn, their items, dead and marks; the burning floor; the ice. */

/* ---- Drawn levels (docs/PLAN-tutorial.md): levels 1 and 2 of the first descent are the owner's drawings, the same every
 * time, for everyone, the dream too. Each is a map of letters, its rooms by type, and what stands in it, in the drawing's own
 * cells (x east from its corner, y south, as media/sheets/tutorial/level*-read.png has them), laid at (ox, oy) in the level's map.
 * The look, the dressing and the torches are the generator's, laid on a seed of the drawing's own, so they lie the same each
 * time too. The letters: # stone, . floor, + floor on a lane, F the burning floor, f on a lane, D a door, G a gold door,
 * C a crimson door, = the one-way gate, B a door barred till an event opens it, S the start (on a lane), h the place to mend and
 * X the exit (walls, used from their open side, north first), I the ice, i on a lane, c a secret's wall, q its room, Q on a lane.
 * Doors are on lanes. From level 3 the drawings are the owner's, from the map editor (docs/PLAN-mapedit.md), with their events
 * (docs/PLAN-events.md). ---- */
typedef struct { int x, y, w, h, rtype; } DrawnRoom;
enum { DT_GOLD, DT_KEY, DT_DEAD, DT_SGOLD, DT_MARK, DT_SWITCH, DT_GSEAL, DT_MAP,   /* SGOLD: in the secret; MARK's kind 0 a coffer,
                                                                      1 a cage (any line), 2 the class's own */
       DT_COMPASS, DT_RELIC, DT_SOCKET, DT_LEVER, DT_PLATE, DT_SPOT };   /* the events' (docs/PLAN-events.md): RELIC's kind its
                                                                      number in _RELIC; SOCKET's the relic it takes, -1 any; SPOT's its tag */
typedef struct { int what, kind; float x, y; } DrawnThing;          /* kind: an IT_* for gold, a key's colour, a MOB_* */
/* A spawn point from the map editor (docs/PLAN-mapedit.md): where, in the drawing's cells; the kind, or -1 the budget's choice; the
 * chance it is filled, %; its chance of being born to an element, %, or -1 by its rank as elem_roll has it; the element, or -1 the
 * run's favourites; its rank, or -1 the level's usual; 1 + the wake group that fills it (0: as the level is laid); 1 + the tag of the
 * spots it is laid at (0: where drawn) */
typedef struct { float x, y; int kind, chance, echance, elem, rank, wake, spot; } DrawnSpawn;
/* The events (docs/PLAN-events.md, section 2b: the format the editor writes, and this reads). A torch: its wall's cell, the face it
 * hangs on (0 east, 1 south, 2 west, 3 north), its group, lit at the start. A trigger: what fires it (TG_*), its type (TT_*), how long
 * it holds (s), whether its torches, fields and doors go back with it, its thing (an index in _THING) or the box walked into, what it
 * waits on (a bit a trigger), its actions and its actions going back (runs in _ACT). An action (TA_*): its torch group, field, wake
 * group or SFX_*; a door's cell; its delay. A pick: a thing the run's dice lay in one of its rooms (a bit each) or at a spot of its
 * tag. A field: a cell of it (the field is every cell of its kind joined to it), live at the start, its harm (% of health each 0.5 s),
 * the chance of its status (%), whether it may kill. */
typedef struct { int x, y, d, group, lit; } DrawnTorch;
enum { TG_PICKUP, TG_PLACE, TG_STRIKE, TG_ENTER, TG_USE, TG_STEP, TG_ALL };
enum { TT_ONCE, TT_EVERY, TT_TOGGLE, TT_TIMED };
typedef struct { int kind, type; float hold; int undo, thing, x0, y0, x1, y1; unsigned needs; int act, nact, back, nback; } DrawnTrig;
enum { TA_FLICKER_ON, TA_FLICKER_OFF, TA_LIGHT_ON, TA_LIGHT_OFF, TA_FLICKER_ALL, TA_FIELD_ON, TA_FIELD_OFF, TA_DOOR_OPEN,
       TA_DOOR_UNLOCK, TA_DOOR_SHUT, TA_WAKE, TA_SECRET, TA_LINE, TA_SOUND };
typedef struct { int kind, arg, x, y; float after; const char *text; } DrawnAct;
typedef struct { int thing; unsigned rooms; int tag; } DrawnPick;
enum { DF_FIRE, DF_ICE };
typedef struct { int kind, x, y, on, dmg, chance, kills; } DrawnField;
typedef struct {
    const char *const *map; int w, h, ox, oy, face;                 /* face: the heading at the start, as G.h */
    const DrawnRoom *room; int nroom, vault[NKEYS];                /* vault: the room each colour's doors keep, -1 none */
    const DrawnThing *thing; int nthing;
    uint32_t seed;                                                 /* the dice its look is laid with */
    int level;                                                     /* the level it is laid on, the first time down (the owner picks) */
    const DrawnSpawn *spawn; int nspawn;                           /* its spawn points (the map editor's), filled by the budget */
    const DrawnTorch *torch; int ntorch;                           /* the events' (docs/PLAN-events.md), each at the end: a row that */
    const DrawnTrig *trig; int ntrig;                              /* leaves them off has none */
    const DrawnAct *act; int nact;
    const char *const *relic; int nrelic;                          /* the relics' names */
    const DrawnPick *pick; int npick;
    const DrawnField *field; int nfield;                           /* none: the tutorial's own fire and ice (DF, IC) */
} Drawn;
/* What a drawn level's events hold while it is played (events.c runs them); laid with the level, never saved */
#define EV_THINGS 256                       /* a drawing's things, for where this play lays each */
#define EV_SPAWNS 64
#define EV_TORCHES 120                                            /* at_torch holds 1 + the torch in a byte */
#define EV_TRIGS 32                         /* a bit each in a trigger's needs */
#define EV_QUEUE 96
#define EV_FIELDS 8
#define EV_FCELLS 192
enum { EA_NONE, EA_IN, EA_OUT };            /* a torch flickering to life, or out */
typedef struct { int wx, wy, d, group, light, lit, anim; float t; } EvTorch;   /* light: its G.lights, -1 none; t: the flicker's time
                                                                      (below 0, waiting its turn in the group) */
typedef struct { int kind, on, dmg, chance, kills, first, n; float t, fade; } EvField;   /* t: since it was lit or put out; fade: the
                                                                      fire's flames, 0 to 1 */
static struct {
    float now;                                                     /* the events' clock: the level's time while they run */
    float tx[EV_THINGS], ty[EV_THINGS];                            /* each thing where this play lays it (the picks), in the drawing's cells */
    float sx[EV_SPAWNS], sy[EV_SPAWNS];                            /* and each spawn point */
    int mk[MAXMK];                                                 /* a thing that wants a spell: 1 + its thing, 0 none */
    float rearm[MAXMK];                                            /* and, struck for a trigger that fires again, when it may be again */
    int nt;
    EvTorch torch[EV_TORCHES];
    int8_t at_torch[MAPN][MAPN], light_of[MAXLIGHTS];             /* 1 + the torch on a wall's cell, or a light is */
    struct { int n, on, inside; float back_t; } tr[EV_TRIGS];      /* fired n times; on (a switch or timed); in its box; when it goes back */
    struct { int act, trig, kind, back; float due; } q[EV_QUEUE]; int nq;   /* actions waiting out their delay; kind: the action's own or
                                                                      its undoing; back: going back's (kept when it goes back) */
    float all_t;                                                   /* every light stuttering, this long yet */
    int nf; EvField f[EV_FIELDS];
    int fx[EV_FCELLS], fy[EV_FCELLS]; float fwait[EV_FCELLS];     /* the fields' cells, each field's together; each ice cell's wait */
    uint8_t fat[MAPN][MAPN];                                       /* 1 + the field a cell is in */
    int f_in; float f_tick, f_stack;                               /* the field the caster stands in (1 +), and its next harm, its next stack */
} EV;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"     /* a row, or a spawn point, leaves off what it has none of: zeros */
static const char *const DRAWN1_MAP[32] = {
    "##S++#################",    /*  0 */
    "####D#################",    /*  1 */
    "##Fff#################",    /*  2 */
    "##FfF#################",    /*  3 */
    "##FfF#################",    /*  4 */
    "##FfF#################",    /*  5 */
    "##.+.#################",    /*  6 */
    "##.+.#################",    /*  7 */
    "##.+.#################",    /*  8 */
    "##.+.............#####",    /*  9 */
    "##.++++++++++++++#####",    /* 10 */
    "##.+...........+.#####",    /* 11 */
    "###h##########.+.#.+..",    /* 12 */
    "##############.+.#++++",    /* 13 */
    "##############.+.#.+..",    /* 14 */
    "##############.++D++++",    /* 15 */
    "##########.+.#.+.#####",    /* 16 */
    "##########+++D++.#####",    /* 17 */
    "##########.+.#.+.#####",    /* 18 */
    "##############.+.#####",    /* 19 */
    "####...........+.#####",    /* 20 */
    "####+++++++++++++#####",    /* 21 */
    "####+............#####",    /* 22 */
    ".++D++################",    /* 23 */
    ".+.#.+####.+.#########",    /* 24 */
    ".+##.+####.+.#########",    /* 25 */
    ".+G+#+#####D##########",    /* 26 */
    ".+#+#+##...+...#######",    /* 27 */
    ".+#+#+##+++++++#######",    /* 28 */
    ".+#+#+##...+...#####X#",    /* 29 */
    ".+#+#+.....+...#....+.",    /* 30 */
    ".+#+G++++++++++D++++++",    /* 31 */
};
static const DrawnThing DRAWN1_THING[] = {
    { DT_GOLD, IT_COINS, 4.01f, 7.02f }, { DT_GOLD, IT_COINS, 6.01f, 10.02f }, { DT_GOLD, IT_COINS, 12.03f, 10.02f },
    { DT_GOLD, IT_CHALICE, 16.04f, 10.02f }, { DT_GOLD, IT_COINS, 3.01f, 11.03f }, { DT_GOLD, IT_COINS, 9.09f, 11.09f },
    { DT_GOLD, IT_GEMS, 20.98f, 14.03f }, { DT_GOLD, IT_COINS, 14.97f, 14.10f }, { DT_GOLD, IT_CHALICE, 20.11f, 15.03f },
    { DT_GOLD, IT_COINS, 20.98f, 15.03f }, { DT_GOLD, IT_GEMS, 11.02f, 17.10f }, { DT_GOLD, IT_CHALICE, 12.67f, 18.04f },
    { DT_GOLD, IT_COINS, 16.03f, 18.04f }, { DT_GOLD, IT_CHALICE, 16.04f, 19.04f }, { DT_GOLD, IT_COINS, 8.08f, 21.04f },
    { DT_GOLD, IT_CHALICE, 13.03f, 22.05f }, { DT_GOLD, IT_CHALICE, 0.94f, 24.05f }, { DT_GOLD, IT_COINS, 4.95f, 24.05f },
    { DT_GOLD, IT_GEMS, 10.96f, 25.05f }, { DT_GOLD, IT_CHALICE, 12.03f, 25.12f }, { DT_GOLD, IT_COINS, 5.31f, 25.90f },
    { DT_GOLD, IT_CHEST, 3.44f, 26.54f }, { DT_GOLD, IT_CROWN, 3.35f, 26.96f }, { DT_GOLD, IT_COINS, 0.94f, 27.06f },
    { DT_GOLD, IT_GEMS, 3.63f, 27.37f }, { DT_GOLD, IT_CHALICE, 9.08f, 28.06f }, { DT_GOLD, IT_COINS, 13.10f, 28.06f },
    { DT_GOLD, IT_GEMS, 3.43f, 28.41f }, { DT_GOLD, IT_GEMS, 1.00f, 29.06f }, { DT_GOLD, IT_CHALICE, 3.67f, 29.22f },
    { DT_GOLD, IT_COINS, 3.33f, 29.82f }, { DT_GOLD, IT_COINS, 1.01f, 30.06f }, { DT_GOLD, IT_COINS, 12.03f, 30.06f },
    { DT_GOLD, IT_COINS, 19.98f, 31.00f }, { DT_GOLD, IT_COINS, 7.02f, 31.06f }, { DT_GOLD, IT_CHALICE, 14.06f, 31.06f },
    { DT_GOLD, IT_CHALICE, 18.95f, 31.09f }, { DT_GOLD, IT_GEMS, 21.11f, 31.13f }, { DT_KEY, 0, 11.20f, 24.57f },
    { DT_DEAD, MOB_SKEL, 15.10f, 12.96f }, { DT_DEAD, MOB_SKEL, 20.98f, 13.10f }, { DT_DEAD, MOB_SKEL, 19.04f, 14.97f },
    { DT_DEAD, MOB_SKEL, 16.04f, 16.04f }, { DT_DEAD, MOB_SKEL, 11.03f, 18.04f }, { DT_DEAD, MOB_SKEL, 14.97f, 20.04f },
    { DT_DEAD, MOB_SKEL, 5.08f, 21.98f }, { DT_DEAD, MOB_SKEL, 1.00f, 28.12f }, { DT_DEAD, MOB_SKEL, 5.95f, 31.06f },
    { DT_DEAD, MOB_SKEL, 9.09f, 31.07f }, { DT_DEAD, MOB_SKEL, 1.00f, 31.13f },
};
static const DrawnRoom DRAWN1_ROOM[] = {
    { 18, 12, 4, 4, RT_BURIAL }, { 10, 16, 3, 3, RT_OSSUARY }, { 10, 24, 3, 2, RT_OSSUARY },   /* the east room, the side room, the key room */
    { 0, 23, 3, 9, RT_UNDER }, { 8, 27, 7, 3, RT_CHAPTER }, { 3, 26, 1, 6, RT_KNIGHTS },     /* the west room, the big room, the vault */
};
static const char *const DRAWN2_MAP[35] = {
    "##S++.#########",    /*  0 */
    "####+.#########",    /*  1 */
    "###.+.#########",    /*  2 */
    "##..+..########",    /*  3 */
    "#+++++++#######",    /*  4 */
    "#.+.#.+.......#",    /*  5 */
    "#.+###++++++++#",    /*  6 */
    "#.+.#.+......+#",    /*  7 */
    "#.++#.+.####.+#",    /*  8 */
    "##.+#++#####.+#",    /*  9 */
    "###+#+##X####=#",    /* 10 */
    "++G+#+#.+.#.++.",    /* 11 */
    ".+###+#.+.#.++.",    /* 12 */
    ".+#+++#.+.#.++.",    /* 13 */
    ".+#+#+#.+.#.++.",    /* 14 */
    ".+#+#+#.+.#.++.",    /* 15 */
    ".+#+++#.+.#IiiI",    /* 16 */
    ".+#++##.+.#IiiI",    /* 17 */
    ".+#+h##.+.#IiiI",    /* 18 */
    ".+#+#+C+++#IiiI",    /* 19 */
    ".+#+#+######ii#",    /* 20 */
    "##.+#+.#####ii#",    /* 21 */
    "#..+#+..####ii#",    /* 22 */
    "#+++#+++++++++#",    /* 23 */
    "#.+###...+....#",    /* 24 */
    "#.+###...+....#",    /* 25 */
    "#+++#+++++++++#",    /* 26 */
    "#+++++++#####D#",    /* 27 */
    "##..+..##....+#",    /* 28 */
    "###.+.##++++++#",    /* 29 */
    "###.+.#+++++++#",    /* 30 */
    "###.+.#+.....+#",    /* 31 */
    "#######c#######",    /* 32 */
    "#######QQQ#####",    /* 33 */
    "#######qqq#####",    /* 34 */
};
static const DrawnThing DRAWN2_THING[] = {
    { DT_GOLD, IT_COINS, 5.30f, 2.50f }, { DT_GOLD, IT_CHALICE, 2.50f, 3.50f },                                     /* the start */
    { DT_GOLD, IT_COINS, 1.40f, 7.00f }, { DT_GOLD, IT_GEMS, 1.30f, 7.90f },                                        /* the west passage */
    { DT_GOLD, IT_COINS, 11.10f, 6.40f },                                                                           /* the east hall */
    { DT_GOLD, IT_CHALICE, 7.40f, 8.70f }, { DT_GOLD, IT_COINS, 5.60f, 12.50f }, { DT_GOLD, IT_COINS, 3.30f, 13.60f },
    { DT_GOLD, IT_GEMS, 5.70f, 15.50f },                                                                            /* the middle way */
    { DT_GOLD, IT_CHALICE, 11.30f, 13.90f }, { DT_GOLD, IT_GEMS, 11.30f, 17.40f },                                  /* the map room, the ice */
    { DT_GOLD, IT_COINS, 7.60f, 11.30f }, { DT_GOLD, IT_CHALICE, 9.30f, 11.40f }, { DT_GOLD, IT_CROWN, 9.60f, 18.60f },   /* the exit room */
    { DT_GOLD, IT_COINS, 1.60f, 14.90f }, { DT_GOLD, IT_CHEST, 0.40f, 20.60f }, { DT_GOLD, IT_GEMS, 1.60f, 20.60f },     /* the west room */
    { DT_GOLD, IT_COINS, 9.60f, 24.10f }, { DT_GOLD, IT_CHALICE, 12.20f, 25.40f },                                  /* the great hall */
    { DT_GOLD, IT_COINS, 1.30f, 27.50f }, { DT_GOLD, IT_COINS, 7.70f, 27.70f }, { DT_GOLD, IT_CHALICE, 2.80f, 28.80f },
    { DT_GOLD, IT_COINS, 4.00f, 30.50f }, { DT_GOLD, IT_GEMS, 4.90f, 31.70f },                                      /* the south-west lobe */
    { DT_GOLD, IT_COINS, 9.40f, 28.50f }, { DT_GOLD, IT_CHALICE, 8.60f, 29.30f },                                   /* the south-east room */
    { DT_SGOLD, IT_CHEST, 9.40f, 33.50f }, { DT_SGOLD, IT_GEMS, 8.50f, 33.60f }, { DT_SGOLD, IT_CROWN, 8.50f, 34.50f },
    { DT_SGOLD, IT_GEMS, 9.50f, 34.50f },                                                                           /* the secret */
    { DT_KEY, 0, 7.60f, 30.80f }, { DT_KEY, 2, 1.00f, 19.60f }, { DT_MAP, 0, 12.80f, 15.40f },
    { DT_DEAD, MOB_SKEL, 2.30f, 5.00f }, { DT_DEAD, MOB_BAT, 13.00f, 5.70f }, { DT_DEAD, MOB_SKEL, 3.40f, 9.40f },
    { DT_DEAD, MOB_SKEL, 11.70f, 12.60f }, { DT_DEAD, MOB_SHAMBLER, 7.90f, 13.10f }, { DT_DEAD, MOB_SKEL, 3.50f, 15.00f },
    { DT_DEAD, MOB_SKEL, 9.50f, 16.40f }, { DT_DEAD, MOB_BAT, 14.30f, 17.60f }, { DT_DEAD, MOB_SHAMBLER, 1.40f, 18.20f },
    { DT_DEAD, MOB_SKEL, 7.10f, 22.60f }, { DT_DEAD, MOB_SHAMBLER, 3.10f, 22.90f }, { DT_DEAD, MOB_SKEL, 11.60f, 26.30f },
    { DT_DEAD, MOB_BAT, 12.90f, 28.80f }, { DT_DEAD, MOB_SKEL, 9.30f, 30.80f }, { DT_DEAD, MOB_SHAMBLER, 3.60f, 31.30f },
    /* the things that want a spell (T22 a): a, b, g, j any line on the main way; c, d, e, f, h, i the class's own. a is moved a
     * row north and f, g, h set to their walls, off the lanes they were drawn on */
    { DT_MARK, 0, 5.50f, 3.50f }, { DT_MARK, 1, 11.80f, 7.60f }, { DT_MARK, 2, 2.40f, 9.50f }, { DT_MARK, 2, 7.30f, 14.80f },
    { DT_MARK, 2, 0.30f, 16.20f }, { DT_MARK, 2, 3.20f, 16.50f }, { DT_MARK, 0, 11.70f, 23.20f }, { DT_MARK, 2, 3.70f, 26.20f },
    { DT_MARK, 2, 5.50f, 31.40f }, { DT_MARK, 1, 11.10f, 31.60f },
    { DT_SWITCH, 0, 14.30f, 11.60f }, { DT_GSEAL, 0, 13.50f, 11.06f },
};
static const DrawnRoom DRAWN2_ROOM[] = {
    { 0, 11, 2, 10, RT_OSSUARY }, { 11, 11, 4, 5, RT_KNIGHTS }, { 7, 11, 3, 9, RT_CHAPTER },   /* the west room, the map room, the exit room */
    { 7, 28, 7, 4, RT_BURIAL }, { 5, 23, 9, 4, RT_UNDER }, { 1, 27, 5, 5, RT_UNDER },          /* the south-east room, the great hall, the lobe */
};
#define NDRAWN 2
static const Drawn DRAWN[NDRAWN] = {
    { DRAWN1_MAP, 22, 32, 11, 6, 0, DRAWN1_ROOM, (int)(sizeof DRAWN1_ROOM / sizeof DRAWN1_ROOM[0]), { 5, -1, -1 },
      DRAWN1_THING, (int)(sizeof DRAWN1_THING / sizeof DRAWN1_THING[0]), 0x7D1A0001u, 1, NULL, 0 },
    { DRAWN2_MAP, 15, 35, 15, 5, 0, DRAWN2_ROOM, (int)(sizeof DRAWN2_ROOM / sizeof DRAWN2_ROOM[0]), { 0, -1, 2 },
      DRAWN2_THING, (int)(sizeof DRAWN2_THING / sizeof DRAWN2_THING[0]), 0x7D1A0002u, 2, NULL, 0 },
};
#pragma GCC diagnostic pop
#define DRAWN_HALL 40                                              /* the halls' region on a drawn level */
static int drawn_level(int level) {                                /* which drawing a level is, 1-based; 0 none */
#ifdef CRAWL_DEBUG
    static int off = -1;
    if (off < 0) { const char *e = getenv("CRAWL_DRAWN"); off = e && atoi(e) == 0; }   /* 0: the generated levels, as before */
    if (off) return 0;
#endif
    if (PK.depth) return 0;
    for (int i = 0; i < NDRAWN; i++) if (DRAWN[i].level == level) return i + 1;   /* the level each is laid on (docs/PLAN-mapedit.md) */
    return 0;
}
static int drawn_tut(int level) { int d = drawn_level(level); return d >= 1 && d <= DRAWN_TUT; }   /* one of the tutorial's two */
static const Drawn *drawn(void) { return &DRAWN[g_drawn - 1]; }
static char drawn_ch(int x, int y) {                               /* the letter at a cell of the level's map */
    const Drawn *D = drawn();
    x -= D->ox; y -= D->oy;
    return x < 0 || y < 0 || x >= D->w || y >= D->h ? '#' : D->map[y][x];
}
static int drawn_front(int wx, int wy, int *fx, int *fy, int secret_ok) {   /* a wall's open side (north, south, west, east
                                                                      first): the cell it is used from, and the way; a secret room's
                                                                      floor only when nothing else is open and it may be */
    static const int ORDER[4] = { 3, 1, 2, 0 };
    for (int pass = 0; pass < 1 + !!secret_ok; pass++)
        for (int k = 0; k < 4; k++) {
            int d = ORDER[k];
            if (is_open(wx + DX[d], wy + DY[d]) && (pass || !G.map[wy + DY[d]][wx + DX[d]].secret)) { *fx = wx + DX[d]; *fy = wy + DY[d]; return (d + 2) & 3; }
        }
    *fx = *fy = -1;
    return 0;
}
static void drawn_clear(int x, int y) {                            /* the dressing taken off a cell the drawing wants */
    if (x < 1 || y < 1 || x >= MAPN - 1 || y >= MAPN - 1) return;
    if (G.map[y][x].kind == K_PROP) G.map[y][x].kind = K_FLOOR;
    G.foot[y][x] = 0; G.bulky[y][x] = 0;
    for (int d = 0; d < G.ndecos; d++) if ((int)G.decos[d].x == x && (int)G.decos[d].y == y) G.decos[d].spr = -1;
}
/* ---- where this play lays each thing (docs/PLAN-events.md, the picks): a thing tagged with rooms or spots stands at one of its
 * places, the run's dice choosing; the rest, and every thing on a level without picks, where drawn ---- */
static float drawn_tx(int i) { return i < EV_THINGS ? EV.tx[i] : drawn()->thing[i].x; }
static float drawn_ty(int i) { return i < EV_THINGS ? EV.ty[i] : drawn()->thing[i].y; }
static int drawn_lane_ch(char ch) { return ch == '+' || ch == 'f' || ch == 'i' || ch == 'Q' || ch == 'S' || ch == 'c' || ch == 'D' || ch == 'G' || ch == 'C' || ch == '=' || ch == 'B'; }
static char drawn_at(int x, int y) { const Drawn *D = drawn(); return x < 0 || y < 0 || x >= D->w || y >= D->h ? '#' : D->map[y][x]; }   /* the drawing's own cells */
static int drawn_spot_in(const DrawnRoom *r, float x, float y) { return x >= (float)r->x && y >= (float)r->y && x < (float)(r->x + r->w) && y < (float)(r->y + r->h); }
/* the map editor's roomSpot: the thing's own place if it stands in the room, else the floor cell nearest the room's middle that is
 * within a step of a lane, not a door, not the start; ties go up and left */
static int drawn_room_spot(int ri, int thing, float *px, float *py) {
    const Drawn *D = drawn();
    const DrawnRoom *r = &D->room[ri];
    const DrawnThing *t = &D->thing[thing];
    int cx = (int)floorf(t->x), cy = (int)floorf(t->y);
    if (cx >= r->x && cy >= r->y && cx < r->x + r->w && cy < r->y + r->h) { *px = t->x; *py = t->y; return 1; }
    double mx = r->x + (r->w - 1) / 2.0, my = r->y + (r->h - 1) / 2.0, bd = 1e9;
    int found = 0;
    for (int y = r->y; y < r->y + r->h; y++)
        for (int x = r->x; x < r->x + r->w; x++) {
            char ch = drawn_at(x, y);
            if (ch == '#' || ch == 'X' || ch == 'h' || ch == 'c' || ch == 'D' || ch == 'G' || ch == 'C' || ch == '=' || ch == 'B' || ch == 'S') continue;
            int near = drawn_lane_ch(ch);
            for (int d = 0; d < 4 && !near; d++) near = drawn_lane_ch(drawn_at(x + DX[d], y + DY[d]));
            if (!near) continue;
            double dd = fabs(x - mx) + fabs(y - my) + (double)(x + y) * 1e-4;
            if (dd < bd) { bd = dd; *px = (float)x + 0.5f; *py = (float)y + 0.5f; found = 1; }
        }
    return found;
}
static void drawn_picks(uint32_t *rs) {                            /* before the level is laid: on the run's dice, not the drawing's */
    const Drawn *D = drawn();
    for (int i = 0; i < D->nthing && i < EV_THINGS; i++) { EV.tx[i] = D->thing[i].x; EV.ty[i] = D->thing[i].y; }
    for (int i = 0; i < D->nspawn && i < EV_SPAWNS; i++) { EV.sx[i] = D->spawn[i].x; EV.sy[i] = D->spawn[i].y; }
    for (int p = 0; p < D->npick; p++) {
        const DrawnPick *P = &D->pick[p];
        if (P->thing < 0 || P->thing >= D->nthing || P->thing >= EV_THINGS) continue;
        float ox[64], oy[64]; int n = 0;
        for (int s = 0; s < D->nthing && n < 64 && P->tag >= 0; s++)   /* a spot of its tag */
            if (D->thing[s].what == DT_SPOT && D->thing[s].kind == P->tag) { ox[n] = D->thing[s].x; oy[n] = D->thing[s].y; n++; }
        for (int r = 0; r < D->nroom && r < 32 && n < 64; r++) {   /* each of its rooms: a spot standing there, else roomSpot */
            if (!(P->rooms >> r & 1)) continue;
            int here = 0;
            for (int s = 0; s < D->nthing && n < 64; s++)
                if (D->thing[s].what == DT_SPOT && drawn_spot_in(&D->room[r], D->thing[s].x, D->thing[s].y)) { ox[n] = D->thing[s].x; oy[n] = D->thing[s].y; n++; here = 1; }
            if (!here && drawn_room_spot(r, P->thing, &ox[n], &oy[n])) n++;
        }
        if (!n) continue;
        int k = (int)(rng_next(rs) % (uint32_t)n);
        EV.tx[P->thing] = ox[k]; EV.ty[P->thing] = oy[k];
#ifdef CRAWL_DEBUG
        fprintf(stderr, "DRAWN level %d: thing %d laid at (%.2f,%.2f), %d of %d places\n", G.level, P->thing, ox[k], oy[k], k + 1, n);
#endif
    }
    for (int i = 0; i < D->nspawn && i < EV_SPAWNS; i++) {         /* a spawn point at one of its tag's spots */
        if (!D->spawn[i].spot) continue;
        float ox[64], oy[64]; int n = 0;
        for (int s = 0; s < D->nthing && n < 64; s++)
            if (D->thing[s].what == DT_SPOT && D->thing[s].kind == D->spawn[i].spot - 1) { ox[n] = D->thing[s].x; oy[n] = D->thing[s].y; n++; }
        if (!n) continue;
        int k = (int)(rng_next(rs) % (uint32_t)n);
        EV.sx[i] = ox[k]; EV.sy[i] = oy[k];
    }
}
/* the owner's torches (from gen_dress, in place of the generator's): each on its wall's face, its light a static one whose share
 * events.c sets each frame */
static int drawn_torches(void) {
    const Drawn *D = drawn();
    if (!D->ntorch) return 0;                                      /* none hung: the generator's, on the drawing's dice, as before */
    for (int i = 0; i < D->ntorch && EV.nt < EV_TORCHES; i++) {
        const DrawnTorch *t = &D->torch[i];
        int wx = D->ox + t->x, wy = D->oy + t->y, d = t->d & 3, fx = wx + DX[d], fy = wy + DY[d];
        if (wx < 1 || wy < 1 || wx >= MAPN - 1 || wy >= MAPN - 1 || G.map[wy][wx].kind != K_WALL || G.map[wy][wx].secret || !is_open(fx, fy)) continue;
        Cell *w = &G.map[wy][wx];
        w->tex = T_TORCH0; w->ftorch = (uint8_t)(d + 1);
        int k = EV.nt++;
        EV.torch[k].wx = wx; EV.torch[k].wy = wy; EV.torch[k].d = d; EV.torch[k].group = t->group; EV.torch[k].lit = t->lit != 0;
        EV.torch[k].light = G.nlights < MAXLIGHTS ? G.nlights : -1;
        light_static(fx + 0.5f - DX[d] * 0.42f, fy + 0.5f - DY[d] * 0.42f, 1.55f, 4.6f);   /* as the generator's */
        if (EV.torch[k].light >= 0) EV.light_of[EV.torch[k].light] = (int8_t)(k + 1);
        EV.at_torch[wy][wx] = (int8_t)(k + 1);
    }
    return D->ntorch;
}
/* the owner's fire and ice (docs/PLAN-events.md): each field every cell of its kind joined to its own, with its settings */
static void ev_fields(void) {
    const Drawn *D = drawn();
    int nc = 0;
    static int qx[MAPN * MAPN], qy[MAPN * MAPN];
    for (int i = 0; i < D->nfield && EV.nf < EV_FIELDS; i++) {
        const DrawnField *F = &D->field[i];
        int sx = D->ox + F->x, sy = D->oy + F->y, fire = F->kind == DF_FIRE;
        int k = EV.nf++;
        EV.f[k].kind = F->kind; EV.f[k].on = F->on != 0; EV.f[k].dmg = F->dmg; EV.f[k].chance = F->chance; EV.f[k].kills = F->kills;
        EV.f[k].first = nc; EV.f[k].n = 0; EV.f[k].t = 99.0f; EV.f[k].fade = EV.f[k].on ? 1.0f : 0.0f;
        char c0 = drawn_ch(sx, sy);
        if (fire ? c0 != 'F' && c0 != 'f' : c0 != 'I' && c0 != 'i') continue;
        int qh = 0, qt = 0;
        qx[qt] = sx; qy[qt] = sy; qt++; EV.fat[sy][sx] = (uint8_t)(k + 1);
        while (qh < qt) {
            int x = qx[qh], y = qy[qh]; qh++;
            if (nc < EV_FCELLS) { EV.fx[nc] = x; EV.fy[nc] = y; nc++; EV.f[k].n++; }
            for (int d = 0; d < 4; d++) {
                int nx = x + DX[d], ny = y + DY[d];
                char ch = drawn_ch(nx, ny);
                if (nx < 1 || ny < 1 || nx >= MAPN - 1 || ny >= MAPN - 1 || EV.fat[ny][nx] || (fire ? ch != 'F' && ch != 'f' : ch != 'I' && ch != 'i')) continue;
                EV.fat[ny][nx] = (uint8_t)(k + 1); qx[qt] = nx; qy[qt] = ny; qt++;
            }
        }
    }
}
static void gen_drawn(void) {
    const Drawn *D = drawn();
    G.faced = crawl_drawn(G.level); G.look = crawl_set(G.level);
    memset(nr_type, 0, sizeof nr_type);
    memset(G.map, 0, sizeof G.map);
    for (int y = 0; y < MAPN; y++)
        for (int x = 0; x < MAPN; x++) { G.map[y][x].kind = K_WALL; G.map[y][x].region = -1; G.map[y][x].door = -1; }
    memset(MM, 0, sizeof MM);                                      /* no plan: the dressing's crossings find none */
    for (int y = 0; y < MG; y++) for (int x = 0; x < MG; x++) { MM[y][x].kind = K_WALL; MM[y][x].region = -1; MM[y][x].room = -1; }
    G.nrooms = 0; G.ndoors = 0; G.nlights = 0; G.nlc = 0; G.ndecos = 0;
    memset(G.bulky, 0, sizeof G.bulky); memset(G.mapped, 0, sizeof G.mapped); memset(G.foot, 0, sizeof G.foot);
    G.nvaults = 0;
    for (int k = 0; k < NKEYS; k++) G.vault_room[k] = G.vault_door[k] = -1;
    G.pz = -1; G.pz_room = G.pz_door = -1; G.pz_exit = 0;           /* no puzzle, trial, lair or secret */
    G.tri = -1; G.tri_room = G.tri_door = -1; G.lairs = 0;
    G.nsecrets = G.secrets_found = G.nlinks = G.links_found = G.trials_failed = 0; G.slide = -1;
    memset(g_lane, 0, sizeof g_lane);
    memset(&DF, 0, sizeof DF);
    memset(&IC, 0, sizeof IC); IC.sw = IC.gate = IC.gate_door = -1; IC.live = 1; IC.t = 99.0f;   /* the ice, laid */
    memset(g_gate, 0, sizeof g_gate);
    for (int r = 0; r < D->nroom && r < MAXROOMS; r++) {
        const DrawnRoom *dr = &D->room[r];
        G.rooms[r] = (Room){ D->ox + dr->x, D->oy + dr->y, dr->w, dr->h, r, RTYPES[dr->rtype].theme, dr->rtype };
        mroom[r][0] = mroom[r][1] = 0; mroom[r][2] = mroom[r][3] = 1;   /* a block each, so nothing laid by the plan's blocks */
        nr_type[dr->rtype]++; G.nrooms++;
    }
    for (int y = 0; y < D->h; y++)
        for (int x = 0; x < D->w; x++) {
            char ch = D->map[y][x];
            int mx = D->ox + x, my = D->oy + y, rg = DRAWN_HALL, th = TH_CORRIDOR, door = ch == 'D' || ch == 'G' || ch == 'C' || ch == '=' || ch == 'B';
            if (ch == 'c') { G.map[my][mx].secret = 2; g_lane[my][mx] = 1; }   /* a secret's wall: a lane once it has slid */
            if (ch == '#' || ch == 'h' || ch == 'X' || ch == 'c') continue;
            if (ch == 'q' || ch == 'Q') {                          /* the secret's room, carved as a secret's is */
                carve(mx, my, -2, TH_CORRIDOR); G.map[my][mx].secret = 1;
                if (ch == 'Q') g_lane[my][mx] = 1;
                continue;
            }
            if (!door)
                for (int r = 0; r < G.nrooms; r++) if (in_room(&G.rooms[r], mx, my)) { rg = r; th = G.rooms[r].theme; }
            carve(mx, my, rg, th);
            if (door) G.map[my][mx].kind = K_DOOR;
            if (ch == '+' || ch == 'f' || ch == 'i' || door || ch == 'S') g_lane[my][mx] = 1;
            if (D->nfield) continue;                               /* its fields are the events' (ev_fields), not the tutorial's */
            if ((ch == 'I' || ch == 'i') && IC.n < ICE_CELLS) { IC.cx[IC.n] = mx; IC.cy[IC.n] = my; IC.n++; IC.at[my][mx] = (uint8_t)IC.n; }
            if ((ch == 'F' || ch == 'f') && DF.n < DFIRE_CELLS) {
                DF.cx[DF.n] = mx; DF.cy[DF.n] = my; DF.n++; DF.at[my][mx] = 1;
                DF.fade = 1.0f; DF.past_y = (float)my + 3.0f;      /* the last row's far edge and two cells more (south: the drawing's way on) */
            }
        }
    for (int y = 0; y < MAPN; y++)                                 /* the doors; a gold door is the vault's */
        for (int x = 0; x < MAPN; x++) {
            if (G.map[y][x].kind != K_DOOR || G.ndoors >= MAXDOORS) continue;
            char ch = drawn_ch(x, y);
            int lk = ch == 'G' || ch == 'C', col = ch == 'C' ? 2 : 0, ax = is_open(x - 1, y) && is_open(x + 1, y);
            G.map[y][x].door = (int16_t)G.ndoors;
            G.doors[G.ndoors++] = (Door){ x, y, ax, 0.0f, 0.0f, 0, ch == '=' || ch == 'B' ? GATED : lk, col, 0 };
            if (ch == 'B') g_gate[G.ndoors - 1] = 1;               /* barred: a portcullis, till an event raises it */
            if (lk) G.vault_door[col] = G.ndoors - 1;
            if (ch == '=') IC.gate_door = G.ndoors - 1;            /* the one-way gate: barred till its seal is struck from the south */
        }
    for (int k = 0; k < NKEYS; k++) if (D->vault[k] >= 0) { G.vault_room[k] = D->vault[k]; G.nvaults = k + 1; }
    for (int y = 0; y < D->h; y++)                                 /* the secrets, laid by hand (7.5): each wall its own room, the q */
        for (int x = 0; x < D->w && G.nsecrets < MAXSECRETS; x++) {   /* cells joined to the cell behind it (docs/PLAN-events.md) */
            if (D->map[y][x] != 'c') continue;
            int sx = D->ox + x, sy = D->oy + y, in = -1, len = 0, x0 = MAPN, y0 = MAPN, x1 = -1, y1 = -1;
            for (int d = 0; d < 4 && in < 0; d++) if (is_open(sx - DX[d], sy - DY[d]) && !G.map[sy - DY[d]][sx - DX[d]].secret) in = d;
            if (in < 0) continue;
            while (G.map[sy + DY[in] * (len + 1)][sx + DX[in] * (len + 1)].secret == 1) len++;
            static int qx[MAPN * MAPN], qy[MAPN * MAPN];
            static uint8_t seen[MAPN][MAPN];
            memset(seen, 0, sizeof seen);
            int qh = 0, qt = 0;
            if (G.map[sy + DY[in]][sx + DX[in]].secret == 1) { qx[qt] = sx + DX[in]; qy[qt] = sy + DY[in]; seen[qy[qt]][qx[qt]] = 1; qt++; }
            while (qh < qt) {
                int cx = qx[qh], cy = qy[qh]; qh++;
                x0 = imin(x0, cx); y0 = imin(y0, cy); x1 = imax(x1, cx); y1 = imax(y1, cy);
                for (int d = 0; d < 4; d++) {
                    int nx = cx + DX[d], ny = cy + DY[d];
                    if (nx < 1 || ny < 1 || nx >= MAPN - 1 || ny >= MAPN - 1 || seen[ny][nx] || G.map[ny][nx].secret != 1) continue;
                    seen[ny][nx] = 1; qx[qt] = nx; qy[qt] = ny; qt++;
                }
            }
            if (x1 < 0) { x0 = x1 = sx + DX[in]; y0 = y1 = sy + DY[in]; }   /* (no room drawn behind it: the cell, as an empty closet) */
            G.secrets[G.nsecrets++] = (Secret){ sx - DX[in], sy - DY[in], in, sx, sy, 0, 0, 0, len, 1, -1, -1, x0, y0, x1, y1, D->seed | 1u, 0, 0, 0 };
            G.nlinks++;
#ifdef CRAWL_DEBUG
            if (g_drawn > DRAWN_TUT) fprintf(stderr, "DRAWN level %d: secret %d at (%d,%d), its room (%d,%d)-(%d,%d)\n", G.level, G.nsecrets, x, y, x0 - D->ox, y0 - D->oy, x1 - D->ox, y1 - D->oy);
#endif
        }
    ev_fields();                                                   /* the owner's fire and ice, by field */
    gen_dress();
    for (int i = 0; i < D->nthing; i++)                            /* what the drawing sets down, and the ice, clear of the dressing */
        if (D->thing[i].what >= DT_SGOLD) drawn_clear(D->ox + (int)drawn_tx(i), D->oy + (int)drawn_ty(i));
    for (int i = 0; i < IC.n; i++) drawn_clear(IC.cx[i], IC.cy[i]);
    for (int i = 0; i < EV.nf; i++) if (EV.f[i].kind == DF_ICE) for (int j = EV.f[i].first; j < EV.f[i].first + EV.f[i].n; j++) drawn_clear(EV.fx[j], EV.fy[j]);
    for (int i = 0; i < D->nspawn && i < EV_SPAWNS; i++) if (D->spawn[i].spot) drawn_clear(D->ox + (int)EV.sx[i], D->oy + (int)EV.sy[i]);
    for (int y = 0; y < MAPN; y++)                                 /* and no prop on a lane: the dressing doesn't know the drawn ones */
        for (int x = 0; x < MAPN; x++) if (g_lane[y][x] && G.map[y][x].kind == K_PROP) drawn_clear(x, y);
    for (int i = 0; i < IC.n; i++) {                               /* each cell's wait as the ice turns: by the walls first */
        int open = 0;
        for (int d = 0; d < 4; d++) open += IC.at[IC.cy[i] + DY[d]][IC.cx[i] + DX[d]] != 0;
        IC.wait[i] = 0.3f * (float)open + 0.15f * (float)(hash2(IC.cx[i], IC.cy[i]) & 3);
    }
    for (int i = 0; i < EV.nf; i++)                                /* and each field's, the same way */
        for (int j = EV.f[i].first; j < EV.f[i].first + EV.f[i].n; j++) {
            int open = 0;
            for (int d = 0; d < 4; d++) open += EV.fat[EV.fy[j] + DY[d]][EV.fx[j] + DX[d]] == i + 1;
            EV.fwait[j] = 0.3f * (float)open + 0.15f * (float)(hash2(EV.fx[j], EV.fy[j]) & 3);
        }
    static int qx[MAPN * MAPN], qy[MAPN * MAPN];                   /* each cell's steps to a lane, for what lies within reach of one */
    int qh = 0, qt = 0;
    for (int y = 0; y < MAPN; y++)
        for (int x = 0; x < MAPN; x++) {
            g_ldist[y][x] = g_lane[y][x] ? 0 : 9;
            if (g_lane[y][x]) { qx[qt] = x; qy[qt] = y; qt++; }
        }
    while (qh < qt) {
        int x = qx[qh], y = qy[qh]; qh++;
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (!is_open(nx, ny) || g_ldist[ny][nx] <= g_ldist[y][x] + 1) continue;
            g_ldist[ny][nx] = (uint8_t)(g_ldist[y][x] + 1); qx[qt] = nx; qy[qt] = ny; qt++;
        }
    }
}
static void drawn_start(void) {
    const Drawn *D = drawn();
    for (int y = 0; y < D->h; y++) for (int x = 0; x < D->w; x++) if (D->map[y][x] == 'S') { G.cx = D->ox + x; G.cy = D->oy + y; }
}
static void drawn_items(void) {                                    /* the gold and the keys where drawn: no potion */
    const Drawn *D = drawn();
    G.nitems = 0; G.items_total = 0; G.items_got = 0; G.value_total = G.value_got = 0;
    for (int i = 0; i < D->nthing; i++) {
        const DrawnThing *t = &D->thing[i];
        if (t->what != DT_GOLD && t->what != DT_KEY && t->what != DT_SGOLD && t->what != DT_MAP && t->what != DT_COMPASS) continue;
        float x = D->ox + drawn_tx(i), y = D->oy + drawn_ty(i);
        int type = t->what == DT_KEY ? IT_KEY : t->what == DT_MAP ? (RUN.has_map ? IT_CHALICE : IT_MAP) : t->what == DT_COMPASS ? IT_COMPASS : t->kind;   /* the map held: gold (T18 a) */
        Item *it = add_item((int)x, (int)y, type);
        if (!it) continue;
        it->x = x; it->y = y;
        if (D->ntrig) it->ev = i + 1;                              /* a trigger may watch it picked up (docs/PLAN-events.md) */
        if (t->what == DT_KEY) it->color = t->kind;
        if (t->what == DT_SGOLD) { it->secret = 1; G.items_total--; G.value_total -= treasure_value(type); }   /* the secret's, out of the count */
        if (type == IT_MAP) { int nl = G.nlights; light_static(x, y, 0.55f, 2.2f); if (G.nlights > nl) G.lights[nl].phase = -1000.0f; }   /* its glow */
    }
}
static void drawn_walls(void) {                                    /* the exit and the places to mend, in the walls drawn */
    const Drawn *D = drawn();
    G.exit_fx = G.exit_fy = -1; G.exit_in_vault = -1;
    G.nrest = 0; G.rest_on = 0; G.rest_goal = -1;
    for (int y = 0; y < D->h; y++)
        for (int x = 0; x < D->w; x++) {
            char ch = D->map[y][x];
            int wx = D->ox + x, wy = D->oy + y, fx, fy, d;
            if (ch == 'X') {
                d = drawn_front(wx, wy, &fx, &fy, 0);
                if (fx < 0) continue;
                G.exit_fx = fx; G.exit_fy = fy; G.exit_dir = d; G.exit_wx = wx; G.exit_wy = wy;
                G.exit_in_vault = vault_of_room(fx, fy);           /* level 2's is behind the crimson door */
                G.map[wy][wx].tex = T_EXIT0;
                light_static(fx + 0.5f + DX[d] * 0.35f, fy + 0.5f + DY[d] * 0.35f, 0.9f, 3.2f);   /* its runes light the stone */
            } else if (ch == 'h' && G.nrest < 2) {
                d = drawn_front(wx, wy, &fx, &fy, 1);                 /* it may face a secret room (the owner's word) */
                if (fx < 0) continue;
                G.map[wy][wx].tex = T_REST;
                G.rest_x[G.nrest] = wx; G.rest_y[G.nrest] = wy; G.rest_fx[G.nrest] = fx; G.rest_fy[G.nrest] = fy; G.nrest++;
                light_static(fx + 0.5f + DX[d] * 0.38f, fy + 0.5f + DY[d] * 0.38f, 0.75f, 2.8f);   /* its candles */
            }
        }
#ifdef CRAWL_DEBUG
    fprintf(stderr, "DRAWN level %d: exit (%d,%d) before (%d,%d), %d to mend, %d rooms, %d doors\n", G.level, G.exit_wx, G.exit_wy, G.exit_fx, G.exit_fy, G.nrest, G.nrooms, G.ndoors);
#endif
}
static void drawn_dead(void) {                                     /* the dead where drawn, rank I, each at its post */
    const Drawn *D = drawn();
    for (int i = 0; i < D->nthing; i++) {
        const DrawnThing *t = &D->thing[i];
        if (t->what != DT_DEAD) continue;
        float x = D->ox + drawn_tx(i), y = D->oy + drawn_ty(i);
        Mob *m = t->kind == MOB_SHAMBLER ? riser_make(t->kind, (int)x, (int)y, -1) : spawn_mob((int)x, (int)y, t->kind);   /* under turned earth */
        if (!m) continue;
        m->x = x; m->y = y;
        set_rank(m, RK_I);
        m->home = G.map[m->cy][m->cx].region;
    }
}
/* the owner's spawn points (docs/PLAN-mapedit.md): wake 0, those filled as the level is laid; else those of wake group wake - 1, now
 * (an event's, docs/PLAN-events.md) */
static int drawn_fill(int wake) {
    const Drawn *D = drawn();
    float B = budget_of(G.level) * (G.corrupt ? 1.4f : 1.0f) * (G.boss >= 0 ? 0.5f : 1.0f);
    int heads, made = 0, tried = 0;
    float spent = set_spent(&heads);
    for (int pass = 0; pass < 2; pass++)                           /* the kinds the owner chose first, then the budget's choices */
        for (int i = 0; i < D->nspawn; i++) {
            const DrawnSpawn *s = &D->spawn[i];
            if ((s->kind < 0) != (pass == 1) || s->wake != wake) continue;
            tried++;
            if (rnd(100) >= s->chance || heads >= 48) continue;
            float sx = i < EV_SPAWNS ? EV.sx[i] : s->x, sy = i < EV_SPAWNS ? EV.sy[i] : s->y;   /* (at a spot, as this play lays it) */
            int x = D->ox + (int)sx, y = D->oy + (int)sy;
            if (x < 1 || y < 1 || x >= MAPN - 1 || y >= MAPN - 1 || G.map[y][x].kind != K_FLOOR || occupied(x, y, NULL)) continue;
            int type = s->kind >= 0 ? s->kind : random_type();
            if (s->kind < 0 && spent + KIND_COST[type] * rank_cost(s->rank >= 0 ? s->rank : RK_I) > B) continue;   /* the budget spent */
            Mob *m = spawn_mob(x, y, type);
            if (!m) continue;
            m->x = D->ox + sx; m->y = D->oy + sy;
            if (s->rank >= 0) set_rank(m, s->rank);
            m->home = G.map[y][x].region;
            m->dsp = 1; m->dsp_ech = s->echance; m->dsp_el = s->elem;
            spent += KIND_COST[type] * rank_cost(m->rank); heads++; made++;
        }
#ifdef CRAWL_DEBUG
    if (!wake) fprintf(stderr, "DRAWN level %d: %d of %d spawn points filled, the dead cost %.2f of %.2f\n", G.level, made, tried, spent, B);
    else fprintf(stderr, "EVENT ev_wake group %d: %d of %d filled, the dead cost %.2f of %.2f t=%.2f\n", wake - 1, made, tried, spent, B, G.t);
#endif
    return made;
}
static void drawn_spawns(void) {                                   /* after the dead drawn */
    if (!drawn()->nspawn) return;                                  /* none: no dice thrown, so the tutorial's levels lie as before */
    drawn_fill(0);
}
static void drawn_marks(void) {                                    /* level 2's things that want a spell, its switch and its gate's seal */
    const Drawn *D = drawn();
    G.nmk = 0; G.ward_said = 0;
    int own = cls_el() >= 0 ? cls_el() : LEDGER.lent - 1;          /* the line held: the class's, or one lent */
    for (int e = 0; e < NELEM && (own < 0 || !align_open(own)); e++) if (align_open(e)) own = e;
    if (own < 0 || !align_open(own)) own = -1;
    for (int i = 0, n = 0; i < D->nthing; i++) {
        const DrawnThing *t = &D->thing[i];
        if (t->what != DT_MARK && t->what != DT_SWITCH && t->what != DT_GSEAL) continue;
        float x = D->ox + drawn_tx(i), y = D->oy + drawn_ty(i);
        int kind = t->what == DT_SWITCH ? MKK_RSEAL : t->what == DT_GSEAL ? MKK_GSEAL : t->kind == 0 ? MKK_COFFER : t->kind == 1 ? MKK_CAGE : -1;
        int el = own >= 0 ? own : EL_PHYS;                         /* the switch and the gate take any spell, whatever is held */
        if (kind < 0) { if (own < 0) continue; kind = own == EL_FIRE && (n++ & 1) ? MKK_RACK : MK_OWN[own]; }
        else if (t->what == DT_MARK && own < 0) continue;
        Marked *m = mk_add(kind, el, (int)x, (int)y);
        if (!m) continue;
        m->x = m->hx = m->vx = x; m->y = m->hy = m->vy = y;
        if (m->deco >= 0) mk_deco(m, MKK[kind].s0, 1);
        if (kind == MKK_RSEAL) { m->pips = 3; IC.sw = G.nmk - 1; }
        if (kind == MKK_GSEAL) { m->pips = 1; m->door = IC.gate_door; IC.gate = G.nmk - 1; }
        EV.mk[G.nmk - 1] = i + 1;                                  /* for a trigger that watches it struck */
    }
#ifdef CRAWL_DEBUG
    fprintf(stderr, "DRAWN level %d: %d things that want a spell (line %d), the switch %d, the gate's seal %d (door %d), %d ice\n", G.level, G.nmk, own, IC.sw, IC.gate, IC.gate_door, IC.n);
    if (getenv("CRAWL_ICE") && !atoi(getenv("CRAWL_ICE"))) IC.live = 0;   /* testing: the ice down from the start */
#endif
}
static int thing_new(int role, float x, float y, int spr);         /* things.c */
/* the events' things (docs/PLAN-events.md), after the puzzles' are cleared: relics carried as a puzzle's things are, the sockets
 * that take them, levers and plates; each in the puzzles' own looks. The compass is an item (drawn_items); a spot is nothing */
static void drawn_events(void) {
    const Drawn *D = drawn();
    for (int i = 0; i < D->nthing; i++) {
        const DrawnThing *t = &D->thing[i];
        float x = D->ox + drawn_tx(i), y = D->oy + drawn_ty(i);
        int k = -1;
        switch (t->what) {
        case DT_RELIC:
            k = thing_new(TR_ITEM, x, y, SP_PZ_FOSSIL);
            if (k >= 0) { snprintf(G.thing[k].name, sizeof G.thing[k].name, "%s", t->kind >= 0 && t->kind < D->nrelic && D->relic[t->kind] ? D->relic[t->kind] : "A RELIC"); G.thing[k].val = t->kind; }
            break;
        case DT_SOCKET:
            k = thing_new(TR_PLACE, x, y, SP_PZ_PLINTH);
            if (k >= 0) { snprintf(G.thing[k].name, sizeof G.thing[k].name, "A PLINTH"); G.thing[k].hold = 0.18f; G.thing[k].want = t->kind; }
            break;
        case DT_LEVER:
            k = thing_new(TR_LEVER, x, y, SP_PZ_LEVER0);
            if (k >= 0) { snprintf(G.thing[k].name, sizeof G.thing[k].name, "A LEVER"); G.thing[k].frames = 2; }
            break;
        case DT_PLATE:
            k = thing_new(TR_PLATE, x, y, -1);
            if (k >= 0) snprintf(G.thing[k].name, sizeof G.thing[k].name, "A FLAGSTONE");
            break;
        default: break;
        }
        if (k >= 0) G.thing[k].ev = i + 1;
    }
#ifdef CRAWL_DEBUG
    fprintf(stderr, "DRAWN level %d: %d events' things, %d torches, %d triggers, %d fields\n", G.level, G.nthing, EV.nt, D->ntrig, EV.nf);
#endif
}

static void ev_field_lights(void);
static void drawn_fire_lights(void) {                              /* a light a row, flickering, low as it goes out */
    ev_field_lights();
    if (!g_drawn || !DF.n || DF.fade <= 0.02f) return;
    for (int i = 0; i < DF.n; i++) {
        if (i && DF.cy[i] == DF.cy[i - 1]) continue;               /* the first cell of each row */
        float sx = 0.0f; int k = 0;
        for (int j = i; j < DF.n && DF.cy[j] == DF.cy[i]; j++) { sx += (float)DF.cx[j] + 0.5f; k++; }
        light_dynamic(sx / (float)k, (float)DF.cy[i] + 0.5f, 1.6f * DF.fade * (0.82f + 0.18f * sinf((float)G.t * (13.0f + (float)i) + (float)i)), 3.6f);
    }
}
static void drawn_fire_tick(float dt) {                            /* every frame, whoever plays */
    if (!g_drawn || !DF.n || DF.fade <= 0.0f) return;
    if (DF.out_t > 0.0f) {                                         /* past: burning low, then out */
        DF.out_t -= dt; DF.fade = fmaxf(0.0f, DF.out_t / DFIRE_OUT);
#ifdef CRAWL_DEBUG
        if (DF.fade <= 0.0f) fprintf(stderr, "EVENT drawn_fire out t=%.2f\n", G.t);
#endif
    } else if (!G.dead && G.py >= DF.past_y) {
        DF.out_t = DFIRE_OUT;
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT drawn_fire past hp=%d/%d burn=%.1f t=%.2f\n", G.hp, G.max_hp, G.af_burn, G.t);
#endif
    }
    if ((DF.crackle -= dt) <= 0.0f) {                              /* heard near it */
        DF.crackle = 0.8f + 0.6f * rndf();
        int i = rnd(DF.n);
        sfx_at(SFX_CRACKLE, (float)DF.cx[i] + 0.5f, (float)DF.cy[i] + 0.5f, 0.9f * DF.fade);
    }
    int cx = (int)floorf(G.px), cy = (int)floorf(G.py);
    int in = cx >= 0 && cy >= 0 && cx < MAPN && cy < MAPN && DF.at[cy][cx];
    if (!in || G.dead || DF.fade < 0.35f || G.shop) { DF.inside = 0; return; }
    float floor_hp = DFIRE_FLOOR * (float)G.max_hp;
    if (!DF.inside) {                                              /* the first touch: alight */
        DF.inside = 1; DF.tick = 0.0f;
        G.blame = (Blame){ BL_HAZARD, HZ_FIRE, 0 };
        if (af_can(AF_FIRE)) af_ignite(fminf(DFIRE_AFTER * (float)G.max_hp, fmaxf(0.0f, (float)G.hp - floor_hp)));
    }
    if ((DF.tick -= dt) > 0.0f) return;
    DF.tick += DFIRE_TICK;
    float room = (float)G.hp - floor_hp - G.af_burn, dmg = fminf(DFIRE_SHARE * (float)G.max_hp * DF.fade, room);
    if (dmg >= 1.0f) { G.blame = (Blame){ BL_HAZARD, HZ_FIRE, 0 }; harm_next = EL_FIRE; hurt_ex(dmg, HURT_ELEM, G.px, G.py); }
    G.af_burn = fminf(G.af_burn, fmaxf(0.0f, (float)G.hp - floor_hp));   /* nor the burning after, below the floor */
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT drawn_fire hurt %.1f hp=%d/%d burn=%.1f %s t=%.2f\n", dmg, G.hp, G.max_hp, G.af_burn, G.player ? "player" : "autopilot", G.t);
#endif
}

/* ---- the owner's fields (docs/PLAN-events.md), on a drawn level that exports them in place of the tutorial's one fire and one ice:
 * each lit or put out by an event; fire harms its share of health each half second while stood in, with its chance of setting you
 * On Fire at the first touch; ice gives a Chilled stack each 0.4 s at its chance and holds what stands on it. Unless it may kill,
 * neither takes you under a quarter of your health, as the tutorial's. Out, the floor is burnt out or thawed and does nothing ---- */
static float ev_ice_k(int f, int j) {                              /* an ice cell: how far laid, 0 bare, 1 ice */
    const EvField *F = &EV.f[f];
    float u = clampf((F->t - (F->on ? EV.fwait[j] : 1.2f - EV.fwait[j])) / ICE_TURN, 0.0f, 1.0f);
    return F->on ? u : 1.0f - u;
}
static int ev_ice_at(int x, int y) {                               /* an event's ice to stand on here */
    if (x < 0 || y < 0 || x >= MAPN || y >= MAPN || !EV.fat[y][x]) return 0;
    int f = EV.fat[y][x] - 1;
    if (EV.f[f].kind != DF_ICE) return 0;
    for (int j = EV.f[f].first; j < EV.f[f].first + EV.f[f].n; j++) if (EV.fx[j] == x && EV.fy[j] == y) return ev_ice_k(f, j) > 0.5f;
    return 0;
}
static int ev_fire_live(int f) { return EV.f[f].kind == DF_FIRE && EV.f[f].fade >= 0.35f; }
static void ev_field_lights(void) {                                /* each burning field: a light a row, as the tutorial's */
    if (!g_drawn || !EV.nf) return;
    for (int f = 0; f < EV.nf; f++) {
        const EvField *F = &EV.f[f];
        if (F->kind != DF_FIRE || F->fade <= 0.02f) continue;
        float sx[MAPN] = { 0 }; int k[MAPN] = { 0 };
        for (int j = F->first; j < F->first + F->n; j++) { sx[EV.fy[j]] += (float)EV.fx[j] + 0.5f; k[EV.fy[j]]++; }
        for (int y = 0; y < MAPN; y++)
            if (k[y]) light_dynamic(sx[y] / (float)k[y], (float)y + 0.5f, 1.6f * F->fade * (0.82f + 0.18f * sinf((float)G.t * (13.0f + (float)y) + (float)y)), 3.6f);
    }
}
static void ev_fields_tick(float dt) {                             /* every frame, whoever plays */
    if (!g_drawn || !EV.nf || PK.depth || G.shop) return;
    for (int f = 0; f < EV.nf; f++) {
        EvField *F = &EV.f[f];
        F->t += dt;
        if (F->kind == DF_FIRE) F->fade = F->on ? fminf(1.0f, F->fade + dt) : fmaxf(0.0f, F->fade - dt / 1.5f);   /* it catches, or burns low */
    }
    for (int j = 0; j < MAXMOBS; j++) {                            /* what stands on live ice is held in it */
        Mob *m = &G.mobs[j];
        if (m->alive && m->state == MS_WALK && !KIND_IS(m, KF_SPIRIT) && !buried(m) && ev_ice_at((int)m->x, (int)m->y)) m->frozen = fmaxf(m->frozen, 0.3f);
    }
    int cx = (int)floorf(G.px), cy = (int)floorf(G.py), fi = cx >= 0 && cy >= 0 && cx < MAPN && cy < MAPN ? EV.fat[cy][cx] : 0;
    if (!fi || G.dead || (EV.f[fi - 1].kind == DF_FIRE ? !ev_fire_live(fi - 1) : !ev_ice_at(cx, cy))) { EV.f_in = 0; return; }
    const EvField *F = &EV.f[fi - 1];
    float floor_hp = F->kills ? 0.0f : DFIRE_FLOOR * (float)G.max_hp;
    if (F->kind == DF_ICE) {
        if (EV.f_in != fi) { EV.f_in = fi; EV.f_stack = 0.0f; }
        if ((EV.f_stack -= dt) > 0.0f) return;
        EV.f_stack = ICE_STACK;
        if (rnd(100) >= F->chance || !af_can(AF_CHILL)) return;
        if (G.af_n < 3) af_chill(1, 0);                            /* held at three on it, as the tutorial's */
        else G.af[AF_CHILL] = G.af0[AF_CHILL];
        return;
    }
    if (EV.f_in != fi) {                                           /* the first touch: alight, by its chance */
        EV.f_in = fi; EV.f_tick = 0.0f;
        G.blame = (Blame){ BL_HAZARD, HZ_FIRE, 0 };
        if (rnd(100) < F->chance && af_can(AF_FIRE)) af_ignite(fminf(DFIRE_AFTER * (float)G.max_hp, fmaxf(0.0f, (float)G.hp - floor_hp)));
    }
    if ((EV.f_tick -= dt) > 0.0f) return;
    EV.f_tick += DFIRE_TICK;
    float room = (float)G.hp - floor_hp - (F->kills ? 0.0f : G.af_burn), dmg = fminf((float)F->dmg / 100.0f * (float)G.max_hp * F->fade, room);
    if (dmg >= 1.0f) { G.blame = (Blame){ BL_HAZARD, HZ_FIRE, 0 }; harm_next = EL_FIRE; hurt_ex(dmg, HURT_ELEM, G.px, G.py); }
    if (!F->kills) G.af_burn = fminf(G.af_burn, fmaxf(0.0f, (float)G.hp - floor_hp));   /* nor the burning after, below the floor */
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT ev_fire_floor %d hurt %.1f hp=%d/%d burn=%.1f %s t=%.2f\n", fi - 1, dmg, G.hp, G.max_hp, G.af_burn, G.player ? "player" : "autopilot", G.t);
#endif
}

/* level 2's ice (docs/PLAN-tutorial.md 7.1) */
static float ice_k(int i) {                                        /* how far a cell is laid: 0 bare, 1 ice */
    float u = clampf((IC.t - (IC.live ? IC.wait[i] : 1.2f - IC.wait[i])) / ICE_TURN, 0.0f, 1.0f);
    return IC.live ? u : 1.0f - u;
}
static int ice_at(int x, int y) {                                  /* ice to stand on here */
    return g_drawn && x >= 0 && y >= 0 && x < MAPN && y < MAPN && IC.at[y][x] && ice_k(IC.at[y][x] - 1) > 0.5f;
}
static void ice_down(void) {                                       /* the switch struck: it thaws */
    IC.live = 0; IC.t = 0.0f;
    if (IC.n) { sfx_at(SFX_HISS, (float)IC.cx[IC.n / 2] + 0.5f, (float)IC.cy[IC.n / 2] + 0.5f, 1.1f); sfx_at(SFX_SHATTER, (float)IC.cx[0] + 0.5f, (float)IC.cy[0] + 0.5f, 0.5f); }
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT ice down by %s t=%.2f\n", G.player ? "player" : "autopilot", G.t);
#endif
}
static void ice_up(void) {                                         /* the map taken: it creeps back in, and the switch is spent */
    if (!IC.n || IC.spent) return;
    IC.live = 1; IC.spent = 1; IC.t = 0.0f;
    if (IC.sw >= 0 && IC.sw < G.nmk) { Marked *m = &G.mk[IC.sw]; m->done = 1; m->done_t = 9.0f; mk_deco(m, SP_MK_RSEAL2, 1); sfx_at(SFX_SHATTER, m->x, m->y, 0.8f); }
    sfx_at(SFX_FROST, (float)IC.cx[IC.n / 2] + 0.5f, (float)IC.cy[IC.n / 2] + 0.5f, 1.2f); snd_sfx_ex(SFX_DEEPCHIME, 0.5f, 0.5f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT ice up t=%.2f\n", G.t);
#endif
}
static void ice_tick(float dt) {                                   /* every frame, whoever plays */
    if (!g_drawn || !IC.n) return;                                 /* (any drawn level's: it read level 2's alone, docs/PLAN-events.md) */
    IC.t += dt;
    if (IC.live && (IC.breath -= dt) <= 0.0f) {                    /* a cold breath, heard near it */
        IC.breath = 2.5f + 2.0f * rndf();
        int i = rnd(IC.n);
        sfx_at(SFX_FROST, (float)IC.cx[i] + 0.5f, (float)IC.cy[i] + 0.5f, 0.3f);
    }
    for (int j = 0; j < MAXMOBS; j++) {                            /* what stands on it is held in it */
        Mob *m = &G.mobs[j];
        if (m->alive && m->state == MS_WALK && !KIND_IS(m, KF_SPIRIT) && !buried(m) && ice_at((int)m->x, (int)m->y)) m->frozen = fmaxf(m->frozen, 0.3f);
    }
    if (G.dead || G.shop || tu_taken() || !ice_at((int)floorf(G.px), (int)floorf(G.py))) { IC.stack = 0.0f; return; }   /* none while led */
    if ((IC.stack -= dt) > 0.0f) return;
    IC.stack = ICE_STACK;
    if (!af_can(AF_CHILL)) return;
    if (G.af_n < 3) af_chill(1, 0);                                /* held at three on it: it never freezes a person */
    else G.af[AF_CHILL] = G.af0[AF_CHILL];
}
