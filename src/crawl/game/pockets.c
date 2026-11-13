/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Pockets: the caster carried across, gen_pocket, the way in and back; crawl_enter; collect (what picking a thing up does). */

/* ---- pockets (docs/PLAN-b6.md): the deeper links of a secret's chain, each a place of its own beyond
 * a doorway. Walking into the doorway at the back of a chained room takes you into the chain's first
 * pocket; its doorway back returns you there, and a doorway on goes deeper. The level waits, as it is,
 * while you are away (G_LEVEL keeps it), and a pocket is built afresh each time from its chain's seed,
 * less what has been taken from it. Dying in one throws you back to the chain's room, whole. ---- */


/* what goes with the caster through a doorway; the rest of G is the place. Ranges of G, in its order */
#define G_SPAN(a, b) (offsetof(GState, b) + sizeof(((GState *)0)->b) - offsetof(GState, a))
#define G_CARRY(from, a, b) memcpy((char *)&G + offsetof(GState, a), (const char *)(from) + offsetof(GState, a), G_SPAN(a, b))
static void carry_caster(const GState *from) {
    G.t = from->t; G.rs = from->rs; G.kills = from->kills; G.card = from->card; G.mood_t = from->mood_t;
    G_CARRY(from, px, blocked);                                    /* where it stands and how it moves, the hand, the spell */
    G_CARRY(from, pvx, unlock_t);                                  /* health, this level's gains, keys */
    G_CARRY(from, poison_t, commune_r);                            /* what is on it, the runes running, the abilities' clocks */
    G_CARRY(from, def_cd, face_side);                              /* the runes' cooldowns, the portrait */
    G_CARRY(from, hurt, chill);
    G_CARRY(from, player, ab_ready);                               /* a person's controls and menus, the abilities' clocks */
    G.tether = 0; G.latch = 0;                                     /* what it held, or held it, is the other place's */
}

static void pocket_say_found(int link) {                           /* the first time into a link: a secret, on the level's count */
    if (link <= PK.reached[PK.chain]) return;
    PK.reached[PK.chain] = (uint8_t)link;
    G_LEVEL.links_found++;
    rec_secret();
    say("A SECRET!   %d OF %d", G_LEVEL.links_found, G_LEVEL.nlinks);
    xp_gain(25.0f * xp_level(), "A SECRET");
    if (G_LEVEL.links_found >= G_LEVEL.nlinks) gain_coin(COIN_ALL, "EVERY SECRET HERE FOUND");
    snd_sfx_ex(SFX_SECRET, 0.9f, 0.5f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT secret %d of %d (pocket %d of chain %d) t=%.2f\n", G_LEVEL.links_found, G_LEVEL.nlinks, link, PK.chain, G.t);
#endif
}

/* A pocket, built into G: blocks of three by three in a square, joined as a maze with loops, some
 * walls opened right across into halls; lit here and there, dark between. It grows with the level and
 * the link, from 13 cells across early in the Crypts to the whole 45 at the end of a long chain in the
 * Deep. The way back is a doorway in its west wall; the way on, if the chain goes on, a doorway in an
 * outer wall as far as can be from it; else the chain's reward (for now a hoard, until the themes bring
 * theirs). Its things come in the same order each time, so what was taken can stay taken. */
static void gen_pocket(const Secret *sc, int link) {
    G.nmk = 0; G.ward_said = 0;                                    /* no seals in a pocket */
    G.faced = 0;                                                    /* a pocket keeps its own look */
    memset(G.haz, 0, sizeof G.haz); G.hz_acc = 0.0f;               /* the level's hazards wait with it */
    if (G.silenced) { G.silenced = 0; snd_set_muffle(0); }
    int lv = (G.level - 1) % DESCENT_LEN;
    float t = clampf(0.6f * (float)lv / (float)(DESCENT_LEN - 1) + 0.4f * (float)(link - 2) / 4.0f, 0.0f, 1.0f);
    int n = clampi(3 + (int)(t * 8.0f + 0.5f), 3, 11), S = 4 * n + 1, o = ((MAPN - S) / 2) & ~3;
    uint32_t keep_rs = G.rs;
    G.rs = sc->seed ^ ((uint32_t)link * 2654435761u);
    G.ptheme = crawl_set(G.level) < 2 ? PT_BACKROOMS : PT_SEVERANCE;   /* the Backrooms for the first two runs, Severance for the last */
    G.vis_x = G.vis_y = G.vis_deco = -1;
    int back = G.ptheme == PT_BACKROOMS, sev = G.ptheme == PT_SEVERANCE;
    for (int y = 0; y < MAPN; y++)
        for (int x = 0; x < MAPN; x++) { Cell *c = &G.map[y][x]; memset(c, 0, sizeof *c); c->kind = K_WALL; c->tex = back ? T_PAPER : sev ? T_WHITE : T_STONE; c->region = -1; c->door = -1; }
    memset(G.mapped, 0, sizeof G.mapped);                          /* the pocket's own map */
    #define BX(i) (o + 4 * (i) + 1)
    for (int j = 0; j < n && !sev; j++)                            /* Severance carves its halls once it knows its far end */
        for (int i = 0; i < n; i++)
            for (int y = BX(j); y < BX(j) + 3; y++) for (int x = BX(i); x < BX(i) + 3; x++) carve(x, y, 0, TH_CORRIDOR);
    /* the maze: a spanning tree over the blocks, then loops; a wall opened right across, or by a door's width */
    static uint8_t seen[11][11], open_e[11][11], open_s[11][11];
    static int st[121];
    memset(seen, 0, sizeof seen); memset(open_e, 0, sizeof open_e); memset(open_s, 0, sizeof open_s);
    int sp = 0, j0 = rnd(n);
    st[sp++] = j0 * 11; seen[j0][0] = 1;
    while (sp > 0) {
        int b = st[sp - 1], bi = b % 11, bj = b / 11, opts[4], no = 0;
        for (int d = 0; d < 4; d++) { int ni = bi + DX[d], nj = bj + DY[d]; if (ni >= 0 && nj >= 0 && ni < n && nj < n && !seen[nj][ni]) opts[no++] = d; }
        if (!no) { sp--; continue; }
        int d = opts[rnd(no)], ni = bi + DX[d], nj = bj + DY[d];
        if (d == 0) open_e[bj][bi] = 1 + rnd(2); else if (d == 2) open_e[nj][ni] = 1 + rnd(2);
        else if (d == 1) open_s[bj][bi] = 1 + rnd(2); else open_s[nj][ni] = 1 + rnd(2);
        seen[nj][ni] = 1; st[sp++] = nj * 11 + ni;
    }
    for (int j = 0; j < n; j++)
        for (int i = 0; i < n; i++) {
            if (i + 1 < n && !open_e[j][i] && rnd(100) < 35) open_e[j][i] = 1 + rnd(2);
            if (j + 1 < n && !open_s[j][i] && rnd(100) < 35) open_s[j][i] = 1 + rnd(2);
        }
    for (int j = 0; j < n && !sev; j++)
        for (int i = 0; i < n; i++) {
            if (open_e[j][i]) for (int k = open_e[j][i] == 2 ? 0 : 1; k < (open_e[j][i] == 2 ? 3 : 2); k++) carve(BX(i) + 3, BX(j) + k, 0, TH_CORRIDOR);
            if (open_s[j][i]) for (int k = open_s[j][i] == 2 ? 0 : 1; k < (open_s[j][i] == 2 ? 3 : 2); k++) carve(BX(i) + k, BX(j) + 3, 0, TH_CORRIDOR);
            if (i + 1 < n && j + 1 < n && open_e[j][i] == 2 && open_s[j][i] == 2 && open_e[j + 1][i] == 2 && open_s[j][i + 1] == 2)
                carve(BX(i) + 3, BX(j) + 3, 0, TH_CORRIDOR);       /* four walls gone: the corner too, a hall */
        }
    if (back)                                                      /* here and there a leak, now and then a socket */
        for (int y = o; y < o + S; y++)
            for (int x = o; x < o + S; x++) {
                uint32_t h = hash2(x, y) ^ sc->seed;
                if (G.map[y][x].kind == K_WALL) G.map[y][x].tex = h % 100 < 6 ? T_PAPER_DAMP : h % 100 < 9 ? T_PAPER_OUTLET : T_PAPER;
            }
    /* the way back, in the west wall; the farthest block, by the maze */
    PK.back_x = o; PK.back_y = BX(j0) + 1;
    int door_tex = back ? T_BDOOR : sev ? T_SDOOR : T_PASSAGE;
    G.map[PK.back_y][PK.back_x].secret = 3; G.map[PK.back_y][PK.back_x].tex = (uint8_t)door_tex;
    static int dist[11][11], q[121];
    for (int j = 0; j < n; j++) for (int i = 0; i < n; i++) dist[j][i] = -1;
    int qh = 0, qt = 0, far = j0 * 11;
    dist[j0][0] = 0; q[qt++] = j0 * 11;
    while (qh < qt) {
        int b = q[qh++], bi = b % 11, bj = b / 11;
        if (dist[bj][bi] > dist[far / 11][far % 11]) far = b;
        for (int d = 0; d < 4; d++) {
            int ni = bi + DX[d], nj = bj + DY[d];
            if (ni < 0 || nj < 0 || ni >= n || nj >= n || dist[nj][ni] >= 0) continue;
            int op = d == 0 ? open_e[bj][bi] : d == 2 ? open_e[nj][ni] : d == 1 ? open_s[bj][bi] : open_s[nj][ni];
            if (!op) continue;
            dist[nj][ni] = dist[bj][bi] + 1; q[qt++] = nj * 11 + ni;
        }
    }
    static uint8_t office[11][11];                                 /* Severance: which blocks are rooms, the rest narrow halls */
    if (sev) {                                                     /* white halls from block to block, turning corners; a few offices */
        for (int j = 0; j < n; j++)
            for (int i = 0; i < n; i++) office[j][i] = (i == 0 && j == j0) || j * 11 + i == far || (hash2(i, j) ^ sc->seed) % 100 < 22;
        for (int j = 0; j < n; j++)
            for (int i = 0; i < n; i++) {
                if (office[j][i]) { for (int y = BX(j); y < BX(j) + 3; y++) for (int x = BX(i); x < BX(i) + 3; x++) carve(x, y, 0, TH_CORRIDOR); }
                else carve(BX(i) + 1, BX(j) + 1, 0, TH_CORRIDOR);
                if (open_e[j][i]) {
                    for (int x = BX(i) + 1; x <= BX(i + 1) + 1; x++) carve(x, BX(j) + 1, 0, TH_CORRIDOR);
                    if (open_e[j][i] == 2 && office[j][i] && office[j][i + 1]) for (int k = 0; k < 3; k++) carve(BX(i) + 3, BX(j) + k, 0, TH_CORRIDOR);
                }
                if (open_s[j][i]) {
                    for (int y = BX(j) + 1; y <= BX(j + 1) + 1; y++) carve(BX(i) + 1, y, 0, TH_CORRIDOR);
                    if (open_s[j][i] == 2 && office[j][i] && office[j + 1][i]) for (int k = 0; k < 3; k++) carve(BX(i) + k, BX(j) + 3, 0, TH_CORRIDOR);
                }
            }
        for (int y = o; y < o + S; y++)                            /* a placard here and there, now and then a picture */
            for (int x = o; x < o + S; x++) {
                uint32_t h = hash2(x, y) ^ sc->seed;
                if (G.map[y][x].kind == K_WALL && !G.map[y][x].secret) G.map[y][x].tex = h % 100 < 5 ? T_WHITE_SIGN : h % 100 < 8 ? T_WHITE_ART : T_WHITE;
            }
    }
    PK.on_x = PK.on_y = -1;
    int gen = 0;
    uint32_t taken = PK.taken[PK.chain][clampi(link, 0, POCKET_LINKS - 1)];
    #define POCKET_ITEM(x, y, type) do { int g_ = gen++; Item *it_ = add_item(x, y, type);   /* made either way, so the dice fall alike */ \
        if (it_) { it_->secret = 1; it_->gen = g_ + 1; if (taken >> g_ & 1) it_->alive = 0; } } while (0)
    if (link < sc->links) {                                        /* the way on: in an outer wall, the farthest such block */
        int best = -1, bd = -1;
        for (int j = 0; j < n; j++)
            for (int i = 0; i < n; i++) {
                if (!(i == n - 1 || j == 0 || j == n - 1) || dist[j][i] < 0 || (i == 0 && j == j0)) continue;
                if (dist[j][i] > bd) { bd = dist[j][i]; best = j * 11 + i; }
            }
        int bi = best % 11, bj = best / 11;
        if (bi == n - 1) { PK.on_x = o + S - 1; PK.on_y = BX(bj) + 1; }
        else if (bj == 0) { PK.on_x = BX(bi) + 1; PK.on_y = o; }
        else { PK.on_x = BX(bi) + 1; PK.on_y = o + S - 1; }
        G.map[PK.on_y][PK.on_x].secret = 3; G.map[PK.on_y][PK.on_x].tex = (uint8_t)door_tex;
        if (sev) {                                                 /* a hall from the block's middle out to the door */
            int mx = BX(bi) + 1, my = BX(bj) + 1, sx = (mx > PK.on_x) - (mx < PK.on_x), sy = (my > PK.on_y) - (my < PK.on_y);
            for (int x = PK.on_x + sx, y = PK.on_y + sy; x != mx || y != my; x += sx, y += sy) carve(x, y, 0, TH_CORRIDOR);
        }
    } else {                                                       /* the chain's end: its reward, for now a hoard */
        int cx = BX(far % 11) + 1, cy = BX(far / 11) + 1, vd = -1;
        for (int d = 0; d < 4 && (back || sev) && vd < 0; d++) {   /* the Backrooms' end: a doorway onto doorways; Severance's, a water cooler */
            int wx = cx + DX[d] * 2, wy = cy + DY[d] * 2;
            if (G.map[wy][wx].kind != K_WALL || G.map[wy][wx].secret) continue;
            vd = d;
            if (back) { G.map[wy][wx].tex = T_VISPORT0; G.vis_x = wx; G.vis_y = wy; }
            else { G.vis_x = cx + DX[d]; G.vis_y = cy + DY[d]; solid_prop(G.vis_x, G.vis_y, PK.vis_used[PK.chain] ? SP_COOLER_LOW : SP_COOLER, 1, K_PROP); G.vis_deco = G.ndecos - 1; }
        }
        if (vd >= 0) {                                             /* the hoard to either side, out of its way */
            int qx = DY[vd] != 0, qy = DX[vd] != 0;
            POCKET_ITEM(cx + DX[vd] + qx, cy + DY[vd] + qy, IT_CHEST);
            POCKET_ITEM(cx + qx, cy + qy, IT_CROWN);
            POCKET_ITEM(cx - qx, cy - qy, IT_GEMS);
        } else {
            POCKET_ITEM(cx, cy, IT_CHEST);
            POCKET_ITEM(cx - 1, cy, IT_CROWN);
            POCKET_ITEM(cx + 1, cy, IT_GEMS);
            light_static(cx + 0.5f, cy + 0.5f, 1.4f, 5.0f);
        }
    }
    /* light here and there, and a few things lying about; in the Backrooms a tube panel over every block,
     * most lit, some failing, some dead */
    for (int j = 0; j < n; j++)
        for (int i = 0; i < n; i++) {
            int r = rnd(100), first = (i == 0 && j == j0) || (back && G.vis_x >= 0 && j * 11 + i == far);   /* the way in and the end, lit */
            if (sev) {                                             /* Severance: every panel lit, and steady */
                G.map[BX(j) + 1][BX(i) + 1].lamp = (uint8_t)(1 + G.nlights);
                light_static(BX(i) + 1.5f, BX(j) + 1.5f, 1.45f + 0.1f * rndf(), 6.0f);
                G.lights[G.nlights - 1].phase = -1000.0f;
            } else if (back) {
                Cell *lc = &G.map[BX(j) + 1][BX(i) + 1];
                if (!first && r >= 75) { lc->lamp = 255; continue; }
                lc->lamp = (uint8_t)(1 + G.nlights);
                light_static(BX(i) + 1.5f, BX(j) + 1.5f, 1.3f + 0.25f * rndf(), 5.0f);
                G.lights[G.nlights - 1].phase = (!first && r >= 60) ? -10.0f - 80.0f * rndf() : -1000.0f;
            } else if (first || r < 55) {
                light_static(BX(i) + 1.5f, BX(j) + 1.5f, 1.25f + 0.35f * rndf(), 5.0f);
                G.lights[G.nlights - 1].phase = -1000.0f;              /* steady: no flames here */
            }
        }
    static const int LOOT[5] = { IT_COINS, IT_COINS, IT_GEMS, IT_CHALICE, IT_CROWN };
    for (int k = 0; k < 2 + link + rnd(2); k++) {
        int i = rnd(n), j = rnd(n);
        if (i == 0 && j == j0) continue;
        POCKET_ITEM(BX(i) + 1, BX(j) + 1, LOOT[rnd(5)]);
    }
    for (int j = 0; j < n && sev; j++)                             /* in the offices, four desks in a corner (off the lanes), where */
        for (int i = 0; i < n; i++) {                              /* nothing lies already */
            uint32_t h = hash2(i, j) ^ sc->seed;
            int x = BX(i) + (int)(h & 1) * 2, y = BX(j) + (int)(h >> 1 & 1) * 2, clear = office[j][i] && !(i == 0 && j == j0) && j * 11 + i != far;
            for (int k = 0; k < G.nitems && clear; k++) clear = !(G.items[k].lx == x && G.items[k].ly == y);
            if (clear) solid_prop(x, y, SP_DESKS, 1, K_PROP);
        }
    if (back) {                                                    /* things from the crypt out of place, and some caught in a loop */
        static const int OUT[8] = { SP_URN, SP_SKULLPILE, SP_BONES1, SP_BONES2, SP_TOMB_ROUND, SP_TOMB_CROSS, SP_BROKEN_ARMS, SP_CANDLESTAND_A };
        static const int HOW[4] = { DF_FLIP, DF_FLOAT, DF_SINK, DF_FLIP | DF_SINK };   /* on the ceiling, floating, half sunk below or above */
        for (int k = 0; k < 1 + n * n / 6; k++) {
            int i = rnd(n), j = rnd(n), cx = BX(i) + rnd(3), cy = BX(j) + rnd(3), sp = OUT[rnd(8)], how = HOW[rnd(4)];
            if (i == 0 && j == j0) continue;
            add_deco_ex(cx + 0.5f, cy + 0.5f, sp, sp == SP_CANDLESTAND_A ? 2 : 1, how);
        }
        static const int LOOPER[5][3] = { { SP_SKEL_A, 1, 0 }, { SP_BAT_A, 1, DF_FLOAT }, { SP_SHAMB_A, 2, 0 }, { SP_SPIDER_A, 6, 0 }, { SP_GHOST_A, 1, 0 } };
        for (int k = 0; k < 1 + link / 2; k++) {                   /* the level's own kinds, harmless and untouchable */
            int i = rnd(n), j = rnd(n), w = rnd(5);
            if ((i == 0 && j == j0) || G.level < LOOPER[w][1]) continue;
            add_deco_ex(BX(i) + 1.5f, BX(j) + 1.5f, LOOPER[w][0], 2, DF_LOOP | LOOPER[w][2]);
        }
    }
    /* its creatures: the level's own kinds, away from the way in, a rank higher for each link past the
     * first (up to black), a few more in the Backrooms; the slain stay slain until you die in the chain */
    uint32_t slain = PK.slain[PK.chain][clampi(link, 0, POCKET_LINKS - 1)];
    int want = clampi((1 + n * n / 4) * (back ? 5 : 4) / 4, 2, 16), mg = 0;
    for (int k = 0, tries = 0; k < want && tries < 200; tries++) {
        int i = rnd(n), j = rnd(n), x = BX(i) + rnd(3), y = BX(j) + rnd(3), type = random_type();
        if (i + abs(j - j0) < 2 || G.map[y][x].kind != K_FLOOR || occupied(x, y, NULL)) continue;
        spawn_wanderer_type(x, y, type);
        k++;
        for (int q = 0; q < MAXMOBS; q++) {
            Mob *m = &G.mobs[q];
            if (!m->alive || m->gen) continue;
            m->gen = ++mg;
            m->rank0 = m->rank + 1;                                /* for its experience: the level's own rank */
            if (KINDS[m->type].family >= 0) set_rank(m, imin(m->rank + link - 1, imin(RK_V, KINDS[m->type].top)));
            m->hp = m->maxhp = m->maxhp * theme_boost();
            if (mg <= 32 && (slain >> (mg - 1) & 1)) { m->alive = 0; if (!KIND_IS(m, KF_UNCOUNTED)) G.kills_total--; }
        }
    }
    for (int i = 0; i < MAXMOBS && mind_on(); i++) {               /* in packs of a kind (section 19, step 8): those near each other */
        Mob *a = &G.mobs[i];
        int pt = a->type == MOB_SKEL || a->type == MOB_SHIELD ? PACK_FILE : a->type == MOB_BAT ? PACK_ROOST : a->type == MOB_HUNTER ? PACK_SPIDERS
               : a->type == MOB_IMP ? PACK_IMPS : 0;
        if (!a->alive || a->pack || !pt) continue;
        int near[4], nn = 0;
        for (int j = i + 1; j < MAXMOBS && nn < 4; j++) {
            Mob *b = &G.mobs[j];
            int bt = b->type == MOB_SKEL || b->type == MOB_SHIELD ? PACK_FILE : b->type == MOB_BAT ? PACK_ROOST : b->type == MOB_HUNTER ? PACK_SPIDERS : b->type == MOB_IMP ? PACK_IMPS : 0;
            if (b->alive && !b->pack && bt == pt && hypotf(b->x - a->x, b->y - a->y) < 6.0f) near[nn++] = j;
        }
        if (!nn) continue;
        int pk = pack_new(pt);
        if (pk < 0) break;
        pack_add(pk, a);
        for (int q = 0; q < nn; q++) pack_add(pk, &G.mobs[near[q]]);
    }
    #undef POCKET_ITEM
    #undef BX
    G.rooms[0] = (Room){ o + 1, o + 4 * j0 + 1, 3, 3, 0, TH_CORRIDOR, RT_HALL }; G.nrooms = 1;
    G.ncells = 0;
    for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) if (G.map[y][x].kind == K_FLOOR && lane(x, y)) G.ncells++;
    G.rs = keep_rs ^ (sc->seed + (uint32_t)link);
#ifdef CRAWL_DEBUG
    int alive = 0;
    for (int i = 0; i < G.nitems; i++) alive += G.items[i].alive;
    fprintf(stderr, "POCKET chain %d link %d of %d: %dx%d at %d, back (%d,%d) on (%d,%d), %d things, %d lights, %d of %d light texels\n", PK.chain, link, sc->links, S, S, o, PK.back_x, PK.back_y, PK.on_x, PK.on_y, alive, G.nlights, G.nlc, MAXLC);
#endif
}

/* stand in the cell before a doorway, facing away from it into the place */
static void stand_at_door(int dx, int dy) {
    for (int d = 0; d < 4; d++) {
        int fx = dx + DX[d], fy = dy + DY[d];
        if (fx < 0 || fy < 0 || fx >= MAPN || fy >= MAPN || G.map[fy][fx].kind != K_FLOOR) continue;
        G.cx = G.tx = fx; G.cy = G.ty = fy; G.px = fx + 0.5f; G.py = fy + 0.5f;
        G.h = G.next_h = d; G.ang = heading_angle(d);
        return;
    }
}

/* into a pocket of the chain: the place is built, the caster brought along */
static void collect(Item *it);
static void pocket_build(int link, int from_deeper) {
    G_KEEP = G;
    memset(&G, 0, sizeof G);
    carry_caster(&G_KEEP);
    G.level = G_KEEP.level; G.retries = G_KEEP.retries; G.blessed = 0;
    G.exit_fx = G.exit_fy = G.exit_wx = G.exit_wy = -1; G.exit_in_vault = -1;
    G.altar_x = G.altar_y = -1; G.altar_seek = -1; G.slide = G.seek_secret = -1;
    G.pz = G.pz_room = G.pz_door = -1; G.msg_deco = G.srch_deco = G.bsh_x = -1;   /* the level's puzzle stays on the level */
    G.tri = G.tri_room = G.tri_door = -1;                         /* and its trial */
    for (int k = 0; k < NKEYS; k++) G.vault_room[k] = G.vault_door[k] = -1;
    gen_pocket(&G_LEVEL.secrets[PK.chain], link);
    spectres_set();                                                /* a pocket's, as on its level, the first time in: */
    if (link <= PK.reached[PK.chain]) G.spec_n = 0;                /* the slain stay slain, the rest gone */
    crawl_art_theme(G.ptheme, G.level);
    snd_set_room(G.ptheme == PT_BACKROOMS ? 1 : G.ptheme == PT_SEVERANCE ? 2 : 0);   /* no music, a hum, an echo; Severance's muzak */
    PK.depth = link; PK.since = 0.0f; G.use_kind = 0;
    if (from_deeper) stand_at_door(PK.on_x, PK.on_y); else stand_at_door(PK.back_x, PK.back_y);
    G.map[G.cy][G.cx].visits = 1; G.explored = 1;
    map_carto();
    G.level_t = 1.0;                                               /* the pocket's own clock; the hand is free at once */
    G.dead = 0; G.ending = 0; G.leaving = G.exiting = 0;
    hunt_cx = -1;
    if (G.player) human_begin(); else { G.state = PS_MOVE; G.u = 1.0f; }
    snd_sfx_ex(SFX_WHOOSH, 0.9f, 0.5f); snd_sfx_ex(SFX_SHIMMER, 0.5f, 0.5f);
    pocket_say_found(link);
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_TAKEALL")) for (int i = 0; i < G.nitems; i++) if (G.items[i].alive) collect(&G.items[i]);   /* testing: what was taken stays taken */
#endif
}

/* back out to the level, before the chain's doorway. A death in there is no death: the dweller comes back
 * whole at the secret's entrance, facing in, and the chain's doorway is sealed for the rest of the level. */
static void pocket_return(int died) {
    G_KEEP = G;
    G = G_LEVEL;
    Secret *sc = &G.secrets[PK.chain];
    carry_caster(&G_KEEP);
    crawl_art_theme(PT_NONE, G.level);
    snd_set_room(0);
    PK.depth = 0; PK.since = 0.0f; PK.auto_back = 0; G.use_kind = 0;
    stand_at_door(sc->door_x, sc->door_y);
    G.camz = 0.5f; G.spd = 0.0f;
    if (died) {                                                    /* the trial failed: thrown out whole, the way in sealed */
        if (G.map[sc->fy][sc->fx].kind == K_FLOOR) {
            G.cx = G.tx = sc->fx; G.cy = G.ty = sc->fy; G.px = sc->fx + 0.5f; G.py = sc->fy + 0.5f;
            G.h = G.next_h = sc->d; G.ang = heading_angle(sc->d);
        }
        sc->sealed = 1; G.map[sc->door_y][sc->door_x].tex = T_SEALED;
        G.trials_failed++; PK.auto_done[PK.chain] = 1;
        G.hp = G.max_hp = gear_max_hp(&RUN.gear);
        G.poison_t = G.slow_t = 0.0f; G.hurt = 0.0f; G.face_t = 0.0f;
        memset(PK.slain[PK.chain], 0, sizeof PK.slain[PK.chain]);
        say("THE DWELLER FAILED THIS TRIAL");
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT pocket_thrown chain %d t=%.2f\n", PK.chain, G.t);
#endif
    }
    hunt_cx = -1;
    if (G.player) human_begin(); else { G.state = PS_MOVE; G.u = 1.0f; }
    snd_sfx_ex(SFX_WHOOSH, 0.8f, 0.5f);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT pocket_out chain %d level_t=%.2f hp=%d t=%.2f player=%d died=%d\n", PK.chain, G.level_t, G.hp, G.t, G.player, died);
#endif
}

/* walked into a doorway (secret 3) at (x, y): which way it goes */
static int pocket_door(int x, int y) {
    if (abs(x - G.cx) + abs(y - G.cy) != 1) return 0;
    return pocket_go(x, y);
}
static int pocket_go(int x, int y) {
    if (!PK.depth) {
        for (int i = 0; i < G.nsecrets; i++)
            if (G.secrets[i].door_x == x && G.secrets[i].door_y == y && G.secrets[i].state == 2) {
                if (G.secrets[i].sealed) {                         /* a failed trial: a knock on the seal, and no more */
                    if (G.knock_t <= 0.0f) { sfx_at(SFX_KNOCK, x + 0.5f, y + 0.5f, 0.8f); G.knock_t = 0.6f; }
                    return 0;
                }
#ifdef CRAWL_DEBUG
                fprintf(stderr, "EVENT pocket_in chain %d level_t=%.2f t=%.2f\n", i, G.level_t, G.t);
#endif
                thing_drop_home();                                 /* what you carried stays on the level */
                G_LEVEL = G; PK.chain = i;
                pocket_build(2, 0);
                return 1;
            }
        return 0;
    }
    if (x == PK.back_x && y == PK.back_y) {
        if (PK.depth <= 2) pocket_return(0); else pocket_build(PK.depth - 1, 1);
        return 1;
    }
    if (x == PK.on_x && y == PK.on_y) { pocket_build(PK.depth + 1, 0); return 1; }
    return 0;
}
static void pocket_reset_level(void) { memset(&PK, 0, sizeof PK); PK.level = G.level; snd_set_room(0); }

static void ledger_first(void);
static uint32_t new_run_id(uint32_t seed);
static void claim_take(void);
static struct { int fight, frail, place, menu; float fight_t, calm_t, cast_t; } MUS;   /* what the music is told (music_frame) */
static void crawl_enter(uint32_t seed) {
    crawl_art_init();
    memset(&MUS, 0, sizeof MUS);
    CON.open = 0; CON.slide = 0.0f; snd_hold(0);                   /* the console goes up; what a code did stays */
    ledger_first();                                                /* the first time: the page to start from */
    int resume = RUN.active;
    if (resume && !RUN.loaded && G.dead && G.dcost) {              /* a death that cost: it has already written level 1 */
        RUN.shop_next = 1; RUN.hp = gear_max_hp(&RUN.gear);
    } else if (resume && !RUN.loaded) {                            /* back from the other scene: where was it? */
        int won = G.tally || G.exiting;
        RUN.level = G.level + won; RUN.hp = G.hp > 0 ? G.hp : gear_max_hp(&RUN.gear);
        RUN.shop_next = won || (G.shop && !G.shop_bought);
        if (G.bchallenge && !G.shop) { RUN.level = RUN.chal_from; RUN.shop_next = 1; }   /* a challenge ends with the scene, */
        if (G.replay && RUN.replay_from && !G.shop) { RUN.level = RUN.replay_from; RUN.replay_from = 0; RUN.shop_next = 1; }   /* and a replay */
    }
    RUN.loaded = 0;
    memset(&G, 0, sizeof G);
    G.rs = seed | 1u;
    if (!resume) {
        memset(&RUN, 0, sizeof RUN);
        RUN.active = 1; RUN.level = 1; RUN.hp = 100; RUN.shop_next = 1;
        RUN.run_id = new_run_id(seed); RUN.peds = 2; RUN.repo_n = 2; RUN.atone = 1;
        gear_reset(&RUN.gear);
#ifdef CRAWL_DEBUG
        if (getenv("CRAWL_LEVEL")) RUN.level = atoi(getenv("CRAWL_LEVEL"));
        if (getenv("CRAWL_STAGE")) RUN.shop_next = 0;              /* straight into the level */
        debug_gear();
        if (RUN.cls) { caster_sync(); RUN.hp = gear_max_hp(&RUN.gear); }   /* a class given here starts whole at its own health (D5) */
        if (getenv("CRAWL_HASMAP")) RUN.has_map = 1;              /* the map, already bought */
        if (getenv("CRAWL_CORNER")) { RUN.corner = atoi(getenv("CRAWL_CORNER")) & ((1 << NCS) - 1); RUN.wind_run = crawl_run(RUN.level); }   /* cornerstones held, a bit each */
        if (getenv("CRAWL_LUCK")) RUN.steps[LN_LUCK] = (uint16_t)clampi((int)((atof(getenv("CRAWL_LUCK")) - LUCK_BASE) * 10.0f + 0.5f), 0, 1000);   /* Luck at this */
        if (getenv("CRAWL_LEARNT")) { memset(RUN.rec.el_kills, 5, sizeof RUN.rec.el_kills); memset(RUN.rec.harms, 3, sizeof RUN.rec.harms); }   /* every element and harm learnt on every kind */
#endif
    }
#ifdef CRAWL_DEBUG
    if (resume && getenv("CRAWL_ALIGN")) align_debug(getenv("CRAWL_ALIGN"));   /* over a loaded run too */
    if (resume && getenv("CRAWL_CONF")) conf_debug(getenv("CRAWL_CONF"));
    if (resume) aug_debug_set();
    if (resume && getenv("CRAWL_CLASS")) { RUN.cls = 0; if (atoi(getenv("CRAWL_CLASS")) > 0) class_take(atoi(getenv("CRAWL_CLASS"))); }
    if (resume && getenv("CRAWL_SOULS")) RUN.souls = atoi(getenv("CRAWL_SOULS"));
    if (resume && getenv("CRAWL_HASMAP")) RUN.has_map = 1;
    if (resume && getenv("CRAWL_LEARNT")) { memset(RUN.rec.el_kills, 5, sizeof RUN.rec.el_kills); memset(RUN.rec.harms, 3, sizeof RUN.rec.harms); }
    if (getenv("CRAWL_SEED")) G.rs = (uint32_t)atoi(getenv("CRAWL_SEED")) | 1u;
#endif
    G.level = RUN.level; G.hp = RUN.hp;
    if (RUN.shop_next) begin_shop(); else start_level();
    LEDGER.has_visit = 0;                                          /* a page's visit is for its own room only */
    if (LEDGER.claim_due) claim_take();                            /* a page opened in danger: before you can move */
}

static void collect(Item *it) {
#ifdef CRAWL_DEBUG
    if (foot_of(it->cx, it->cy)) fprintf(stderr, "EVENT part_taken %d by %s t=%.2f\n", it->type, G.player ? "a person" : "the dream", G.t);
#endif
    it->alive = 0;
    if (it->gen && PK.depth) PK.taken[PK.chain][clampi(PK.depth, 0, POCKET_LINKS - 1)] |= 1u << (it->gen - 1);   /* gone from the pocket for good */
    switch (it->type) {
    case IT_KEY:
        G.has_key[it->color] = 1; snd_sfx(SFX_KEYGET); G.gold = 1.0f; G.grin_t = 1.6f;
#ifdef CRAWL_DEBUG
        bfs_from(G.doors[G.vault_door[it->color]].x, G.doors[G.vault_door[it->color]].y);
        fprintf(stderr, "EVENT key%d t=%.2f path=%d\n", it->color, G.t, bfs_dist[G.cy][G.cx]);
#endif
        break;
    case IT_POTION:
        if (it->color == TRI_REWARD) {                             /* the trial's draught: whole again, the poison gone (1.0.0b18.1) */
            G.hp = G.max_hp; G.poison_t = 0.0f;
            say("THE TRIAL'S DRAUGHT: YOU ARE WHOLE");
            snd_sfx(SFX_POTION); snd_sfx_ex(SFX_SHIMMER, 0.5f, 0.5f);
#ifdef CRAWL_DEBUG
            fprintf(stderr, "EVENT draught by %s t=%.2f\n", G.player ? "player" : "autopilot", G.t);
#endif
            break;
        }
        G.hp = imin(G.max_hp, G.hp + potion_heal()); snd_sfx(SFX_POTION);
        break;
    case IT_SCRIP: {                                               /* a boss's: to the Hexed Totem's account at once */
        int b = clampi(it->color, 0, BOSS_REC - 1), n = (G.bstep >= 3 ? 2 : 1) + (G.corrupt != 0);   /* the bell's one more */
        RUN.scrip[b] += n; RUN.rec.boss[b].scrip = (uint16_t)imin(65535, RUN.rec.boss[b].scrip + n);
        toast(SP_SCRIP, -1, RK_I, 0, b < NBOSS ? BOSSES[b].short_name : "A BOSS", n > 2 ? "THREE OF ITS ODIOUS SCRIP" : n > 1 ? "TWO OF ITS ODIOUS SCRIP" : "ITS ODIOUS SCRIP");
        snd_sfx(SFX_KEYGET); G.gold = 1.0f; G.grin_t = 1.6f;
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT scrip %d +%d = %d t=%.2f\n", b, n, RUN.scrip[b], G.t);
#endif
        break;
    }
    case IT_MAP:                                                   /* the tutorial's (docs/PLAN-tutorial.md 7.4): yours for good */
        RUN.has_map = 1;
        snd_sfx(SFX_KEYGET); snd_sfx_ex(SFX_SHIMMER, 0.7f, 0.5f); G.gold = 1.0f; G.grin_t = 1.6f;
        if (G.player) say("THE MAP IS YOURS: M UNROLLS IT");
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT map_taken by %s t=%.2f\n", G.player ? "player" : "autopilot", G.t);
#endif
        if (g_drawn == 2) { ice_up(); tu_map_taken(); }           /* and the ice comes back behind you; the lessons show it */
        break;
    case IT_BONE:                                                  /* for the spear */
        if (RUN.gear.bones < bones_max()) RUN.gear.bones++;
        snd_sfx_ex(SFX_BONES, 0.5f, 0.5f);
        break;
    case IT_URN: {                                                 /* it shatters and the souls inside are yours */
        soul_add(5 + rnd(6));
        snd_sfx_ex(SFX_SHATTER, 0.8f, 0.5f); snd_sfx_ex(SFX_SHIMMER, 0.6f, 0.5f); G.chill = 0.5f;
        break;
    }
    case IT_RING: {                                                /* a ring, for nothing */
        Offer o;
        memset(&o, 0, sizeof o);
        o.kind = GK_PIECE; o.id = IK_RING;
        o.piece = piece_roll(o.id, G.level, -1, &G.rs); o.tier = G.level;
        take_piece(&o, 0);
        snd_sfx(SFX_KEYGET); G.gold = 1.0f; G.grin_t = 1.6f;
        break;
    }
    default: {
        int v = it->value ? it->value : treasure_value(it->type);   /* a lucky drop carries its own worth */
        int got = (int)(v * (1.0f + gear_total(&RUN.gear, ST_TREASURE) / 100.0f) * (G.corrupt ? 1.4f : 1.0f) + 0.5f);   /* Treasure Hunter, the
                                                                                                  caster's own, the bell's */
        RUN.treasure += got; G.level_treasure += got;
        if (!it->secret) { G.value_got += v; G.items_got++; }
        if (it->color == TRI_REWARD) {                             /* the trial's hoard (1.0.0b18.1) */
            say("THE TRIAL'S HOARD: $%d", got);
            snd_sfx(SFX_KEYGET);
#ifdef CRAWL_DEBUG
            fprintf(stderr, "EVENT trial_hoard %d by %s t=%.2f\n", got, G.player ? "player" : "autopilot", G.t);
#endif
        }
        snd_sfx_ex(SFX_COIN, it->type >= IT_CROWN ? 1.2f : 0.9f, 0.5f);
        G.gold = it->type >= IT_CROWN ? 1.0f : 0.6f;
        if (it->type >= IT_CROWN) G.grin_t = 1.6f;
        break;
    }
    }
    pop_readout();
}

