/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * A drawn level read while the game runs, from the world editor's export: the same text that is pasted into drawn.c's DRAWN[]
 * (docs/PLAN-mapedit.md, part 3: the editor's PLAY). crawl_drawn_text reads it into g_dtext, which then takes the place of
 * whatever is drawn for its level, and a new run begins on that level. The names in it (DT_GOLD, MOB_SKEL, RT_CISTERN ...)
 * are looked up in a table the compiler fills, so their values are the game's own; a name not in it is refused by name. */

#define DXT_ACTS 256
#define DXT_RELICS 32
#define DXT_POOL 16384
#define DXT_LIGHTS 96                                               /* a drawing's lights (docs/PLAN-lights.md) */
static struct {
    char rows[MAPN][MAPN + 1]; const char *row[MAPN]; int nrow;
    DrawnThing thing[EV_THINGS]; int nthing;
    DrawnRoom room[MAXROOMS]; int nroom;
    DrawnSpawn spawn[EV_SPAWNS]; int nspawn;
    DrawnTorch torch[EV_TORCHES]; int ntorch;
    DrawnTrig trig[EV_TRIGS]; int ntrig;
    DrawnAct act[DXT_ACTS]; int nact;
    const char *relic[DXT_RELICS]; int nrelic;
    DrawnPick pick[EV_THINGS]; int npick;
    DrawnField field[EV_FIELDS]; int nfield;
    DrawnHaz haz[MAXSITE]; int nhaz;
    DrawnCell hzcell[DH_CELLS]; int nhzcell;
    DrawnLight light[DXT_LIGHTS]; int nlight;
    char pool[DXT_POOL]; int npool;                                 /* the strings: relics' names, lines */
} DXT;

#define DXN(x) { #x, x }
static const struct { const char *n; int v; } DXT_NAMES[] = {
    DXN(DT_GOLD), DXN(DT_KEY), DXN(DT_DEAD), DXN(DT_SGOLD), DXN(DT_MARK), DXN(DT_SWITCH), DXN(DT_GSEAL), DXN(DT_MAP),
    DXN(DT_COMPASS), DXN(DT_RELIC), DXN(DT_SOCKET), DXN(DT_LEVER), DXN(DT_PLATE), DXN(DT_SPOT),
    DXN(TG_PICKUP), DXN(TG_PLACE), DXN(TG_STRIKE), DXN(TG_ENTER), DXN(TG_USE), DXN(TG_STEP), DXN(TG_ALL),
    DXN(TT_ONCE), DXN(TT_EVERY), DXN(TT_TOGGLE), DXN(TT_TIMED),
    DXN(TA_FLICKER_ON), DXN(TA_FLICKER_OFF), DXN(TA_LIGHT_ON), DXN(TA_LIGHT_OFF), DXN(TA_FLICKER_ALL), DXN(TA_FIELD_ON),
    DXN(TA_FIELD_OFF), DXN(TA_DOOR_OPEN), DXN(TA_DOOR_UNLOCK), DXN(TA_DOOR_SHUT), DXN(TA_WAKE), DXN(TA_SECRET), DXN(TA_LINE),
    DXN(TA_SOUND), DXN(TA_HAZ_ON), DXN(TA_HAZ_OFF), DXN(DF_FIRE), DXN(DF_ICE),
    DXN(HH_CLOCK), DXN(HH_ENTER),                                  /* the hazards (section 2c): every one the editor offers, by run */
    DXN(HZ_CENSER), DXN(HZ_BREATH), DXN(HZ_COLD), DXN(HZ_ROCK), DXN(HZ_VOTIVE), DXN(HZ_RELIQ),
    DXN(HZ_SLUICE), DXN(HZ_SNARE), DXN(HZ_BILE), DXN(HZ_GALV), DXN(HZ_SILT), DXN(HZ_SAINT),
    DXN(HZ_TIMBER), DXN(HZ_DAMP), DXN(HZ_CART), DXN(HZ_LODE), DXN(HZ_FIRE), DXN(HZ_TITHE),
    DXN(HZ_FISSURE), DXN(HZ_CRYSTAL), DXN(HZ_DCOLD), DXN(HZ_VOID), DXN(HZ_ASH), DXN(HZ_GLASS), DXN(HZ_MIRROR),
    DXN(DL_CHANDELIER), DXN(DL_LANTERN), DXN(DL_CANDLESTAND), DXN(DL_BRAZIER), DXN(DL_MINELAMP), DXN(DL_CRYSTALS),   /* the lights */
    DXN(DL_FIREPIT), DXN(DL_MAGMA), DXN(DL_TOTEM), DXN(DL_CORONA), DXN(DL_SANCTUARY), DXN(DL_BONES), DXN(DL_CAGE),
    DXN(RT_HALL), DXN(RT_OSSUARY), DXN(RT_BURIAL), DXN(RT_CHAPTER), DXN(RT_KNIGHTS), DXN(RT_UNDER), DXN(RT_MERCHANT),
    DXN(RT_PLOT), DXN(RT_SAINTS), DXN(RT_CISTERN), DXN(RT_NAVE), DXN(RT_CANAL), DXN(RT_SPINNERS), DXN(RT_GALLERY),
    DXN(RT_STOPE), DXN(RT_CHARNEL), DXN(RT_FOSSIL), DXN(RT_CHAPEL), DXN(RT_GEODE), DXN(RT_GLASS), DXN(RT_CINDER),
    DXN(RT_VENT), DXN(RT_THRONE),
    DXN(MOB_SKEL), DXN(MOB_GHOST), DXN(MOB_BAT), DXN(MOB_WSKULL), DXN(MOB_SHIELD), DXN(MOB_SPIDER), DXN(MOB_SHAMBLER),
    DXN(MOB_DROWNED), DXN(MOB_HOUND), DXN(MOB_WRAITH), DXN(MOB_HUNTER), DXN(MOB_GHOUL), DXN(MOB_KNIGHT), DXN(MOB_BROOD),
    DXN(MOB_LING), DXN(MOB_SAC), DXN(MOB_SHADE), DXN(MOB_IMP), DXN(MOB_DEACON), DXN(MOB_GARGOYLE), DXN(MOB_WHIPPER),
    DXN(MOB_SEXTON), DXN(MOB_LIGHTLESS), DXN(MOB_RAT),
    DXN(RK_I), DXN(RK_II), DXN(RK_III), DXN(RK_IV), DXN(RK_V), DXN(RK_GILDED),
    DXN(IT_COINS), DXN(IT_CHALICE), DXN(IT_GEMS), DXN(IT_CROWN), DXN(IT_CHEST),
    DXN(EL_FIRE), DXN(EL_FROST), DXN(EL_LIGHT), DXN(EL_POISON), DXN(EL_SPIRIT), DXN(EL_PHYS),
    DXN(SFX_DEEPCHIME), DXN(SFX_SHIMMER), DXN(SFX_KNOCK), DXN(SFX_KEYGET), DXN(SFX_BONES), DXN(SFX_WAIL),
};

/* the text in tokens: names, numbers (1, -1, 0.50f, 0x7D1A0003u), strings, and single marks; comments and # lines passed over */
enum { XT_END, XT_ID, XT_NUM, XT_STR, XT_MARK };
typedef struct { int t; char id[64]; double v; const char *s; char c; } Xt;
typedef struct { const char *p; char *err; int nerr; int bad; int line; } Xp;
static void dxt_fail(Xp *P, const char *what, const char *name) {
    if (P->bad) return;
    P->bad = 1;
    snprintf(P->err, (size_t)P->nerr, "line %d: %s%s%s", P->line, what, name ? ": " : "", name ? name : "");
}
static void dxt_skip(Xp *P) {
    for (;;) {
        const char *p = P->p;
        if (*p == '\n') { P->line++; P->p++; }
        else if (*p == ' ' || *p == '\t' || *p == '\r') P->p++;
        else if (p[0] == '/' && p[1] == '*') {
            const char *e = strstr(p + 2, "*/");
            for (const char *q = p; q < (e ? e : p + strlen(p)); q++) if (*q == '\n') P->line++;
            P->p = e ? e + 2 : p + strlen(p);
        } else if ((p[0] == '/' && p[1] == '/') || p[0] == '#') { while (*P->p && *P->p != '\n') P->p++; }
        else return;
    }
}
static Xt dxt_next(Xp *P) {
    Xt t; memset(&t, 0, sizeof t);
    dxt_skip(P);
    const char *p = P->p;
    if (!*p || P->bad) { t.t = XT_END; return t; }
    if (isalpha((unsigned char)*p) || *p == '_') {
        int n = 0;
        while (isalnum((unsigned char)*p) || *p == '_') { if (n < 63) t.id[n++] = *p; p++; }
        t.id[n] = 0; t.t = XT_ID; P->p = p;
        return t;
    }
    if (isdigit((unsigned char)*p) || ((*p == '-' || *p == '.') && (isdigit((unsigned char)p[1]) || p[1] == '.'))) {
        char *e;
        int neg = *p == '-';
        const char *q = neg ? p + 1 : p;
        if (q[0] == '0' && (q[1] == 'x' || q[1] == 'X')) { t.v = (double)strtoull(q, &e, 16); if (neg) t.v = -t.v; }
        else t.v = strtod(p, &e);
        while (*e == 'f' || *e == 'F' || *e == 'u' || *e == 'U' || *e == 'l' || *e == 'L') e++;
        t.t = XT_NUM; P->p = e;
        return t;
    }
    if (*p == '"') {
        t.s = DXT.pool + DXT.npool; p++;
        while (*p && *p != '"' && *p != '\n') {
            char c = *p++;
            if (c == '\\' && *p) { c = *p++; c = c == 'n' ? '\n' : c == 't' ? '\t' : c; }
            if (DXT.npool >= DXT_POOL - 2) { dxt_fail(P, "too many words in the level's strings", NULL); t.t = XT_END; return t; }
            DXT.pool[DXT.npool++] = c;
        }
        if (*p != '"') { dxt_fail(P, "a string that never ends", NULL); t.t = XT_END; return t; }
        DXT.pool[DXT.npool++] = 0;
        t.t = XT_STR; P->p = p + 1;
        return t;
    }
    t.t = XT_MARK; t.c = *p; P->p = p + 1;
    return t;
}
static Xt dxt_peek(Xp *P) { Xp s = *P; int np = DXT.npool; Xt t = dxt_next(P); *P = s; DXT.npool = np; return t; }
static int dxt_mark(Xp *P, char c) {                               /* the next token is this mark: taken */
    Xt t = dxt_next(P);
    if (t.t == XT_MARK && t.c == c) return 1;
    char w[4] = { '\'', c, '\'', 0 };
    dxt_fail(P, "expected", w);
    return 0;
}
static int dxt_is_part(const char *id, const char **suffix) {     /* DRAWNn_SUFFIX: one of the level's parts */
    if (strncmp(id, "DRAWN", 5)) return 0;
    const char *p = id + 5;
    if (!isdigit((unsigned char)*p)) return 0;
    while (isdigit((unsigned char)*p)) p++;
    if (*p != '_') return 0;
    *suffix = p + 1;
    return 1;
}

/* a value: a number or a name the table knows, a string, NULL, a part named, a count ((int)(sizeof ...)), or a group { ... } */
enum { XV_NUM, XV_STR, XV_NULL, XV_PART, XV_COUNT, XV_GROUP };
typedef struct { int k; double v; const char *s; double g[4]; int ng; } Xv;
static void dxt_parens(Xp *P) {                                    /* past a ( ... ), its opening taken */
    for (int depth = 1; depth > 0 && !P->bad;) {
        Xt t = dxt_next(P);
        if (t.t == XT_END) { dxt_fail(P, "a bracket that never closes", NULL); return; }
        if (t.t == XT_MARK && t.c == '(') depth++;
        if (t.t == XT_MARK && t.c == ')') depth--;
    }
}
static Xv dxt_value1(Xp *P);
static Xv dxt_value(Xp *P) {                                       /* and two numbers or names with | between (HH_CLOCK | HH_ENTER) */
    Xv v = dxt_value1(P);
    for (;;) {
        Xt n = dxt_peek(P);
        if (P->bad || !(n.t == XT_MARK && n.c == '|')) return v;
        dxt_next(P);
        Xv w = dxt_value1(P);
        if (P->bad) return v;
        if (v.k != XV_NUM || w.k != XV_NUM) { dxt_fail(P, "a '|' between things that are not numbers", NULL); return v; }
        v.v = (double)((int64_t)v.v | (int64_t)w.v);
    }
}
static Xv dxt_value1(Xp *P) {
    Xv v; memset(&v, 0, sizeof v);
    Xt t = dxt_next(P);
    const char *suffix;
    if (t.t == XT_NUM) { v.k = XV_NUM; v.v = t.v; }
    else if (t.t == XT_STR) { v.k = XV_STR; v.s = t.s; }
    else if (t.t == XT_ID && !strcmp(t.id, "NULL")) v.k = XV_NULL;
    else if (t.t == XT_ID && dxt_is_part(t.id, &suffix)) v.k = XV_PART;
    else if (t.t == XT_ID) {
        int i, n = (int)(sizeof DXT_NAMES / sizeof DXT_NAMES[0]);
        for (i = 0; i < n && strcmp(DXT_NAMES[i].n, t.id); i++) {}
        if (i == n) dxt_fail(P, "a name this game does not know", t.id);
        else { v.k = XV_NUM; v.v = DXT_NAMES[i].v; }
    } else if (t.t == XT_MARK && t.c == '(') {
        dxt_parens(P);
        Xt n = dxt_peek(P);
        if (n.t == XT_MARK && n.c == '(') { dxt_next(P); dxt_parens(P); }
        v.k = XV_COUNT;
    } else if (t.t == XT_MARK && t.c == '{') {
        v.k = XV_GROUP;
        for (;;) {
            Xt n = dxt_peek(P);
            if (n.t == XT_MARK && n.c == '}') { dxt_next(P); break; }
            Xv e = dxt_value(P);
            if (P->bad) break;
            if (e.k != XV_NUM || v.ng >= 4) { dxt_fail(P, "a group of numbers holds something else", NULL); break; }
            v.g[v.ng++] = e.v;
            n = dxt_next(P);
            if (n.t == XT_MARK && n.c == '}') break;
            if (!(n.t == XT_MARK && n.c == ',')) { dxt_fail(P, "expected ',' or '}'", NULL); break; }
        }
    } else dxt_fail(P, "something out of place", t.t == XT_ID ? t.id : NULL);
    return v;
}
/* the values of one { ... }, its opening taken: n of them, at most max */
static int dxt_group(Xp *P, Xv *out, int max) {
    int n = 0;
    for (;;) {
        Xt t = dxt_peek(P);
        if (t.t == XT_MARK && t.c == '}') { dxt_next(P); return n; }
        Xv v = dxt_value(P);
        if (P->bad) return n;
        if (n >= max) { dxt_fail(P, "too many values in a group", NULL); return n; }
        out[n++] = v;
        t = dxt_next(P);
        if (t.t == XT_MARK && t.c == '}') return n;
        if (!(t.t == XT_MARK && t.c == ',')) { dxt_fail(P, "expected ',' or '}'", NULL); return n; }
    }
}
static double dxt_num(Xp *P, const Xv *v) {
    if (v->k == XV_NUM) return v->v;
    if (v->k == XV_NULL) return 0.0;
    dxt_fail(P, "a number was wanted", NULL);
    return 0.0;
}
#define XI(i) ((int)dxt_num(P, &v[i]))
#define XF(i) ((float)dxt_num(P, &v[i]))
#define XU(i) ((unsigned)(uint32_t)(int64_t)dxt_num(P, &v[i]))
static void dxt_item(Xp *P, const char *part, Xv *v, int n) {     /* one { ... } of a part, into its array */
    #define NEED(lo, hi) if (n < (lo) || n > (hi)) { dxt_fail(P, "the wrong number of values in", part); return; }
    #define ROOM(cnt, cap) if ((cnt) >= (cap)) { dxt_fail(P, "more than the game holds of", part); return; }
    if (!strcmp(part, "THING")) {
        NEED(4, 4) ROOM(DXT.nthing, EV_THINGS)
        DXT.thing[DXT.nthing++] = (DrawnThing){ XI(0), XI(1), XF(2), XF(3) };
    } else if (!strcmp(part, "ROOM")) {
        NEED(5, 5) ROOM(DXT.nroom, MAXROOMS)
        DXT.room[DXT.nroom++] = (DrawnRoom){ XI(0), XI(1), XI(2), XI(3), XI(4) };
    } else if (!strcmp(part, "SPAWN")) {
        NEED(7, 9) ROOM(DXT.nspawn, EV_SPAWNS)
        DXT.spawn[DXT.nspawn++] = (DrawnSpawn){ XF(0), XF(1), XI(2), XI(3), XI(4), XI(5), XI(6), n > 7 ? XI(7) : 0, n > 8 ? XI(8) : 0 };
    } else if (!strcmp(part, "TORCH")) {
        NEED(5, 5) ROOM(DXT.ntorch, EV_TORCHES)
        DXT.torch[DXT.ntorch++] = (DrawnTorch){ XI(0), XI(1), XI(2), XI(3), XI(4) };
    } else if (!strcmp(part, "TRIG")) {
        NEED(14, 14) ROOM(DXT.ntrig, EV_TRIGS)
        DXT.trig[DXT.ntrig++] = (DrawnTrig){ XI(0), XI(1), XF(2), XI(3), XI(4), XI(5), XI(6), XI(7), XI(8), XU(9), XI(10), XI(11), XI(12), XI(13) };
    } else if (!strcmp(part, "ACT")) {
        NEED(6, 6) ROOM(DXT.nact, DXT_ACTS)
        if (v[5].k != XV_STR && v[5].k != XV_NULL) { dxt_fail(P, "an action's line is not a string", NULL); return; }
        DXT.act[DXT.nact++] = (DrawnAct){ XI(0), XI(1), XI(2), XI(3), XF(4), v[5].k == XV_STR ? v[5].s : NULL };
    } else if (!strcmp(part, "PICK")) {
        NEED(3, 3) ROOM(DXT.npick, EV_THINGS)
        DXT.pick[DXT.npick++] = (DrawnPick){ XI(0), XU(1), XI(2) };
    } else if (!strcmp(part, "FIELD")) {
        NEED(7, 7) ROOM(DXT.nfield, EV_FIELDS)
        DXT.field[DXT.nfield++] = (DrawnField){ XI(0), XI(1), XI(2), XI(3), XI(4), XI(5), XI(6) };
    } else if (!strcmp(part, "HAZ")) {
        NEED(19, 19) ROOM(DXT.nhaz, MAXSITE)
        DXT.haz[DXT.nhaz++] = (DrawnHaz){ XI(0), XI(1), XI(2), XI(3), XI(4), XI(5), XI(6), XF(7), XF(8), XF(9), XI(10), XI(11), XI(12), XI(13),
                                          XI(14), XI(15), XI(16), XI(17), XF(18) };
    } else if (!strcmp(part, "HZCELL")) {
        NEED(2, 2) ROOM(DXT.nhzcell, DH_CELLS)
        DXT.hzcell[DXT.nhzcell++] = (DrawnCell){ XI(0), XI(1) };
    } else if (!strcmp(part, "LIGHT")) {
        NEED(3, 3) ROOM(DXT.nlight, DXT_LIGHTS)
        DXT.light[DXT.nlight++] = (DrawnLight){ XI(0), XI(1), XI(2) };
    } else dxt_fail(P, "a part this game does not read", part);
    #undef NEED
    #undef ROOM
}
static void dxt_string(Xp *P, const char *part, const char *s) {  /* one string of a part: a row of the map, a relic's name */
    if (!strcmp(part, "MAP")) {
        if (DXT.nrow >= MAPN) { dxt_fail(P, "the map has more rows than the level holds", NULL); return; }
        if (strlen(s) > MAPN) { dxt_fail(P, "a row of the map is wider than the level", NULL); return; }
        strcpy(DXT.rows[DXT.nrow], s); DXT.row[DXT.nrow] = DXT.rows[DXT.nrow]; DXT.nrow++;
    } else if (!strcmp(part, "RELIC")) {
        if (DXT.nrelic >= DXT_RELICS) { dxt_fail(P, "more relics than the game holds", NULL); return; }
        DXT.relic[DXT.nrelic++] = s;
    } else dxt_fail(P, "a string where none belongs, in", part);
}
static void dxt_part(Xp *P, const char *part) {                    /* DRAWNn_PART[...] = { ... };, its name taken */
    char name[16];
    snprintf(name, sizeof name, "%s", part);
    for (Xt t = dxt_next(P); !(t.t == XT_MARK && t.c == ']'); t = dxt_next(P))
        if (t.t == XT_END) { dxt_fail(P, "a '[' that never closes", NULL); return; }
    if (!dxt_mark(P, '=') || !dxt_mark(P, '{')) return;
    for (;;) {
        Xt t = dxt_next(P);
        if (t.t == XT_MARK && t.c == '}') return;
        if (t.t == XT_STR) dxt_string(P, name, t.s);
        else if (t.t == XT_MARK && t.c == '{') { Xv v[24]; int n = dxt_group(P, v, 24); if (!P->bad) dxt_item(P, name, v, n); }
        else { dxt_fail(P, "something out of place in", name); return; }
        if (P->bad) return;
        t = dxt_next(P);
        if (t.t == XT_MARK && t.c == '}') return;
        if (!(t.t == XT_MARK && t.c == ',')) { dxt_fail(P, "expected ',' or '}' in", name); return; }
    }
}

int crawl_drawn_text(const char *text, char *err, int nerr);       /* 0: read, and in place of its level's drawing; else why not, in err */
int crawl_drawn_text(const char *text, char *err, int nerr) {
    static char none[2];
    if (!err || nerr < 1) { err = none; nerr = (int)sizeof none; }
    err[0] = 0;
    g_dtext_on = 0;
    memset(&DXT, 0, sizeof DXT);
    Xp PS = { text ? text : "", err, nerr, 0, 1 }, *P = &PS;
    Xv v[40]; int nrow = -1;                                       /* v: the row in DRAWN[] */
    for (;;) {
        Xt t = dxt_next(P);
        if (P->bad || t.t == XT_END) break;
        const char *suffix;
        if (t.t == XT_ID && dxt_is_part(t.id, &suffix)) {
            Xt n = dxt_peek(P);
            if (n.t == XT_MARK && n.c == '[') dxt_part(P, suffix);
        } else if (t.t == XT_MARK && t.c == '{') {
            if (nrow >= 0) { dxt_fail(P, "two rows for DRAWN[]: paste one level at a time", NULL); break; }
            nrow = dxt_group(P, v, 40);
        }
    }
    if (P->bad) return 1;
    #define BAD(why) do { dxt_fail(P, why, NULL); return 1; } while (0)
    if (!DXT.nrow) BAD("no map (DRAWNn_MAP)");
    if (!DXT.nroom) BAD("no rooms (DRAWNn_ROOM)");
    if (nrow < 13) BAD("no row for DRAWN[], or a short one: it is the line after \"its row in DRAWN[]\"");
    int w = (int)strlen(DXT.row[0]), h = DXT.nrow;
    for (int y = 0; y < h; y++) {
        if ((int)strlen(DXT.row[y]) != w) BAD("the map's rows are not all as wide as the first");
        for (const char *c = DXT.row[y]; *c; c++) if (!strchr("#.+FfDGC=BShXIicqQ", *c)) BAD("a letter in the map this game does not read");
    }
    {                                                              /* the row's own numbers */
      int rw = XI(1), rh = XI(2), ox = XI(3), oy = XI(4), face = XI(5);
      if (v[8].k != XV_GROUP || v[8].ng != NKEYS) BAD("the row's vaults are not three numbers");
      if (rw != w || rh != h) BAD("the row's width and height are not the map's");
      if (ox < 0 || oy < 0 || ox + w > MAPN || oy + h > MAPN) BAD("the map does not fit in the level where the row lays it");
      g_dtext = (Drawn){ 0 };
      g_dtext.map = DXT.row; g_dtext.w = w; g_dtext.h = h; g_dtext.ox = ox; g_dtext.oy = oy; g_dtext.face = face & 3;
      g_dtext.room = DXT.room; g_dtext.nroom = DXT.nroom;
      for (int k = 0; k < NKEYS; k++) g_dtext.vault[k] = (int)v[8].g[k];
      g_dtext.thing = DXT.thing; g_dtext.nthing = DXT.nthing;
      g_dtext.seed = (uint32_t)(int64_t)dxt_num(P, &v[11]);
      g_dtext.level = XI(12);
      if (P->bad) return 1;
    }
    if (g_dtext.level < 1 || g_dtext.level > 99) BAD("the level a drawing is for must be 1 to 99");
    if (DXT.nspawn) { g_dtext.spawn = DXT.spawn; g_dtext.nspawn = DXT.nspawn; }
    if (DXT.ntorch) { g_dtext.torch = DXT.torch; g_dtext.ntorch = DXT.ntorch; }
    if (DXT.ntrig) { g_dtext.trig = DXT.trig; g_dtext.ntrig = DXT.ntrig; }
    if (DXT.nact) { g_dtext.act = DXT.act; g_dtext.nact = DXT.nact; }
    if (DXT.nrelic) { g_dtext.relic = DXT.relic; g_dtext.nrelic = DXT.nrelic; }
    if (DXT.npick) { g_dtext.pick = DXT.pick; g_dtext.npick = DXT.npick; }
    if (DXT.nfield) { g_dtext.field = DXT.field; g_dtext.nfield = DXT.nfield; }
    if (DXT.nhaz) { g_dtext.haz = DXT.haz; g_dtext.nhaz = DXT.nhaz; }
    if (DXT.nhzcell) { g_dtext.hzcell = DXT.hzcell; g_dtext.nhzcell = DXT.nhzcell; }
    if (nrow > 27) {                                               /* the row's hazards, as many as were read */
        if (nrow != 31 && nrow != 33) BAD(nrow > 31 ? "the row's lights are not two values (the lights, how many)" : "the row's hazards are not four values (the hazards, how many, their cells, how many)");
        if ((v[27].k != XV_PART && v[27].k != XV_NULL) || (v[29].k != XV_PART && v[29].k != XV_NULL)) BAD("the row's hazards are not named parts");
        if (XI(28) != DXT.nhaz || XI(30) != DXT.nhzcell) BAD("the row's count of hazards or of their cells is not what was read");
        if (P->bad) return 1;
    }
    if (DXT.nhaz && !DXT.nhzcell) BAD("hazards with no cells (DRAWNn_HZCELL)");
    if (nrow > 31) {                                               /* the row's lights, after the hazards (docs/PLAN-lights.md) */
        if (v[31].k != XV_PART && v[31].k != XV_NULL) BAD("the row's lights are not a named part");
        if (XI(32) != DXT.nlight) BAD("the row's count of lights is not what was read");
        if (P->bad) return 1;
    } else if (DXT.nlight) BAD("lights (DRAWNn_LIGHT) the row does not name: it ends with them, after the hazards");
    if (DXT.nlight) { g_dtext.light = DXT.light; g_dtext.nlight = DXT.nlight; }
    /* what the compiled drawings are trusted with, checked: every index in range */
    for (int i = 0; i < DXT.nroom; i++) { const DrawnRoom *r = &DXT.room[i];
        if (r->x < 0 || r->y < 0 || r->w < 1 || r->h < 1 || r->x + r->w > w || r->y + r->h > h || r->rtype < 0 || r->rtype >= NRT) BAD("a room outside the map, or of no type"); }
    for (int k = 0; k < NKEYS; k++) if (g_dtext.vault[k] < -1 || g_dtext.vault[k] >= DXT.nroom) BAD("a vault that is no room");
    for (int i = 0; i < DXT.nthing; i++) { const DrawnThing *t = &DXT.thing[i];
        if (t->what < DT_GOLD || t->what > DT_SPOT || t->x < 0.0f || t->y < 0.0f || t->x >= (float)w || t->y >= (float)h) BAD("a thing outside the map, or of no kind");
        if ((t->what == DT_DEAD && (t->kind < 0 || t->kind >= NKINDS)) || ((t->what == DT_GOLD || t->what == DT_SGOLD) && (t->kind < IT_COINS || t->kind > IT_CHEST))
            || (t->what == DT_KEY && (t->kind < 0 || t->kind >= NKEYS)) || (t->what == DT_RELIC && (t->kind < 0 || t->kind >= DXT.nrelic))
            || (t->what == DT_SOCKET && (t->kind < -1 || t->kind >= DXT.nrelic)) || (t->what == DT_MARK && (t->kind < 0 || t->kind > 2))) BAD("a thing of a kind this game does not have"); }
    for (int i = 0; i < DXT.nspawn; i++) { const DrawnSpawn *s = &DXT.spawn[i];
        if (s->kind < -1 || s->kind >= NKINDS || s->rank < -1 || s->rank > RK_GILDED || s->elem < -1 || s->elem >= NELEM || s->wake < 0 || s->spot < 0
            || (s->spot ? !(fabsf(s->x) <= (float)MAPN && fabsf(s->y) <= (float)MAPN)   /* (one laid at a spot is written off the map) */
                        : !(s->x >= 0.0f && s->y >= 0.0f && s->x < (float)w && s->y < (float)h))) BAD("a spawn point out of range"); }
    for (int i = 0; i < DXT.ntorch; i++) if (DXT.torch[i].d < 0 || DXT.torch[i].d > 3 || DXT.torch[i].group < 0) BAD("a torch out of range");
    for (int i = 0; i < DXT.ntrig; i++) { const DrawnTrig *t = &DXT.trig[i];
        if (t->kind < TG_PICKUP || t->kind > TG_ALL || t->type < TT_ONCE || t->type > TT_TIMED || t->thing < -1 || t->thing >= DXT.nthing
            || t->act < 0 || t->nact < 0 || t->act + t->nact > DXT.nact || t->back < 0 || t->nback < 0 || t->back + t->nback > DXT.nact) BAD("a trigger out of range"); }
    for (int i = 0; i < DXT.nact; i++) if (DXT.act[i].kind < TA_FLICKER_ON || DXT.act[i].kind > TA_HAZ_OFF) BAD("an action of a kind this game does not have");
    for (int i = 0; i < DXT.nact; i++) if ((DXT.act[i].kind == TA_HAZ_ON || DXT.act[i].kind == TA_HAZ_OFF) && (DXT.act[i].arg < 0 || DXT.act[i].arg >= DXT.nhaz)) BAD("an action on a hazard that is not there");
    for (int i = 0; i < DXT.npick; i++) if (DXT.pick[i].thing < 0 || DXT.pick[i].thing >= DXT.nthing) BAD("a pick of a thing that is not there");
    for (int i = 0; i < DXT.nfield; i++) if ((DXT.field[i].kind != DF_FIRE && DXT.field[i].kind != DF_ICE) || DXT.field[i].x < 0 || DXT.field[i].x >= w || DXT.field[i].y < 0 || DXT.field[i].y >= h) BAD("a field out of range");
    for (int i = 0; i < DXT.nhaz; i++) { const DrawnHaz *z = &DXT.haz[i];   /* the hazards: a kind the editor offers; the anchor in the */
        if (!dh_kind_ok(z->kind)) BAD("a hazard of a kind this game does not lay");   /* drawing or its ring (a wall set in the edge) */
        if (z->ax < -1 || z->ay < -1 || z->ax > w || z->ay > h || g_dtext.ox + z->ax < 0 || g_dtext.oy + z->ay < 0 || g_dtext.ox + z->ax >= MAPN || g_dtext.oy + z->ay >= MAPN) BAD("a hazard's anchor outside the map");
        if (z->cell < 0 || z->ncell < 0 || z->cell + z->ncell > DXT.nhzcell) BAD("a hazard's cells are not in DRAWNn_HZCELL");
        if ((z->zone != 0 && z->zone != 1) || z->how < 0 || z->how > (HH_CLOCK | HH_ENTER) || (z->on != 0 && z->on != 1) || z->kills < -1 || z->kills > 1
            || z->harm < -1 || z->harm > 100 || z->chance < -1 || z->chance > 100 || !(z->every >= 0.0f) || !(z->give >= 0.0f) || !(z->first >= 0.0f)
            || !(z->warn >= -1.0f) || z->every > 3600.0f || z->give > 3600.0f || z->first > 3600.0f || z->warn > 60.0f) BAD("a hazard's settings out of range"); }
    for (int i = 0; i < DXT.nlight; i++) { const DrawnLight *L = &DXT.light[i];
        if (L->style < 0 || L->style >= NDL) BAD("a light of a style this game does not have");
        if (L->x < 0 || L->y < 0 || L->x >= w || L->y >= h) BAD("a light outside the map"); }
    for (int i = 0; i < DXT.nhzcell; i++) if (DXT.hzcell[i].x < 0 || DXT.hzcell[i].y < 0 || DXT.hzcell[i].x >= w || DXT.hzcell[i].y >= h) BAD("a hazard's cell outside the map");
    #undef BAD
    g_dtext_on = 1;
    return 0;
}
#undef XI
#undef XF
#undef XU

/* The generator's lights on the drawing read (docs/PLAN-lights.md, section 1): the drawing laid as the game lays it for its level, on
 * its own seed with its own dressing (but none of its own lights, so that what the generator would hang and stand is what comes back),
 * and the light props it hung or stood that keep their picture, as DL_* and the drawing's cells, in the order laid. Nothing of a game
 * in play is disturbed: everything the laying writes (the level, its plan, lanes, fire, ice, events, hazards, the faces' and the
 * dressing's own counts) is put back as it was, byte for byte, so it may be asked at any time, and of a game never begun (the editor
 * loads the game in its own page only to ask this). Things a pick lays by the run's dice are taken where drawn. How many, or -1: no
 * drawing read. */
int crawl_drawn_lights(DrawnLight *out, int max);
int crawl_drawn_lights(DrawnLight *out, int max) {
    if (!g_dtext_on) return -1;
    #define KEEP_ALL(X) X(G) X(MM) X(nr_type) X(mroom) X(g_lane) X(g_ldist) X(DF) X(IC) X(EV) X(DH) X(GLP) X(room_shrine) X(wall_lights) \
                        X(g_gate) X(g_drawn) X(g_events) X(uf) X(g_foot) X(g_foot_n) X(glp_on)
    #define KEEP_SIZE(v) + sizeof v
    size_t need = 0 KEEP_ALL(KEEP_SIZE);
    unsigned char *keep = (unsigned char *)malloc(need), *k = keep;
    if (!keep) return -1;
    #define KEEP_OUT(v) memcpy(k, &v, sizeof v); k += sizeof v;
    #define KEEP_IN(v) memcpy(&v, k, sizeof v); k += sizeof v;
    KEEP_ALL(KEEP_OUT)
    memset(&G, 0, sizeof G);                                       /* a fresh level, as start_level begins one */
    memset(&EV, 0, sizeof EV); memset(&DH, 0, sizeof DH);
    int nlight = g_dtext.nlight;
    g_dtext.nlight = 0;                                            /* the generator's, not the drawing's own */
    g_drawn = NDRAWN + 1;
    const Drawn *D = &g_dtext;
    G.level = D->level; G.boss = boss_of_level(G.level);
    for (int i = 0; i < D->nthing && i < EV_THINGS; i++) { EV.tx[i] = D->thing[i].x; EV.ty[i] = D->thing[i].y; }
    for (int i = 0; i < D->nspawn && i < EV_SPAWNS; i++) { EV.sx[i] = D->spawn[i].x; EV.sy[i] = D->spawn[i].y; }
    g_events = D->ntrig || D->ntorch || D->nfield;
    G.rs = D->seed;
    gen_drawn();
    int n = 0;
    for (int i = 0; i < GLP.n; i++) {
        int d = GLP.p[i].deco;
        if (d < 0 || d >= G.ndecos || G.decos[d].spr < 0) continue;  /* cleared off a lane or a thing's cell: its picture is gone */
        if (n < max && out) out[n] = (DrawnLight){ GLP.p[i].style, GLP.p[i].x - D->ox, GLP.p[i].y - D->oy };
        n++;
    }
    g_dtext.nlight = nlight;
    k = keep;
    KEEP_ALL(KEEP_IN)
    free(keep);
    #undef KEEP_ALL
    #undef KEEP_SIZE
    #undef KEEP_OUT
    #undef KEEP_IN
    return n < max ? n : max;
}

#ifdef CRAWL_DEBUG
/* the drawing read against DRAWN[n - 1], field for field (build/host_debug drawncheck): 0 the same, else the first difference */
int crawl_drawn_same(int n, char *why, int nwhy);
int crawl_drawn_same(int n, char *why, int nwhy) {
    #define DIFF(what) do { snprintf(why, (size_t)nwhy, "%s", what); return 1; } while (0)
    if (!g_dtext_on) DIFF("nothing read");
    if (n < 1 || n > NDRAWN) DIFF("no such drawing");
    const Drawn *a = &g_dtext, *b = &DRAWN[n - 1];
    if (a->w != b->w || a->h != b->h || a->ox != b->ox || a->oy != b->oy || a->face != b->face) DIFF("the size, the place or the face");
    for (int y = 0; y < a->h; y++) if (strcmp(a->map[y], b->map[y])) DIFF("a row of the map");
    if (a->nroom != b->nroom || memcmp(a->room, b->room, sizeof *a->room * (size_t)a->nroom)) DIFF("the rooms");
    if (memcmp(a->vault, b->vault, sizeof a->vault)) DIFF("the vaults");
    if (a->nthing != b->nthing || memcmp(a->thing, b->thing, sizeof *a->thing * (size_t)a->nthing)) DIFF("the things");
    if (a->seed != b->seed || a->level != b->level) DIFF("the seed or the level");
    if (a->nspawn != b->nspawn || (a->nspawn && memcmp(a->spawn, b->spawn, sizeof *a->spawn * (size_t)a->nspawn))) DIFF("the spawn points");
    if (a->ntorch != b->ntorch || (a->ntorch && memcmp(a->torch, b->torch, sizeof *a->torch * (size_t)a->ntorch))) DIFF("the torches");
    if (a->ntrig != b->ntrig || (a->ntrig && memcmp(a->trig, b->trig, sizeof *a->trig * (size_t)a->ntrig))) DIFF("the triggers");
    if (a->nact != b->nact) DIFF("the number of actions");
    for (int i = 0; i < a->nact; i++) {
        const DrawnAct *x = &a->act[i], *y = &b->act[i];
        if (x->kind != y->kind || x->arg != y->arg || x->x != y->x || x->y != y->y || x->after != y->after || !x->text != !y->text || (x->text && strcmp(x->text, y->text))) DIFF("an action");
    }
    if (a->nrelic != b->nrelic) DIFF("the number of relics");
    for (int i = 0; i < a->nrelic; i++) if (strcmp(a->relic[i], b->relic[i])) DIFF("a relic's name");
    if (a->npick != b->npick || (a->npick && memcmp(a->pick, b->pick, sizeof *a->pick * (size_t)a->npick))) DIFF("the picks");
    if (a->nfield != b->nfield || (a->nfield && memcmp(a->field, b->field, sizeof *a->field * (size_t)a->nfield))) DIFF("the fields");
    if (a->nhaz != b->nhaz || (a->nhaz && memcmp(a->haz, b->haz, sizeof *a->haz * (size_t)a->nhaz))) DIFF("the hazards");
    if (a->nhzcell != b->nhzcell || (a->nhzcell && memcmp(a->hzcell, b->hzcell, sizeof *a->hzcell * (size_t)a->nhzcell))) DIFF("the hazards' cells");
    if (a->nlight != b->nlight || (a->nlight && memcmp(a->light, b->light, sizeof *a->light * (size_t)a->nlight))) DIFF("the lights");
    #undef DIFF
    why[0] = 0;
    return 0;
}
#endif
