/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The merchant's rooms: her wares' sprites, the pedestals' arc, the portraits hung, gen_shop, the dream's shopping, begin_shop, purchases. */

/* ================================================================== */
/* The merchant's antechamber, before each level: the owner's cat on a chest, and five to seven wares
 * on pedestals in an arc round the back wall (an attack spell, a defensive rune, the rest things to
 * wear), one to three to be had for the gold in hand, deeper down more. The first is free. A person
 * can hold one for the next visit. Then out through the east door and into the level. */
/* ================================================================== */

#define CAT_X 10.62f
#define CAT_Y 15.45f
#define CAT_S 1.05f
#define SHOP_MID 15.5f                      /* the middle of the room, east to west */
#define SHOP_DOOR 21                        /* the east door, and the way on */
#define SHOP_BEYOND() (G.px > SHOP_DOOR + 1.4f && G.py > 12.0f && G.py < 18.0f)   /* past it, in the way on: not the room north of hers,
                                                                                    which reaches as far east (1.0.0b11) */
enum { SH_GREET, SH_LOOK, SH_THINK, SH_GO, SH_BUY, SH_THANK, SH_SHRINE_GO, SH_SHRINE, SH_HALL, SH_LEAVE };
static int offer_sprite(const Offer *o) {
    static const int PIECE_SPR[NIK] = { SP_ITEM_RING, SP_ITEM_BRACER, SP_ITEM_BOOTS, SP_ITEM_AMULET, SP_ITEM_HOOD, SP_ITEM_ROBE, SP_ITEM_RING, SP_ITEM_BAG };
    return o->kind == GK_ATK ? SP_ITEM_TOME : (o->kind == GK_DEF ? SP_ITEM_RUNE : PIECE_SPR[o->piece.kind]);
}

static void ped_arc(void) {                                        /* the pedestals in an arc, its ends come forward */
    float half = 3.0f + 0.4f * (float)(G.ped_n - 5);
    for (int i = 0; i < G.ped_n; i++) {
        float t = G.ped_n > 1 ? -1.0f + 2.0f * (float)i / (float)(G.ped_n - 1) : 0.0f;
        G.ped_x[i] = SHOP_MID + t * half; G.ped_y[i] = 12.9f + 0.75f * t * t;
    }
}
/* a portrait's candles: their light in the room (shared between its flames), how far it goes, and their glow on the wall about them.
 * A third of a torch's (0.45, as the plan had it) made her room half again as bright: a ninth, and a strong glow on the wall, keeps
 * it as it was with a pool of light about each candle (CRAWL_PORLIGHT=light,reach,glow to try others) */
static float POR_LIGHT = 0.15f, POR_REACH = 2.0f, POR_GLOW = 0.8f;
/* the portraits of the slain hung on her walls (docs/PLAN-portraits.md, section 7), a bestiary page each in its order: the first
 * third on her south wall, read from the left as you face it; the second on her north wall, the spiderling round its corner by its
 * mother; the last in the back room, along its north wall and down its east. A face that carried a banner or a torch gives it up. */
static int por_gold(int k, int r);
static int por_can_draw(int i);
static int por_is_drawn(int i);
/* its candles lit: once its tapestry may come down (one of its kind slain, its boss fallen), and for good after; till then
 * cold, so that a lit candle by a tapestry still hung says there is one to draw back */
static int por_lit(int i) { return por_can_draw(i) || por_is_drawn(i); }
static void por_hang(void) {
    static const int8_t AT[22][3] = {
        { 20, 18, 3 }, { 18, 18, 3 }, { 17, 18, 3 }, { 13, 18, 3 }, { 12, 18, 3 }, { 11, 18, 3 }, { 10, 18, 3 },
        { 10, 11, 1 }, { 11, 11, 1 }, { 13, 11, 1 }, { 14, 11, 1 }, { 16, 11, 1 }, { 17, 11, 1 }, { 19, 11, 1 }, { 21, 12, 2 },
        { 7, 11, 1 }, { 8, 11, 1 }, { 9, 12, 2 }, { 9, 14, 2 }, { 9, 15, 2 }, { 9, 16, 2 }, { 9, 17, 2 } };
    int order[NKINDS], nk = por_kinds(order);
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_PORLIGHT")) sscanf(getenv("CRAWL_PORLIGHT"), "%f,%f,%f", &POR_LIGHT, &POR_REACH, &POR_GLOW);   /* testing: light,reach,glow */
    if (getenv("CRAWL_PORTRAITS") && atoi(getenv("CRAWL_PORTRAITS")) >= 0) {   /* testing: every portrait drawn back, n of each kind's
                                                                      ranks unlocked (0: their shadows); below 0 none hung */
        int n = atoi(getenv("CRAWL_PORTRAITS"));                  /* (7 and over: the feats too) */
        for (int i = 0; i < nk; i++) {
            int k = order[i], rk[NRANK], nr = por_ranks(k, rk), fi = feat_of(k);
            RUN.por_drawn |= 1u << k;
            for (int j = 0; j < nr && j < n; j++) {
                int bit = k == MOB_GHOST ? ACH_POR0 + j : k == MOB_WRAITH ? (j < 3 ? ACH_POR0 + 3 + j : ACH_BOSS0 + 6 * BOSS_WRAITH) : k * NRANK + rk[j];
                RUN.rec.got[bit >> 3] |= (uint8_t)(1 << (bit & 7));
            }
            if (n >= 7 && fi >= 0) RUN.rec.got[(ACH_FEAT0 + fi) >> 3] |= (uint8_t)(1 << ((ACH_FEAT0 + fi) & 7));
        }
        for (int b = 0; b < NBOSS && n > 0; b++) {                 /* and each boss slain, n - 1 of its steps with it */
            if (!getenv("CRAWL_PORVEIL")) RUN.por_drawn |= 1u << (POR_BOSSBIT + b);   /* (CRAWL_PORVEIL=1: their tapestries still down) */
            for (int st = 0; st < imin(n, 6); st++) RUN.rec.got[(ACH_BOSS0 + 6 * b + st) >> 3] |= (uint8_t)(1 << ((ACH_BOSS0 + 6 * b + st) & 7));
        }
        por_fill();
        RUN.por_open |= POR_WOKEN;                                 /* and her room paying */
    }
#endif
    POR.n = 0; POR.nwf = 0;
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_PORGOLD")) {                                 /* testing: the gold each rank pays, against docs/portraits/ranks.csv */
        static const char *const RW[NRANK] = { "I", "II", "III", "IV", "V", "G" };
        for (int i = 0; i < nk; i++) {
            int k = order[i];
            for (int r = 0; r < NRANK; r++)
                if (slayer_need(k, r)) fprintf(stderr, "PORGOLD %s,%s,%d,%d\n", KINDS[k].name, RW[r], slayer_need(k, r), por_gold(k, r));
            for (int q = 0; q < NPORACH; q++) {                    /* the Spectre's, the Wraith's (and its fall, paying nothing), the feats */
                PorAch a;
                por_ach(q, &a);
                if (a.kind == k) fprintf(stderr, "PORGOLD %s,%s,%d,%d\n", KINDS[k].name, a.gem == POR_FEAT ? "Feat" : RW[a.gem], a.need, por_gold(k, a.gem));
            }
            if (k == MOB_WRAITH) fprintf(stderr, "PORGOLD %s,IV,1,%d\n", KINDS[k].name, por_gold(k, RK_IV));
        }
    }
    if (getenv("CRAWL_PORTRAITS") && atoi(getenv("CRAWL_PORTRAITS")) < 0) return;   /* testing: none hung, to weigh their cost */
#endif
    for (int i = 0; i < nk && i < 22; i++) {
        PorAt *a = &POR.at[POR.n];
        a->x = AT[i][0]; a->y = AT[i][1]; a->fd = AT[i][2]; a->kind = order[i];
        a->ph = -0.83f * (float)i;                                 /* each turns about in its own time */
        Cell *c = &G.map[a->y][a->x];
        if (c->kind != K_WALL || c->por) continue;
        c->por = (uint8_t)(1 + POR.n); c->fover[a->fd] = 0;
        a->boss = a->half = 0;
        POR.veil_t[POR.n] = -1e9f;                                 /* drawn back long since, or not at all */
        POR.drawn[POR.n++] = 0;
    }
    /* the bosses' four on the grave plot's far wall, two faces each, either side of the Accursed Portal, read from the left as you face
     * it: the knight, the spider, the Portal, the Deacon, the Wraith Unbound (the mock-up's order, as the owner saw it) */
    static const int8_t BX[NBOSS] = { 20, 18, 15, 13 };            /* each one's left half; its right the cell west of it */
    for (int b = 0; b < NBOSS && POR.n + 2 <= NPOR; b++) {
        Cell *l = &G.map[PORTAL_Y][BX[b]], *r = &G.map[PORTAL_Y][BX[b] - 1];
        if (l->kind != K_WALL || r->kind != K_WALL || l->por || r->por) continue;
        for (int h = 0; h < 2; h++) {
            PorAt *a = &POR.at[POR.n];
            *a = (PorAt){ BX[b] - h, PORTAL_Y, 3, -1, 0, -0.9f * (float)b, 1 + b, h };
            Cell *c = h ? r : l;
            c->por = (uint8_t)(1 + POR.n); c->fover[3] = 0;
            POR.veil_t[POR.n] = -1e9f; POR.drawn[POR.n++] = 0;
        }
    }
    /* their candles dealt, the same every visit: never as a neighbour's (side by side, or meeting at a corner), and where it can
     * be, as none a cell further off either; and two tapestries never tied back against each other: side by side on a wall, a
     * portrait's goes to its left, but the last's of the run, which may go either way */
    int drape_left = 0;                                            /* the layouts whose tapestry goes to the left */
    for (int l = 0; l < NPLAYOUT; l++) if (!portrait_drape_side(l)) drape_left |= 1 << l;
    for (int i = 0; i < POR.n; i++) {
        PorAt *a = &POR.at[i];
        if (a->boss) continue;                                     /* theirs below */
        float cx = (float)a->x + 0.5f + 0.5f * (float)DX[a->fd], cy = (float)a->y + 0.5f + 0.5f * (float)DY[a->fd];
        int nearm = 0, farm = 0, keep = 0, cand[NPLAYOUT], nc = 0;
        for (int j = 0; j < POR.n; j++) {
            const PorAt *b = &POR.at[j];
            if (b->boss) continue;
            float bx = (float)b->x + 0.5f + 0.5f * (float)DX[b->fd], by = (float)b->y + 0.5f + 0.5f * (float)DY[b->fd], d = hypotf(cx - bx, cy - by);
            if (j < i && d < 1.05f) nearm |= 1 << b->layout; else if (j < i && d < 2.1f) farm |= 1 << b->layout;
            if (j != i && d < 1.05f && b->fd == a->fd && (bx - cx) * (float)DY[a->fd] - (by - cy) * (float)DX[a->fd] > 0.0f)
                keep = ~drape_left;                                /* one to its right as seen: its tapestry goes left */
        }
        for (int l = 0; l < NPLAYOUT; l++) if (!((nearm | farm | keep) >> l & 1)) cand[nc++] = l;
        if (!nc) for (int l = 0; l < NPLAYOUT; l++) if (!((nearm | keep) >> l & 1)) cand[nc++] = l;
        if (!nc) for (int l = 0; l < NPLAYOUT; l++) if (!(nearm >> l & 1)) cand[nc++] = l;
        uint32_t h = (uint32_t)(a->x * 73 + a->y * 19 + a->fd * 7 + 101) * 2654435761u;   /* not hash2: that has the level in it */
        h ^= h >> 15; h *= 2246822519u; h ^= h >> 13;
        a->layout = cand[h % (uint32_t)nc];
        if (!por_lit(i)) continue;                                 /* its candles cold: no light, no glow */
        /* each flame a light of a third of a torch's, shared between its flames, and a glow on the wall about it */
        float fl[4][3], nx = (float)DX[a->fd], ny = (float)DY[a->fd], rx = ny, ry = -nx;   /* rx, ry: to the right as seen */
        int nf = portrait_layout_flames(a->layout, fl);
        for (int k = 0; k < nf && POR.nwf < NPOR * 4; k++) {
            float u = fl[k][0] / (float)TS - 0.5f, wx = cx + rx * u, wy = cy + ry * u, share = 1.0f / (nf > 1 ? 2.0f : 1.0f);
            PorFlame *w = &POR.wf[POR.nwf];
            w->fd = a->fd; w->plane = a->fd == 0 || a->fd == 2 ? cx : cy; w->along = a->fd == 0 || a->fd == 2 ? wy : wx;
            w->z = fl[k][1] / (float)TS; w->pw = POR_GLOW * share; w->light = G.nlights;
            light_static_ph(wx + nx * 0.12f, wy + ny * 0.12f, POR_LIGHT * share, POR_REACH, (float)((h >> (4 * k)) % 1000u) * 0.01f);
            if (G.nlights > w->light) POR.nwf++;                   /* (none if the lights were full) */
        }
    }
    float bf[6][3];                                                /* a boss's two stands of three: a light each, a glow a flame */
    int nbf = portrait_boss_flames(bf);
    for (int i = 0; i < POR.n; i++) {
        const PorAt *a = &POR.at[i];
        if (!a->boss || a->half || !por_lit(i)) continue;          /* (cold till it has fallen) */
        float nx = (float)DX[a->fd], ny = (float)DY[a->fd], rx = ny, ry = -nx;
        float cx = (float)a->x + 0.5f + 0.5f * nx + rx * 0.5f, cy = (float)a->y + 0.5f + 0.5f * ny + ry * 0.5f;   /* between its two faces */
        for (int st = 0; st < 2; st++) {
            int light = -1;
            for (int k = st * 3; k < st * 3 + 3 && k < nbf && POR.nwf < NPOR * 4; k++) {
                float u = bf[k][0] / (float)TS - 1.0f, wx = cx + rx * u, wy = cy + ry * u;
                if (light < 0) {
                    float mu = bf[st * 3 + 1][0] / (float)TS - 1.0f;
                    light = G.nlights;
                    light_static_ph(cx + rx * mu + nx * 0.12f, cy + ry * mu + ny * 0.12f, POR_LIGHT * 1.5f, POR_REACH, (float)(a->boss * 7 + st * 3) * 0.37f);
                    if (G.nlights <= light) break;
                }
                PorFlame *w = &POR.wf[POR.nwf++];
                w->fd = a->fd; w->plane = a->fd == 0 || a->fd == 2 ? cx : cy; w->along = a->fd == 0 || a->fd == 2 ? wy : wx;
                w->z = bf[k][1] / (float)TS; w->pw = POR_GLOW * 0.5f; w->light = light;
            }
        }
    }
#ifdef CRAWL_DEBUG
    { static int said; if (!said++) {
        fprintf(stderr, "PORTRAITS hung %d, candles %d, lights %d:", POR.n, POR.nwf, G.nlights);
        for (int i = 0; i < POR.n; i++) fprintf(stderr, " %d@%d,%d:%d%s", POR.at[i].kind, POR.at[i].x, POR.at[i].y, POR.at[i].layout, por_lit(i) ? "" : "c");
        fprintf(stderr, "\n"); } }
#endif
}

static void gen_shop(void) {
    G.faced = 1; G.look = 0;                                        /* her hall in the Crypts' stone, whatever the run (begin_shop draws level 1's) */
    memset(G.map, 0, sizeof G.map);
    for (int y = 0; y < MAPN; y++)
        for (int x = 0; x < MAPN; x++) {
            Cell *c = &G.map[y][x];
            c->kind = K_WALL; c->region = -1; c->door = -1;
            c->tex = (uint8_t)(hash2(x, y) % 10 < 8 ? T_BRICK : T_BANNER);
        }
    G.nrooms = 0; G.ndoors = 0; G.nlights = 0; G.nlc = 0; G.ndecos = 0; G.nitems = 0;
    G.items_total = G.items_got = G.value_total = G.value_got = 0; G.nvaults = 0;
    G.nsecrets = G.secrets_found = G.nlinks = G.links_found = 0; G.slide = G.seek_secret = -1;
    for (int k = 0; k < NKEYS; k++) G.vault_room[k] = G.vault_door[k] = -1;
    memset(G.bulky, 0, sizeof G.bulky); memset(G.fog, 0, sizeof G.fog); memset(G.mapped, 0, sizeof G.mapped); memset(G.foot, 0, sizeof G.foot);
    for (int y = 12; y <= 17; y++) for (int x = 10; x <= 20; x++) carve(x, y, 0, TH_BRICK);
    G.rooms[G.nrooms++] = (Room){ 10, 12, 11, 6, 0, TH_BRICK, RT_MERCHANT };
    for (int x = SHOP_DOOR; x <= SHOP_DOOR + 5; x++) carve(x, 15, 1, TH_CORRIDOR);
    G.map[15][SHOP_DOOR].kind = K_DOOR; G.map[15][SHOP_DOOR].door = 0;
    G.doors[0] = (Door){ SHOP_DOOR, 15, 1, 0.0f, 0.0f, 0, 0, 0, 0 }; G.ndoors = 1;
    G.map[11][15].tex = T_SIGN;                                    /* the merchant's sign behind the wares */
    G.map[11][12].tex = G.map[11][18].tex = T_TORCH0;
    light_static(12.5f, 11.92f, 1.35f, 4.6f); light_static(18.5f, 11.92f, 1.35f, 4.6f);
    G.map[17][9].tex = T_TORCH0; light_static(10.08f, 17.5f, 1.2f, 4.2f);   /* by the cat */
    G.map[13][SHOP_DOOR].tex = T_TORCH0; light_static(SHOP_DOOR - 0.08f, 13.5f, 1.2f, 4.2f);   /* the east end */
    G.map[11][14].tex = G.map[11][16].tex = T_BRICK;
    add_deco(10.4f, 12.4f, SP_CANDLES); light_static(10.45f, 12.45f, 0.9f, 3.0f);
    add_deco(15.5f, 12.3f, SP_CANDLES); light_static(15.5f, 12.35f, 0.8f, 3.2f);   /* under the sign */
    add_deco(20.6f, 14.05f, SP_URN);                                /* by the way on, under its torch: it stood before the skeletons' portrait (1.0.0b21.2) */
    G.sh_deco = G.ndecos;                                          /* the Rune Shrine, across from the wares */
    add_deco(SHRINE_X, SHRINE_Y, RUN.shrine ? SP_SHRINE_C : SP_SHRINE_S);
    G.map[(int)SHRINE_Y][(int)SHRINE_X].kind = K_PROP;
    add_deco(SHRINE_X - 1.15f, 17.6f, SP_CANDLES); light_static(SHRINE_X - 1.1f, 17.55f, 0.95f, 3.4f);
    add_deco(SHRINE_X + 1.15f, 17.6f, SP_CANDLES); light_static(SHRINE_X + 1.1f, 17.55f, 0.95f, 3.4f);
    for (int y = 12; y <= 17; y++) for (int x = 4; x <= 8; x++) carve(x, y, 2, TH_BRICK);   /* the back room, through an arch beside her */
    carve(9, 13, 2, TH_BRICK);
    G.rooms[G.nrooms++] = (Room){ 4, 12, 5, 6, 2, TH_BRICK, RT_MERCHANT };
    solid_prop((int)TOTEM_X, (int)TOTEM_Y, SP_HEXTOTEM, 1, K_PILLAR);   /* the Hexed Totem, in its corner */
    add_deco(TOTEM_X + 1.05f, TOTEM_Y - 0.1f, SP_CANDLES); light_static(TOTEM_X + 1.0f, TOTEM_Y + 0.2f, 1.0f, 3.6f);   /* candles at its foot */
    G.map[18][6].tex = T_TORCH0; light_static(6.5f, 17.92f, 1.2f, 4.4f);
    solid_prop(LECTERN_X, LECTERN_Y, SP_LECTERN, 1, K_PROP);        /* the Corpus Compendium, open on its lectern */
    for (int y = 13; y <= 16; y++) G.map[y][3].tex = T_REPOSITORY;  /* the Wretched Repository, shut in the west wall */
    light_static(LECTERN_X + 0.5f, LECTERN_Y + 0.4f, 0.8f, 3.0f);
    for (int y = 20; y <= 26; y++) for (int x = 11; x <= 21; x++) carve(x, y, 3, TH_CRYPT);   /* the grave plot, through an arch past the shrine */
    carve(19, 18, 3, TH_CRYPT); carve(19, 19, 3, TH_CRYPT);
    G.rooms[G.nrooms++] = (Room){ 11, 20, 11, 7, 3, TH_CRYPT, RT_PLOT };
    G.map[PORTAL_Y][PORTAL_X].tex = T_PORTAL0;                     /* the Accursed Portal in its far wall */
    lantern(16, 23); lantern(19, 21);
    for (int i = 0; i < RUN.nstones; i++)                          /* a stone a death, the newest clean-cut */
        solid_prop(STONE_AT[i][0], STONE_AT[i][1], i == 0 ? SP_TOMB_FRESH : SP_TOMB_ROUND + (int)((uint64_t)RUN.stones[i].when % 3), 1, K_PROP);
    G.map[21][10].tex = T_TORCH0; light_static(10.92f, 21.5f, 1.1f, 4.2f);
    for (int y = 3; y <= 9; y++) for (int x = 15; x <= 25; x++) carve(x, y, 4, TH_BRICK);   /* the room north of hers (1.0.0b11), */
    carve(20, 10, 4, TH_BRICK); carve(20, 11, 4, TH_BRICK);         /* through an arch in her north wall */
    G.rooms[G.nrooms++] = (Room){ 15, 3, 11, 7, 4, TH_BRICK, RT_MERCHANT };
    solid_prop(LAMP_X, LAMP_Y, SP_LAMPSTAND, 1, K_PILLAR);          /* the Lampstand against its far wall */
    add_deco(LAMP_X - 1.6f, LAMP_Y + 0.4f, SP_CANDLES); light_static(LAMP_X - 1.55f, LAMP_Y + 0.5f, 0.9f, 3.4f);
    add_deco(LAMP_X + 2.6f, LAMP_Y + 0.4f, SP_CANDLES); light_static(LAMP_X + 2.55f, LAMP_Y + 0.5f, 0.9f, 3.4f);
    G.map[9][14].tex = T_TORCH0; light_static(15.08f, 9.5f, 1.0f, 4.0f);
    G.bell_deco = G.ndecos; solid_prop(BELL_X, BELL_Y, RUN.bell_cursed ? SP_DBELL_C : SP_DBELL, 1, K_PROP);   /* the Dead Bell in its east half, */
    G.font_deco = G.ndecos; solid_prop(FONT_X, FONT_Y, RUN.font_cursed ? SP_FONT_C : SP_FONT, 1, K_PROP);    /* the Weeping Font in its west */
    add_deco(BELL_X + 1.45f, BELL_Y + 0.3f, SP_CANDLES); light_static(BELL_X + 1.4f, BELL_Y + 0.5f, 0.85f, 3.2f);   /* candles by each */
    add_deco(FONT_X - 0.45f, FONT_Y + 0.3f, SP_CANDLES); light_static(FONT_X - 0.4f, FONT_Y + 0.5f, 0.85f, 3.2f);
    G.ped_n = clampi(RUN.peds, 2, NPED);                           /* the pedestals bought */
    ped_arc();
    for (int i = 0; i < G.ped_n; i++) { G.ped_deco[i] = G.ndecos; add_deco(G.ped_x[i], G.ped_y[i], SP_PEDESTAL); }
    G.ncells = 0;
    for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) if (G.map[y][x].kind == K_FLOOR) G.ncells++;
    lay_faces();
    por_hang();                                                    /* the portraits of the slain, over the faces laid */
}

static void shop_turn(float to, float dur) {
    G.sa0 = G.ang; G.sa1 = G.ang + wrap_pi(to - G.ang); G.sdur = dur; G.smove = 0.0f;
    G.sx0 = G.sx1 = G.px; G.sy0 = G.sy1 = G.py;
}
static void shop_walk(float x, float y, float speed) {
    G.sx0 = G.px; G.sy0 = G.py; G.sx1 = x; G.sy1 = y;
    G.sa0 = G.sa1 = G.ang; G.sdur = fmaxf(0.2f, hypotf(x - G.px, y - G.py) / speed); G.smove = 1.0f;
}
static float face_of(float x, float y) { return atan2f(y - G.py, x - G.px); }

/* which ware, if any: best value for the gold, a whim now and then, sometimes saving up for a
 * spell it cannot yet afford, sometimes splashing out on the dearest thing it can */
static int shop_choose(void) {
    const Gear *g = &RUN.gear;
    float v[NPED];
    int best = -1, dear = -1, purse = RUN.treasure;
    float bs = 0.0f;
    if (G.shop_bought > 0) purse -= (int)(gear_haul(G.level) * 0.15f);   /* after the first, a little gold kept back */
    for (int i = 0; i < G.ped_n; i++) {
        v[i] = G.offer_gone[i] ? 0.0f : gear_value(g, &G.offers[i], G.level);
        if (v[i] <= 0.0f || G.offers[i].price > purse) continue;
        float s = v[i] / sqrtf(fmaxf(1.0f, (float)G.offers[i].price) / 100.0f + 1.0f) * (0.8f + 0.4f * rndf());
        if (s > bs) { bs = s; best = i; }
        if (dear < 0 || G.offers[i].price > G.offers[dear].price) dear = i;
    }
    G.shop_saving = 0;
    if (G.level == 1 || G.shop_bought > 0) return best;            /* the first is free; after one, the best for the gold */
    #define SPELL(o) ((o).kind == GK_ATK || (o).kind == GK_DEF)
    float next = gear_haul(G.level) * 0.6f;                        /* what the coming level should bring, cautiously */
    if (RUN.goal > 0) {                                            /* saving: a spell worth the wait now? */
        int sp = -1;
        for (int i = 0; i < G.ped_n; i++)
            if (SPELL(G.offers[i]) && G.offers[i].price <= RUN.treasure && v[i] > 0.5f && (sp < 0 || v[i] > v[sp])) sp = i;
        if (sp >= 0) { RUN.goal = 0; return sp; }
        if (RUN.treasure < RUN.goal && RUN.goal <= RUN.treasure + 2.0f * next && rndf() < 0.7f) { G.shop_saving = 1; return -1; }
        RUN.goal = 0;                                              /* bought elsewhere, or out of reach */
    }
    for (int i = 0; i < G.ped_n; i++) {                            /* a spell it can't quite afford: put the gold by, */
        if (!SPELL(G.offers[i]) || G.offers[i].price <= RUN.treasure || v[i] <= (best >= 0 ? v[best] : 0.3f)) continue;
        if (G.offers[i].price > RUN.treasure + 2.0f * next) continue;   /* if two levels might bring it */
        float p = G.offers[i].price <= RUN.treasure + next ? 0.4f : 0.15f;
        if (best >= 0 && SPELL(G.offers[best])) p *= 0.3f;         /* a spell in hand already */
        if (rndf() < p) { RUN.goal = G.offers[i].price; G.shop_saving = 1; return -1; }
    }
    #undef SPELL
    if (dear >= 0 && rndf() < 0.12f) return dear;                  /* splurge */
    return best;
}

static void por_dream(void);
static void begin_shop(void) {
    hz_clear(); snd_set_boss(0, 0);
    g_drawn = 0;                                                   /* her rooms are on the plan's grid */
    G.pz = -1; G.nthing = 0; G.carry = 0;                          /* nor a puzzle */
    G.msg_deco = G.srch_deco = G.bsh_x = -1; docs_clear();
    G.tri = G.tri_room = G.tri_door = -1; G.work_i = 0; snd_set_trial(0);
    G.bchallenge = 0;
    G.shop = 1; G.state = PS_SHOP; G.nmk = 0;
    memset(G.wisps, 0, sizeof G.wisps);                            /* nor into the merchant's room */
#ifdef CRAWL_DEBUG
    { static int once; clock_t c0 = clock(); portrait_keep_runs();
      if (!once++) fprintf(stderr, "PORTRAITS the runs' stone kept in %.1f ms\n", (double)(clock() - c0) * 1000.0 / CLOCKS_PER_SEC); }
#else
    portrait_keep_runs();                                          /* the first time: each run's stone for the portraits behind them */
#endif
    crawl_art_level(1);                                            /* the antechamber looks the same every time */
    gen_shop();
    por_dream();                                                   /* the dream draws a tapestry back now and then */
    memset(G.mobs, 0, sizeof G.mobs); G.nroute = 0; G.npacks = 0; memset(G.shots, 0, sizeof G.shots); memset(G.booms, 0, sizeof G.booms);
    memset(G.bones, 0, sizeof G.bones); memset(G.beams, 0, sizeof G.beams); memset(G.burns, 0, sizeof G.burns);
    memset(G.trails, 0, sizeof G.trails); memset(G.venoms, 0, sizeof G.venoms); memset(G.motes, 0, sizeof G.motes);
    soul_cap(1);
    reset_caster_state();
    if (G.hp <= 0) G.hp = G.max_hp;
    G.tally = 0; G.dead = 0; G.exiting = G.leaving = 0; G.readout = 0.0f; G.card = 99.0f;
    G.hurt = G.gold = G.kick = G.chill = 0.0f;
    G.px = 13.6f; G.py = 16.2f; G.cx = 13; G.cy = 16;              /* close by the merchant, the wares before you */
    G.ang = face_of(CAT_X, CAT_Y);
    G.camz = 0.5f; G.hand = 0.0f; G.spd = 0.0f; G.bob = 0.0f; G.bar = 1.0f;
    old_spells_said();                                             /* a page from before the Alignment */
    old_lines_said();                                              /* or before the Confessions */
#ifdef CRAWL_DEBUG
    por_trace("her room");
#endif
    if (!RUN.cls && !LEDGER.yours && !LEDGER.kept) class_take(1 + class_random());   /* the dream: what it was, at random, so every line shows in time */
    line_lend();
    gift_settle();                                                 /* the Lampstand's gift: the dream's at once */
    G.gift_said = 0;
    if (d6_on()) align_sync();                                     /* the wares by the lines and the class as they are now (D6) */
    int bag_up = 0;
    for (int p = 0; p < G.ped_n; p++) {                            /* now and then the BFS9000, the rest to wear; one bag at most */
        int pool = p == 0 ? POOL_ATTACK : POOL_GEAR;               /* runes are the Rune Shrine's, spells the lines' */
        Offer o = gear_offer(pool, G.level, &RUN.gear, &G.rs);
        for (int k = 0; k < 8 && bag_up && o.kind == GK_PIECE && o.piece.kind == IK_BAG; k++) o = gear_offer(pool, G.level, &RUN.gear, &G.rs);
        bag_up |= o.kind == GK_PIECE && o.piece.kind == IK_BAG;
        if (G.level == 1) o.price = 0;
        G.offers[p] = o; G.offer_gone[p] = G.offer_back[p] = 0;
    }
    if (RUN.gear.has_held) {                                       /* last visit's hold: back as it was, at the price it showed */
        const Offer *h = &RUN.gear.held;
        int p = h->kind == GK_ATK ? 0 : imin(2, G.ped_n - 1);
        G.offers[p] = *h; G.offer_back[p] = 1;
    }
    G.shop_left = gear_shop_purchases(G.level); G.shop_confirm = G.shop_letgo = G.shop_held = -1; G.comp_tried = 0;
    G.sh_look = G.sh_takes = G.sh_pulls = G.sh_nq = G.sh_busy = G.sh_auto = G.sh_fav = 0; G.sh_mode = 0; G.sh_arm = -1;
    G.sh_qt = G.sh_glow = G.sh_note_t = 0.0f; memset(G.sh_flash, 0, sizeof G.sh_flash);
    G.shop_step = SH_GREET; G.shop_t = 0.0; G.shop_pick = -1; G.shop_bought = 0; G.shop_look = -1;
    G.shop_gold = RUN.treasure; G.shop_saving = 0; G.sparkle = 0.0f;
    G.shop_heal = G.hp + (G.max_hp - G.hp) * 4 / 10;               /* a rest by the purring cat */
    G.cat_blink = G.cat_slow = G.cat_ear = G.cat_trill = 0.0f; G.purr_t = 0.4f;
    G.cat_next_blink = 2.5f + 2.0f * rndf(); G.cat_next_ear = 3.0f + 3.0f * rndf(); G.cat_tail_k = 0.0f;
    G.sdur = 0.0f; G.smove = 0.0f;
    G.level_t = 0.0;
    if (LEDGER.has_visit && LEDGER.visit.level == G.level) {       /* a page opened here: her visit as it was signed */
        const Visit *v = &LEDGER.visit;
        for (int p = 0; p < G.ped_n && p < v->ped_n; p++) { G.offers[p] = v->offers[p]; G.offer_gone[p] = v->gone[p]; G.offer_back[p] = v->back[p]; }
        G.shop_left = v->left; G.shop_bought = v->bought; G.sh_takes = v->takes; G.shop_held = v->held < G.ped_n ? v->held : -1;
    }
    LEDGER.has_visit = 0;
    tu_shop_begin();                                               /* after level 1, the first time: a page on her wares */
#ifdef CRAWL_DEBUG
    fprintf(stderr, "SHOP level %d gold %d souls %d:", G.level, RUN.treasure, RUN.souls);
    for (int p = 0; p < G.ped_n; p++) { char nm[80]; gear_name(&G.offers[p], nm, sizeof nm); fprintf(stderr, " [%s $%d%s%s%s]", nm, G.offers[p].price, G.offers[p].rare ? " RARE" : "", G.offer_back[p] ? " HELD" : "", G.offer_gone[p] ? " GONE" : ""); }
    fprintf(stderr, " left %d\n", G.shop_left);
#endif
}

static void shop_cat(float dt) {
    G.cat_blink = fmaxf(0.0f, G.cat_blink - dt);
    G.cat_slow = fmaxf(0.0f, G.cat_slow - dt);
    G.cat_ear = fmaxf(0.0f, G.cat_ear - dt);
    G.cat_trill = fmaxf(0.0f, G.cat_trill - dt);
    G.cat_tail_k = fmaxf(0.0f, G.cat_tail_k - dt * 0.25f);
    if ((G.cat_next_blink -= dt) <= 0.0f) { if (G.cat_slow <= 0.0f) G.cat_blink = 0.16f; G.cat_next_blink = 2.5f + 4.0f * rndf(); }
    if ((G.cat_next_ear -= dt) <= 0.0f) { G.cat_ear = 0.32f; G.cat_ear_side = rnd(2); G.cat_next_ear = 3.5f + 5.0f * rndf(); }
    float want = clampf(wrap_pi(atan2f(G.py - CAT_Y, G.px - CAT_X)) * 2.2f, -1.0f, 1.0f);   /* eyes on the caster */
    G.cat_look += (want - G.cat_look) * fminf(1.0f, dt * 4.0f);
    if ((G.purr_t -= dt) <= 0.0f) { sfx_at(SFX_PURR, CAT_X, CAT_Y, 0.55f); G.purr_t = 1.95f; }   /* quietly, on and on */
}

/* the ware on pedestal i changes hands */
static void shop_purchase(int i) {
    const Offer *o = &G.offers[i];
    G.shop_pick = i; G.shop_bought++; G.shop_left--; G.offer_gone[i] = 1; G.sparkle = 0.8f; G.shop_confirm = -1;
    if (i == G.shop_held) { RUN.gear.has_held = 0; G.shop_held = -1; }   /* bought after all: the hold is spent */
    RUN.treasure -= o->price;
    if (o->kind == GK_PIECE) take_piece(o, G.player);
    else gear_take(&RUN.gear, o);
    if (o->price > 0) snd_sfx_ex(SFX_COIN, 1.1f, 0.5f);
    snd_sfx_ex(SFX_SHIMMER, 0.8f, 0.5f);
    G.grin_t = 2.5f;
    sfx_at(SFX_TRILL, CAT_X, CAT_Y, 1.3f); G.cat_trill = 0.5f; G.cat_slow = 1.6f; G.cat_tail_k = 1.0f;
#ifdef CRAWL_DEBUG
    char nm[80]; gear_name(o, nm, sizeof nm);
    fprintf(stderr, "EVENT bought %s for %d (left %d) t=%.2f\n", nm, o->price, RUN.treasure, G.t);
#endif
}


/* another pedestal, bought from her: the arc opens out and a ware is set on it at once */
static int ped_price(void) { return RUN.peds < NPED ? PED_PRICE[RUN.peds + 1] : 0; }
static void ped_buy(void) {
    if (RUN.peds >= NPED || G.ped_n >= NPED || RUN.treasure < ped_price()) return;
    RUN.treasure -= ped_price(); RUN.peds++;
    if (G.shop) {
        int p = G.ped_n++, bag_up = 0;
        ped_arc();
        for (int i = 0; i < p; i++) if (G.ped_deco[i] >= 0 && G.ped_deco[i] < G.ndecos) { G.decos[G.ped_deco[i]].x = G.ped_x[i]; G.decos[G.ped_deco[i]].y = G.ped_y[i]; }
        G.ped_deco[p] = G.ndecos; add_deco(G.ped_x[p], G.ped_y[p], SP_PEDESTAL);
        for (int i = 0; i < p; i++) bag_up |= G.offers[i].kind == GK_PIECE && G.offers[i].piece.kind == IK_BAG;
        Offer o = gear_offer(POOL_GEAR, G.level, &RUN.gear, &G.rs);
        for (int k = 0; k < 8 && bag_up && o.kind == GK_PIECE && o.piece.kind == IK_BAG; k++) o = gear_offer(POOL_GEAR, G.level, &RUN.gear, &G.rs);
        if (G.level == 1) o.price = 0;
        G.offers[p] = o; G.offer_gone[p] = G.offer_back[p] = 0;
        G.shop_gold = RUN.treasure; G.sparkle = 0.8f;
    }
    snd_sfx_ex(SFX_COIN, 1.1f, 0.5f); snd_sfx_ex(SFX_PUSHWALL, 0.6f, 0.5f);
    sfx_at(SFX_TRILL, CAT_X, CAT_Y, 1.1f); G.cat_trill = 0.5f; G.cat_slow = 1.2f;
    say(RUN.peds >= NPED ? "SHE SETS OUT THE SEVENTH PEDESTAL, THE LAST" : "SHE SETS OUT ANOTHER PEDESTAL");
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT pedestal %d for %d (left %d) t=%.2f\n", RUN.peds, PED_PRICE[RUN.peds], RUN.treasure, G.t);
#endif
}

