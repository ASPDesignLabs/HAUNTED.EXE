/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The console. */

/* ---- the console (~) ----
 * A plate that drops from the top while you play, as in Quake: a greeting, the date and the hour, and a
 * line to type in. The crypt waits while it is down and its sounds with it; the music plays on. The codes
 * all begin CATA. They are kept here only as hashes (FNV-1a over the word, in capitals), and the list the
 * last of them prints is kept scrambled, so the words can't be read off the source. */
enum { ARG_NONE, ARG_NUM, ARG_OPTNUM, ARG_NAME };
enum { CH_LEVEL, CH_SEED, CH_GEAR, CH_BAG, CH_ABIL, CH_GOLD, CH_SOULS, CH_TPEXIT, CH_TPSECRET, CH_TPALTAR,
       CH_WISP, CH_FOLLOW, CH_RANK, CH_WRAITH, CH_MEET, CH_HELP };
typedef struct { uint32_t hash; uint8_t len, arg, what; } ConCode;
static const ConCode CON_CODES[] = {
    { 0x22375510u,  8, ARG_NUM, CH_LEVEL },
    { 0xFA1F68B3u,  8, ARG_NUM, CH_SEED },
    { 0xA949BC21u,  8, ARG_NUM, CH_GEAR },
    { 0x12387D8Cu,  8, ARG_NUM, CH_BAG },
    { 0x477E6818u,  8, ARG_NUM, CH_ABIL },
    { 0x195BAFB8u,  9, ARG_NONE, CH_GOLD },
    { 0x1653EAEBu, 11, ARG_NONE, CH_SOULS },
    { 0x52F797C1u, 10, ARG_NONE, CH_TPEXIT },
    { 0x914EE69Cu,  9, ARG_OPTNUM, CH_TPSECRET },
    { 0xDB447926u,  9, ARG_NONE, CH_TPALTAR },
    { 0x4A355609u,  8, ARG_NONE, CH_WISP },
    { 0xB6CAFEECu, 11, ARG_NONE, CH_FOLLOW },
    { 0x73316BC4u,  8, ARG_NUM, CH_RANK },
    { 0xB8075035u, 10, ARG_NONE, CH_WRAITH },
    { 0x45627696u,  8, ARG_NAME, CH_MEET },
    { 0xE53746F5u,  8, ARG_NONE, CH_HELP },
};
static const uint8_t CON_HELP[490] = {
    0x19, 0x4C, 0xB8, 0x66, 0xEA, 0x74, 0x65, 0xFB, 0xE2, 0x5B, 0x34, 0xCF, 0xB6, 0xFD, 0x87, 0xA4, 0x64, 0x3D, 0x68, 0xF8,
    0x5E, 0x0D, 0x35, 0x6D, 0xD7, 0x69, 0x44, 0x31, 0x6C, 0x8A, 0x59, 0xD7, 0xBB, 0x7E, 0xC9, 0x02, 0x0A, 0x71, 0x8E, 0xEB,
    0x42, 0x15, 0xE0, 0x47, 0x7F, 0x8A, 0x48, 0x5F, 0x8F, 0x6B, 0x99, 0x9B, 0x3E, 0x27, 0x42, 0x14, 0x7F, 0x65, 0x57, 0xDA,
    0x43, 0xAD, 0x98, 0xED, 0x90, 0x0E, 0x6D, 0x33, 0xAF, 0x25, 0x2F, 0xA6, 0x47, 0x75, 0x1A, 0x0F, 0xF6, 0xD9, 0x4D, 0x65,
    0x2F, 0x0F, 0x25, 0xD7, 0xED, 0xD1, 0xF5, 0x37, 0x9E, 0x45, 0xE5, 0xEB, 0x86, 0x5D, 0x11, 0x86, 0x68, 0x4D, 0x82, 0x8D,
    0xCD, 0xD0, 0x54, 0x4A, 0xF1, 0x34, 0xB7, 0x04, 0x56, 0x57, 0x88, 0x73, 0x2A, 0x3C, 0x3C, 0x55, 0x1F, 0xE6, 0x7C, 0xBB,
    0x23, 0xD0, 0x05, 0x93, 0x0F, 0x7D, 0xA1, 0xC3, 0xEB, 0xAD, 0x38, 0xE8, 0x0E, 0x84, 0xAA, 0x68, 0x03, 0xC1, 0xD5, 0x28,
    0x5F, 0x7F, 0x1C, 0x53, 0xE4, 0xBD, 0x9C, 0x17, 0xBF, 0x83, 0xB9, 0xF7, 0x5B, 0xF1, 0xBD, 0xDF, 0xD7, 0x64, 0xAF, 0x23,
    0x35, 0xE3, 0x2C, 0x86, 0x82, 0x9D, 0x60, 0x1C, 0xAD, 0xE7, 0x7A, 0x85, 0xF5, 0x18, 0xBC, 0xD2, 0x02, 0xF2, 0x1D, 0x05,
    0xDA, 0xC1, 0xB0, 0xFB, 0x92, 0xF4, 0xB4, 0x33, 0xB6, 0x59, 0x08, 0x03, 0x5D, 0x82, 0xE0, 0xA3, 0x64, 0xB2, 0xA1, 0x3F,
    0xC3, 0x9D, 0x95, 0xFD, 0x00, 0x3C, 0xDB, 0xE7, 0xCA, 0xFD, 0xCD, 0x47, 0x0E, 0x21, 0x63, 0xB4, 0x07, 0xA9, 0x77, 0x35,
    0x65, 0xC8, 0x8C, 0x02, 0xFF, 0xAA, 0x1E, 0x42, 0x5D, 0x42, 0xA0, 0xAB, 0x02, 0xA1, 0x3B, 0xEF, 0xA2, 0xD1, 0x6D, 0xF3,
    0xCF, 0xA5, 0xD5, 0xC3, 0xD4, 0x62, 0x91, 0x4F, 0xB3, 0x46, 0x96, 0x1E, 0x85, 0xE2, 0x58, 0x2D, 0x7A, 0x2D, 0xB8, 0x68,
    0x8E, 0x70, 0x00, 0xF8, 0x87, 0x56, 0x46, 0xAA, 0xC2, 0x99, 0xE0, 0xBD, 0x0A, 0x2C, 0x11, 0x84, 0x57, 0x4B, 0x33, 0x7A,
    0xC6, 0x64, 0x25, 0x33, 0x32, 0x88, 0x4A, 0xA3, 0xDA, 0x0D, 0xAC, 0x13, 0x01, 0x71, 0x94, 0x83, 0x27, 0x15, 0xF6, 0x43,
    0x77, 0x9A, 0x23, 0x33, 0x8B, 0x71, 0x88, 0x96, 0x4C, 0x6B, 0x53, 0x1A, 0x66, 0x04, 0x53, 0xD6, 0x55, 0xB9, 0x98, 0x83,
    0xBA, 0x6D, 0x0C, 0x26, 0xA0, 0x51, 0x25, 0xB3, 0x4B, 0x01, 0x74, 0x78, 0x9F, 0xAA, 0x58, 0x1F, 0x4A, 0x1B, 0x2E, 0xB2,
    0xFB, 0x8B, 0xF3, 0x3A, 0x86, 0x24, 0xF4, 0xF6, 0xEA, 0x4E, 0x0A, 0x8A, 0x77, 0x4D, 0xEC, 0xC1, 0xC1, 0xDD, 0x4C, 0x44,
    0xF5, 0x55, 0xA0, 0x07, 0x33, 0x39, 0xFF, 0x1A, 0x59, 0x2D, 0x16, 0x54, 0x1F, 0xF5, 0x11, 0xC9, 0x33, 0xCB, 0x0F, 0xFF,
    0x08, 0x09, 0xD8, 0xC3, 0x88, 0xCC, 0x22, 0xEC, 0x0E, 0x80, 0x8D, 0x1E, 0x6E, 0xB5, 0xA2, 0x4F, 0x51, 0x70, 0x04, 0x37,
    0xEF, 0xD9, 0xB6, 0x74, 0xBF, 0x95, 0xB1, 0xEC, 0x40, 0xE4, 0xAD, 0xAB, 0xAE, 0x69, 0xB8, 0x23, 0x2E, 0xE5, 0x49, 0xE7,
    0x99, 0x83, 0x01, 0x02, 0xB6, 0xFD, 0x18, 0xAF, 0xF8, 0x16, 0xBF, 0x99, 0x09, 0xFC, 0x08, 0x16, 0xDD, 0xA0, 0xDC, 0x97,
    0x92, 0x9D, 0xA4, 0x3F, 0xA6, 0x28, 0x18, 0x60, 0x48, 0x88, 0xED, 0xB3, 0x3B, 0xA3, 0xA5, 0x4B, 0xC0, 0x8C, 0xF4, 0xE6,
    0x02, 0x2A, 0xA8, 0xFD, 0xAB, 0x90, 0xB9, 0x7D, 0x7D, 0x40, 0x64, 0xBA, 0x1A, 0xA0, 0x68, 0x6F, 0x06, 0xA9, 0xF8, 0x63,
    0x9A, 0xB9, 0x04, 0x4E, 0x5D, 0x31, 0xCC, 0xC2, 0x71, 0xA1,
};

static uint32_t fnv1a(const char *s, int n) {
    uint32_t h = 0x811C9DC5u;
    for (int i = 0; i < n; i++) { h ^= (uint8_t)s[i]; h *= 0x01000193u; }
    return h;
}

/* a line into the console, wrapped at word ends to fit */
static void con_print(const char *msg, int ink) {
    for (const char *p = msg; *p; ) {
        int n = (int)strlen(p);
        if (n > 37) { n = 37; while (n > 0 && p[n] != ' ') n--; if (n == 0) n = 37; }
        if (CON.n == CON_LINES) { memmove(CON.line, CON.line + 1, sizeof CON.line[0] * (CON_LINES - 1)); memmove(CON.ink, CON.ink + 1, CON_LINES - 1); CON.n--; }
        memcpy(CON.line[CON.n], p, (size_t)n); CON.line[CON.n][n] = 0; CON.ink[CON.n++] = (uint8_t)ink;
#ifdef CRAWL_DEBUG
        fprintf(stderr, "CONSOLE %s\n", CON.line[CON.n - 1]);
#endif
        p += n; while (*p == ' ') p++;
    }
}
static void con_printf(int ink, const char *fmt, ...) {
    char buf[96];
    va_list ap; va_start(ap, fmt); vsnprintf(buf, sizeof buf, fmt, ap); va_end(ap);
    con_print(buf, ink);
}

/* the day and the hour as the crypt would say them: FRIDAY 23 OCTOBER 2026, A QUARTER PAST TEN AT NIGHT */
static void con_when(char *out, int cap) {
    static const char *const DAY[7] = { "SUNDAY", "MONDAY", "TUESDAY", "WEDNESDAY", "THURSDAY", "FRIDAY", "SATURDAY" };
    static const char *const MONTH[12] = { "JANUARY", "FEBRUARY", "MARCH", "APRIL", "MAY", "JUNE", "JULY", "AUGUST",
                                           "SEPTEMBER", "OCTOBER", "NOVEMBER", "DECEMBER" };
    static const char *const HOUR[12] = { "TWELVE", "ONE", "TWO", "THREE", "FOUR", "FIVE", "SIX", "SEVEN", "EIGHT", "NINE", "TEN", "ELEVEN" };
    static const char *const MIN[12] = { "", "FIVE PAST ", "TEN PAST ", "A QUARTER PAST ", "TWENTY PAST ", "TWENTY-FIVE PAST ", "HALF PAST ",
                                         "TWENTY-FIVE TO ", "TWENTY TO ", "A QUARTER TO ", "TEN TO ", "FIVE TO " };
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    if (!tm) { snprintf(out, (size_t)cap, "THE HOUR IS LOST DOWN HERE"); return; }
    int m5 = (tm->tm_min + 2) / 5, h = tm->tm_hour;
    if (m5 == 12) { m5 = 0; h = (h + 1) % 24; }
    int named = (h + (m5 > 6)) % 24;                               /* "to" names the next hour */
    const char *part = named >= 5 && named < 12 ? " IN THE MORNING" : (named >= 12 && named < 17 ? " IN THE AFTERNOON"
                     : (named >= 17 && named < 21 ? " IN THE EVENING" : " AT NIGHT"));
    char hour[48];
    if (m5 == 0 && (named == 0 || named == 12)) snprintf(hour, sizeof hour, "%s", named ? "NOON" : "MIDNIGHT");
    else if (m5 == 0) snprintf(hour, sizeof hour, "%s O'CLOCK%s", HOUR[named % 12], part);
    else snprintf(hour, sizeof hour, "%s%s%s", MIN[m5], HOUR[named % 12], part);
    snprintf(out, (size_t)cap, "%s %d %s %d, %s", DAY[tm->tm_wday % 7], tm->tm_mday, MONTH[tm->tm_mon % 12], tm->tm_year + 1900, hour);
}

static void con_open(void) {
    static const char *const HELLO[] = { "THE CRYPT HEARS YOU TYPING.", "SPEAK, CASTER. THE DEAD ARE LISTENING.",
        "A DRAUGHT FROM BELOW. SOMEONE LEFT THIS OPEN.", "THE BONES WRITE DOWN EVERY WORD.",
        "WELCOME BACK. THE TORCHES HAD BEGUN TO WORRY.", "MIND THE DUST. NOBODY HAS TYPED HERE IN YEARS." };
    char when[96], ver[48];
    CON.open = 1; CON.scroll = 0;
    snd_hold(1);                                                   /* the crypt's sounds wait with it */
    if (CON.n) con_print("", R_STONE + 6);
    con_print(HELLO[CON.opens++ % (int)(sizeof HELLO / sizeof HELLO[0])], R_BONE + 13);
    con_when(when, sizeof when);
    con_print(when, R_STONE + 12);
    snprintf(ver, sizeof ver, "HAUNTED.EXE %s, CATACOMB", HVS_VERSION);
    for (char *c = ver; *c; c++) if (*c >= 'a' && *c <= 'z') *c = (char)(*c - 32);
    con_print(ver, R_GOLD + 11);
}
static void con_close(void) { CON.open = 0; snd_hold(0); }

/* the list the help code prints: unscrambled into lines as it goes */
static void con_help(void) {
    char line[48];
    int n = 0, k = 0x5A, lines = 1;
    con_print("THE CODES. PGUP, PGDN OR THE WHEEL.", R_GOLD + 12);
    for (int i = 0; i <= (int)sizeof CON_HELP; i++) {
        char c = i < (int)sizeof CON_HELP ? (char)(CON_HELP[i] ^ k) : '\n';
        k = (k * 37 + 11) & 0xFF;
        if (c == '\n' || n == 46) { line[n] = 0; con_print(line, R_BONE + 12); lines++; n = 0; }
        else line[n++] = c;
    }
    CON.scroll = lines > CON_ROWS ? lines - CON_ROWS : 0;           /* the list from its top */
}

/* a creature by its name in the bestiary, letters only: CALL then GHOUL, SKELETONKNIGHT ... */
static int con_kind(const char *name) {
    for (int k = 0; k < NKINDS; k++)
        for (int w = 0; w < 2; w++) {
            const char *a = w ? KINDS[k].plural : KINDS[k].name, *b = name;
            if (!a || !*a) continue;
            for (; *a && *b; a++) { if ((*a >= 'A' && *a <= 'Z') || (*a >= '0' && *a <= '9')) { if (*a != *b) break; b++; } }
            while (*a && !((*a >= 'A' && *a <= 'Z') || (*a >= '0' && *a <= '9'))) a++;
            if (!*a && !*b) return k;
        }
    return -1;
}

/* what a code does; 1 if it did it */
static int con_do(int what, int num, const char *rest) {
    static const char *const RANK_SAY[NRANK] = { "AT THEIR OWN RANK", "ROTTEN", "COLD", "BLOODY", "BLACK", "GILDED" };
    int in_crypt = !G.shop && !G.dead && !G.tally && !G.exiting;
    int ok = R_GOLD + 12, no = R_BLOOD + 13;
    switch (what) {
    case CH_LEVEL:
        if (num < 1 || num > 99) { con_print("LEVELS GO FROM 1 TO 99.", no); return 0; }
        G.level = RUN.level = num; RUN.hp = G.hp > 0 ? G.hp : gear_max_hp(&RUN.gear); G.retries = 0; G.ui = UI_NONE;
        begin_shop();
        G.sdur = 0.0f; G.shop_look = -1; G.pc_t = -1.0f;
        con_printf(ok, "DOWN YOU GO: THE MERCHANT, THEN LEVEL %d.", num);
        return 1;
    case CH_SEED:
        G.rs = (uint32_t)num | 1u;
        if (!G.shop) { start_level(); con_printf(ok, "THE STONES SHIFT. LEVEL %d AGAIN, FROM SEED %d.", G.level, num); }
        else con_printf(ok, "THE NEXT LEVEL WILL GROW FROM SEED %d.", num);
        return 1;
    case CH_GEAR:
        if (num < 1 || num > 15) { con_print("TIERS GO FROM 1 TO 15.", no); return 0; }
        give_gear(num);
        G.max_hp = gear_max_hp(&RUN.gear); if (G.hp > G.max_hp) G.hp = G.max_hp;
        con_printf(ok, "EVERY SPELL AT TIER %d, EVERY RUNE %s, AND NEW CLOTHES.", num, QUAL_NAME[rune_from_tier(num) - 1]);
        return 1;
    case CH_BAG:
        if (num < 1 || num > NQUAL) { con_print("QUALITIES GO FROM 1, COMMON, TO 5, EXQUISITE.", no); return 0; }
        give_bag(num - 1);
        con_printf(ok, "A BAG, %s, ALL BUT ONE PLACE FILLED.", QUAL_NAME[num - 1]);
        return 1;
    case CH_ABIL:
        if (num < 1 || num >= NABIL) { con_print("ABILITIES GO FROM 1 TO 13.", no); return 0; }
        if (!give_ability(num)) { con_print("YOU WEAR NOTHING FOR IT TO TEACH.", no); return 0; }
        G.max_hp = gear_max_hp(&RUN.gear); if (G.hp > G.max_hp) G.hp = G.max_hp;
        con_printf(ok, "ALL YOU WEAR KNOWS %s NOW.", ABIL_NAME[num]);
        return 1;
    case CH_GOLD:
        RUN.treasure += 10000; pop_readout();
        con_print("10,000 GOLD. THE MERCHANT WILL BE PLEASED.", ok);
        return 1;
    case CH_SOULS:
        soul_add(100);
        con_print("A HUNDRED SOULS, FROM SOMEWHERE.", ok);
        return 1;
    case CH_RANK:
        if (num < 1 || num > NRANK) { con_print("RANKS GO FROM 1 TO 5, AND 6 FOR GILDED.", no); return 0; }
        CON.rank = num - 1;
        con_printf(ok, "FROM NOW ON THEY COME %s.", RANK_SAY[num - 1]);
        return 1;
    case CH_HELP:
        con_help();
        return 1;
    }
    if (!in_crypt) { con_print("NOT HERE. IN THE CRYPT ITSELF.", no); return 0; }
    switch (what) {
    case CH_TPEXIT:
        if (G.exit_fx < 0) { con_print("THIS LEVEL HAS NO WAY OUT.", no); return 0; }
        tp_exit();
        con_print("YOU STAND BEFORE THE WAY OUT.", ok);
        return 1;
    case CH_TPSECRET: {
        int i = -1;
        if (!G.nsecrets) { con_print("THIS LEVEL KEEPS NO SECRETS.", no); return 0; }
        if (num > 0) {
            if (num > G.nsecrets) { con_printf(no, "THIS LEVEL HAS ONLY %d.", G.nsecrets); return 0; }
            if (G.secrets[num - 1].state) { con_print("THAT ONE IS OPEN ALREADY.", no); return 0; }
            i = num - 1;
        } else for (int k = 0; k < G.nsecrets && i < 0; k++) if (!G.secrets[k].state) i = k;
        if (i < 0) { con_print("EVERY SECRET HERE IS FOUND.", no); return 0; }
        tp_secret(i);
        con_print("A CRACKED WALL, BEFORE YOU.", ok);
        return 1;
    }
    case CH_TPALTAR:
        if (!tp_altar()) { con_print("THERE IS NO ALTAR ON THIS LEVEL.", no); return 0; }
        con_print("THE BLACK ALTAR, BEFORE YOU.", ok);
        return 1;
    case CH_WISP:
        if (!summon_wisp(WK_EXIT, 0, 0.0f)) { con_print("NO WISP WOULD COME.", no); return 0; }
        con_print("A WISP, FOR NOTHING.", ok);
        return 1;
    case CH_FOLLOW:
        if (!G.wisps[0].on && !G.wisps[1].on) { con_print("NO WISP TO FOLLOW. L CASTS ONE.", no); return 0; }
        G.pilgrim = 1;
        con_print("YOUR FEET KNOW THE WAY NOW. ANY KEY TAKES THEM BACK.", ok);
        return 1;
    case CH_WRAITH:
        for (int i = 0; i < MAXMOBS; i++) if (G.mobs[i].alive && G.mobs[i].type == MOB_WRAITH) { con_print("IT IS ALREADY HERE.", no); return 0; }
        G.wraith_now = 1; G.wraith_done = 0; G.wraith_try = 0.0f;
        con_print("SOMETHING COLD IS ON ITS WAY.", ok);
        return 1;
    case CH_MEET: {
        int k = con_kind(rest);
        if (!*rest) { con_print("CALL WHAT? A NAME FROM THE BESTIARY.", no); return 0; }
        if (k < 0) { con_print("THE BESTIARY HAS NO SUCH NAME.", no); return 0; }
        if (!meet_creature(k, 1)) { con_print("NO ROOM AHEAD FOR IT. FACE A LONGER WAY.", no); return 0; }
        con_printf(ok, "A %s, AHEAD.", KINDS[k].name);
        return 1;
    }
    }
    return 0;
}

/* a word typed and Enter: which code, if any, and what follows it */
static void con_run(const char *w) {
    int n = (int)strlen(w);
    for (int i = 0; i < (int)(sizeof CON_CODES / sizeof CON_CODES[0]); i++) {
        const ConCode *c = &CON_CODES[i];
        if (n < c->len || fnv1a(w, c->len) != c->hash) continue;
        const char *rest = w + c->len;
        int num = 0;
        if (c->arg == ARG_NONE && *rest) continue;
        if (c->arg == ARG_NUM || c->arg == ARG_OPTNUM) {
            int digits = 1;
            for (const char *d = rest; *d; d++) digits &= *d >= '0' && *d <= '9';
            if (!digits || strlen(rest) > 9) { con_print("THE CRYPT KNOWS THE WORD, NOT WHAT FOLLOWS IT.", R_BLOOD + 13); return; }
            if (!*rest && c->arg == ARG_NUM) { con_print("THAT WORD WANTS A NUMBER AFTER IT.", R_BLOOD + 13); return; }
            num = atoi(rest);
        }
        if (con_do(c->what, num, rest) && c->what != CH_HELP && !CON.used) {
            CON.used = 1;                                          /* the price, until the program is closed */
            con_print("THE DEAD TAKE NOTE. NO HONOURS AND NOTHING SIGNED WHILE THIS LASTS.", R_BLOOD + 12);
        }
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT console code %d %d t=%.2f\n", c->what, num, G.t);
#endif
        return;
    }
    con_print("THE CRYPT DOES NOT KNOW THAT WORD.", R_BLOOD + 13);
}

/* 1 while the console is down and the crypt should wait */
static int con_update(float dt) {
    CON.t += dt;
    CON.slide = CON.open ? fminf(1.0f, CON.slide + dt / 0.25f) : fmaxf(0.0f, CON.slide - dt / 0.25f);
    if (CON.open && !G.player) con_close();
    if (!G.player) return 0;
    if (!CON.open) {
        if (IN.hit[KEY_CONSOLE] && !G.ui) { con_open(); return 1; }
        return 0;
    }
    if (IN.hit[KEY_CONSOLE] || IN.hit[KEY_ESC]) { con_close(); return 1; }
    for (int i = 0; i < IN.ntext && CON.ntyped < 24; i++) CON.typed[CON.ntyped++] = IN.text[i];
    CON.typed[CON.ntyped] = 0;
    if (IN.hit[KEY_BACK] && CON.ntyped) CON.typed[--CON.ntyped] = 0;
    if (IN.hit[KEY_UP] && CON.last[0]) { memcpy(CON.typed, CON.last, sizeof CON.typed); CON.ntyped = (int)strlen(CON.typed); }
    int most = CON.n > CON_ROWS ? CON.n - CON_ROWS : 0;
    if (IN.hit[KEY_PGUP] || IN.wheel > 0) CON.scroll = clampi(CON.scroll + (IN.wheel > 0 ? IN.wheel : CON_ROWS - 1), 0, most);
    if (IN.hit[KEY_PGDN] || IN.wheel < 0) CON.scroll = clampi(CON.scroll - (IN.wheel < 0 ? -IN.wheel : CON_ROWS - 1), 0, most);
    if (IN.hit[KEY_ENTER] && CON.ntyped) {
        char w[25];
        memcpy(w, CON.typed, sizeof w);
        memcpy(CON.last, w, sizeof w);
        CON.ntyped = 0; CON.typed[0] = 0; CON.scroll = 0;
        con_printf(R_STONE + 11, "> %s", w);
        con_run(w);
    }
    return 1;
}

/* the plate: black, an iron rim below, five rows and the one you type in */
static void con_render(Frame *f) {
    if (CON.slide <= 0.0f) return;
    int ph = CON_ROWS * 10 + 22, top = (int)(-ph * (1.0f - smoothstepf(0.0f, 1.0f, CON.slide)));
    for (int y = 0; y < ph; y++) {
        int yy = top + y;
        if (yy < 0 || yy >= H) continue;
        uint8_t c = y == ph - 3 ? (uint8_t)(R_STONE + 11) : (y == ph - 2 ? (uint8_t)(R_IRON + 6) : (y == ph - 1 ? (uint8_t)(R_IRON + 2) : 0));
        memset(&f->pix[yy * W], c, W);
    }
    int first = CON.n - CON_ROWS - CON.scroll;
    for (int r = 0; r < CON_ROWS; r++) {
        int i = first + r;
        if (i >= 0 && i < CON.n) gfx_text8(f, 6, top + 4 + r * 10, CON.line[i], CON.ink[i], 1);
    }
    if (first > 0) gfx_text8(f, W - 10, top + 4, "\x18", R_GOLD + 10, 1);                     /* more above */
    if (CON.scroll > 0) gfx_text8(f, W - 10, top + 4 + (CON_ROWS - 1) * 10, "\x19", R_GOLD + 10, 1);   /* and below */
    int iy = top + 4 + CON_ROWS * 10 + 3;
    for (int x = 6; x < W - 6; x++) gfx_px(f, x, iy - 3, R_STONE + 4);
    gfx_text8(f, 6, iy, ">", R_GOLD + 12, 1);
    gfx_text8(f, 18, iy, CON.typed, R_BONE + 14, 1);
    if (fmodf(CON.t, 0.8f) < 0.45f) gfx_rect(f, 18 + CON.ntyped * 8, iy + 6, 7, 2, R_BONE + 14);   /* the cursor */
}

