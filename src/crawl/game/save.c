/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * Saves: the gear, the record, the hall, the shrine and the stones packed and unpacked, a page's sections, run_pack, run_unpack, the progress, the mark, ledger_first. */

static void piece_pack(uint8_t *p, const Piece *pc) {
    p[0] = pc->kind; p[1] = pc->quality; p[2] = pc->level; p[3] = pc->anointed; p[4] = pc->ability; p[5] = pc->nstat;
    for (int i = 0; i < PIECE_STATS; i++) { p[6 + i] = pc->stat[i]; put16(p + 6 + PIECE_STATS + 2 * i, pc->val[i]); }
}
static Piece piece_unpack(const uint8_t *p) {                       /* anything out of range comes back as nothing */
    Piece pc;
    memset(&pc, 0, sizeof pc);
    if (p[0] >= NIK || p[1] >= NQUAL || p[4] >= NABIL || p[5] > PIECE_STATS) return pc;
    for (int i = 0; i < p[5]; i++) if (p[6 + i] >= NSTAT) return pc;
    pc.kind = p[0]; pc.quality = p[1]; pc.level = p[2]; pc.anointed = p[3] != 0; pc.ability = p[4]; pc.nstat = p[5];
    for (int i = 0; i < PIECE_STATS; i++) { pc.stat[i] = p[6 + i]; pc.val[i] = (int16_t)get16(p + 6 + PIECE_STATS + 2 * i); }
    return pc;
}
#define OFFER_BYTES (8 + PIECE_BYTES)
static void offer_pack(uint8_t *p, const Offer *o) {                /* a ware: what it is, its price, and a piece's make */
    p[0] = (uint8_t)o->kind; p[1] = (uint8_t)o->id; p[2] = (uint8_t)o->tier; p[3] = (uint8_t)o->rare;
    put32(p + 4, o->price);
    piece_pack(p + 8, &o->piece);
}
static int offer_unpack(const uint8_t *p, Offer *o) {               /* 0 if it is no ware the merchant sells */
    memset(o, 0, sizeof *o);
    o->kind = p[0]; o->id = p[1]; o->tier = p[2]; o->rare = p[3] != 0; o->price = get32(p + 4);
    o->piece = piece_unpack(p + 8);
    return !(o->kind > GK_PIECE || (o->kind == GK_ATK && o->id >= NATK) || (o->kind == GK_DEF && o->id >= NDEF) || o->price < 0
             || (o->kind == GK_PIECE && o->piece.kind == IK_NONE));
}
static void gear_pack(uint8_t *p, const Gear *g) {               /* the spells' bytes keep only the BFS9000's tier now */
    for (int i = 0; i < NATK; i++) *p++ = (uint8_t)(i == AT_BFS ? g->atk[i] : -1);
    for (int i = 0; i < NDEF; i++) *p++ = (uint8_t)g->def[i];
    for (int s = 0; s < NSLOT; s++, p += PIECE_BYTES) piece_pack(p, &g->worn[s]);
    *p++ = g->bag_size; *p++ = g->bag_quality;
    for (int i = 0; i < BAG_MAX; i++, p += PIECE_BYTES) piece_pack(p, &g->bag[i]);
    put16(p, g->bones); p += 2;
    *p++ = g->has_held;
    offer_pack(p, &g->held);
}
static void gear_unpack(const uint8_t *p, Gear *g, int by_quality) {   /* runes by quality from version 5, by tier before */
    static const int SLOT_IK[NSLOT] = { IK_WRIST, IK_WRIST, IK_BOOTS, IK_NECK, IK_HOOD, IK_ROBE, IK_RING };
    gear_reset(g);
    for (int i = 0; i < NATK; i++, p++) g->atk[i] = (int8_t)(i == AT_BFS ? clampi((int8_t)*p, -1, 15) : -1);   /* an old save's spells forgotten (O1) */
    for (int i = 0; i < NDEF; i++, p++) g->def[i] = (int8_t)(by_quality ? clampi((int8_t)*p, 0, RUNE_EXQ) : rune_from_tier(clampi((int8_t)*p, 0, 15)));
    for (int s = 0; s < NSLOT; s++, p += PIECE_BYTES) {
        g->worn[s] = piece_unpack(p);
        if (g->worn[s].kind != SLOT_IK[s]) memset(&g->worn[s], 0, sizeof g->worn[s]);   /* only what goes there */
    }
    g->bag_size = (uint8_t)clampi(p[0], 0, BAG_MAX); g->bag_quality = (uint8_t)clampi(p[1], 0, NQUAL - 1); p += 2;
    for (int i = 0; i < BAG_MAX; i++, p += PIECE_BYTES) if (i < g->bag_size) g->bag[i] = piece_unpack(p);
    g->bones = (int16_t)clampi(get16(p), 0, BONES_MAX + 15); p += 2;   /* the most any caster carries (1.0.0b11) */
    g->has_held = *p++ != 0;
    if (!offer_unpack(p, &g->held)) g->has_held = 0;
    if (!g->has_held) memset(&g->held, 0, sizeof g->held);
}
/* versions 1 and 2: the spellbook as it was; the robe, hood and best ring as Common pieces of their tier */
static void gear_from_old(const uint8_t *p, Gear *g, uint32_t seed) {
    static const int ROBE_STAT[6] = { -1, ST_ARMOR, ST_RES_POISON, ST_ARMOR, ST_ARMOR, ST_REGEN };   /* plain, silk, shadow, thorned, blessed */
    static const int HOOD_STAT[5] = { -1, ST_ARMOR, ST_ARMOR, ST_EFFECT, ST_HP };                   /* plain, warded, hunter's, cowl */
    uint32_t rs = seed | 1u;
    gear_reset(g);
    for (int i = 0; i < NATK; i++) g->atk[i] = (int8_t)(i == AT_BFS ? clampi((int8_t)p[i], -1, 15) : -1);
    for (int i = 0; i < NDEF; i++) g->def[i] = (int8_t)rune_from_tier(clampi((int8_t)p[NATK + i], 0, 15));
    int robe = (int8_t)p[18], robe_t = (int8_t)p[19], hood = (int8_t)p[20], hood_t = (int8_t)p[21];
    if (robe > 0 && robe < 6 && robe_t > 0) g->worn[SL_ROBE] = piece_make(IK_ROBE, robe_t, ROBE_STAT[robe], &rs);
    if (hood > 0 && hood < 5 && hood_t > 0) g->worn[SL_HOOD] = piece_make(IK_HOOD, hood_t, HOOD_STAT[hood], &rs);
    int best = 0;
    for (int f = 0; f < 4; f++) if ((int8_t)p[22 + f] >= 0 && (int8_t)p[26 + f] > best) best = (int8_t)p[26 + f];
    if (best > 0) g->worn[SL_RING] = piece_make(IK_RING, best, ST_DAMAGE, &rs);
}
/* the record, little-endian, field by field: the shape it has had since version 2, then (version 6)
 * the rest of the levels cleared and of the achievements */
static void rec_pack(uint8_t *p, const Record *r) {                /* version 7: the kills by spell left empty (LG_ELEM has them) */
    for (int k = 0; k < REC_KINDS; k++) for (int i = 0; i < NRANK; i++, p += 2) put16(p, r->kills[k][i]);
    memset(p, 0, REC_KINDS * REC_SPELLS); p += REC_KINDS * REC_SPELLS;
    memcpy(p, r->seen, sizeof r->seen); p += sizeof r->seen;
    put16(p, r->secrets); put16(p + 2, r->descents); put32(p + 4, (int32_t)(uint32_t)r->cleared); p += 8;
    memcpy(p, r->got, 256 / 8);
}
static void rec_unpack(const uint8_t *p, Record *r) {
    for (int k = 0; k < REC_KINDS; k++) for (int i = 0; i < NRANK; i++, p += 2) r->kills[k][i] = (uint16_t)get16(p);
    for (int k = 0; k < REC_KINDS; k++) {                          /* before version 7: by spell, into their elements, */
        int sum[NELEM] = { 0 }, knew = 0;                          /* and a spell learnt stays learnt */
        for (int sp = 0; sp < REC_SPELLS; sp++, p++) {
            int el = sp < NATK ? ATK_ELEM[sp] : -1;
            if (el < 0) continue;
            sum[el] += *p;
            if (*p >= 3) knew |= 1 << el;
        }
        for (int el = 0; el < NELEM; el++) if (sum[el]) r->el_kills[k][el] = (uint8_t)imin(255, imax(sum[el], knew >> el & 1 ? 5 : 0));
    }
    memcpy(r->seen, p, sizeof r->seen); p += sizeof r->seen;
    r->secrets = (uint16_t)get16(p); r->descents = (uint16_t)get16(p + 2); r->cleared = (uint32_t)get32(p + 4); p += 8;
    memcpy(r->got, p, 256 / 8);
}
static void rec_pack_extra(uint8_t *p, const Record *r) {
    put32(p, (int32_t)(uint32_t)(r->cleared >> 32)); p += 4;
    memcpy(p, r->got + 256 / 8, (ACH_BITS - 256) / 8); p += (ACH_BITS - 256) / 8;
    for (int b = 0; b < BOSS_REC; b++, p += 9) { p[0] = r->boss[b].met; put16(p + 1, r->boss[b].slain); put16(p + 3, r->boss[b].tries); put16(p + 5, r->boss[b].best); put16(p + 7, r->boss[b].scrip); }
}
static void rec_unpack_extra(const uint8_t *p, Record *r) {
    r->cleared |= (uint64_t)(uint32_t)get32(p) << 32; p += 4;
    memcpy(r->got + 256 / 8, p, (ACH_BITS - 256) / 8); p += (ACH_BITS - 256) / 8;
    for (int b = 0; b < BOSS_REC; b++, p += 9) { r->boss[b].met = p[0] != 0; r->boss[b].slain = (uint16_t)get16(p + 1); r->boss[b].tries = (uint16_t)get16(p + 3); r->boss[b].best = (uint16_t)get16(p + 5); r->boss[b].scrip = (uint16_t)get16(p + 7); }
}
/* version 6's hall, as far as it is built: the runs completed, each boss's scrip and challenge step, the
 * ranks met since the last death, the omens named, each boss ability's rank and the one readied */
#define SAVE_V6_HALL (2 + 2 * BOSS_REC + BOSS_REC + REC_KINDS + 1 + BOSS_REC + 1 + 2)
static void hall_pack(uint8_t *p) {
    put16(p, clampi(RUN.runs_done, 0, 65535)); p += 2;
    for (int b = 0; b < BOSS_REC; b++, p += 2) put16(p, clampi(RUN.scrip[b], 0, 65535));
    for (int b = 0; b < BOSS_REC; b++) *p++ = (uint8_t)clampi(RUN.bstep[b], 0, 5);
    memcpy(p, RUN.top_rank, REC_KINDS); p += REC_KINDS;
    *p++ = (uint8_t)RUN.omens;
    for (int b = 0; b < BOSS_REC; b++) *p++ = (uint8_t)clampi(RUN.abil[b], 0, ABIL_TOP);
    *p++ = (uint8_t)clampi(RUN.abil_sel, 0, BOSS_REC - 1);
    put16(p, clampi(RUN.run_top, 0, 999));
}
static void hall_unpack(const uint8_t *p) {
    RUN.runs_done = get16(p); p += 2;
    for (int b = 0; b < BOSS_REC; b++, p += 2) RUN.scrip[b] = get16(p);
    for (int b = 0; b < BOSS_REC; b++) RUN.bstep[b] = clampi(*p++, 0, 5);
    for (int k = 0; k < REC_KINDS; k++) RUN.top_rank[k] = (uint8_t)clampi(p[k], 0, NRANK);
    p += REC_KINDS;
    RUN.omens = *p++ & ((1 << NHZ) - 1);
    for (int b = 0; b < BOSS_REC; b++) RUN.abil[b] = (uint8_t)(b < NBOSS ? clampi(*p, 0, ABIL_TOP) : 0), p++;
    RUN.abil_sel = clampi(*p++, 0, NBOSS - 1);
    RUN.run_top = clampi(get16(p), 0, 999);
}
/* before version 6 a descent was twenty levels: the 40th, 80th ... stay as the descents they are now
 * (the 40th as level 40 cleared), and the rest are let go */
static void rec_from_old(Record *r) {
    uint8_t old[19];
    for (int k = 2; k <= MAX_DESCENT; k++) { int b = ACH_DESCENT0 + k - 2; old[k - 2] = r->got[b >> 3] >> (b & 7) & 1; r->got[b >> 3] &= (uint8_t)~(1 << (b & 7)); }
    for (int k = 2; k <= MAX_DESCENT; k++) {
        if (!old[k - 2] || (k * 20) % DESCENT_LEN) continue;
        int nk = k * 20 / DESCENT_LEN, b = nk == 1 ? ach_level(DESCENT_LEN) : ACH_DESCENT0 + nk - 2;
        r->got[b >> 3] |= (uint8_t)(1 << (b & 7));
    }
    r->descents /= 2;
}
#define SAVE_V4_EXTRA 4                     /* version 4: the wisp L casts, and the toggle */
#define SAVE_V5_SHRINE 32                   /* version 5: Cursed Coins and the Rune Shrine */
#define SAVE_V6_EXT (REC_EXTRA + SAVE_V6_HALL)   /* version 6: runs of ten, the bosses (docs/PLAN-b7.md) */
/* Version 7 (1.0.0b11, docs/PLAN-b11.md) keeps version 6's shape: what changed goes in sections (LG_ELEM: the
 * record by element), and the kills by spell in the fixed part are left empty */
#define SAVE_VERSION 7
static int save_version(const uint8_t *buf, int len) {             /* 1 to 7 if it is a save we can read, else 0 */
    if (len < SAVE_V1 || (uint32_t)get32(buf) != SAVE_MAGIC) return 0;
    int v = get32(buf + 4), v5 = SAVE_V3 + REC_BYTES + SAVE_V4_EXTRA + SAVE_V5_SHRINE;
    if (v == 1 || (v == 2 && len >= SAVE_V1 + REC_BYTES) || (v == 3 && len >= SAVE_V3 + REC_BYTES)
        || (v == 4 && len >= SAVE_V3 + REC_BYTES + SAVE_V4_EXTRA) || (v == 5 && len >= v5) || ((v == 6 || v == 7) && len >= v5 + SAVE_V6_EXT)) return v;
    return 0;
}
static int save_end(int v) {                                       /* a grave's time follows */
    return (v >= 3 ? SAVE_V3 : SAVE_V1) + (v >= 2 ? REC_BYTES : 0) + (v >= 4 ? SAVE_V4_EXTRA : 0) + (v >= 5 ? SAVE_V5_SHRINE : 0) + (v >= 6 ? SAVE_V6_EXT : 0);
}
/* the shrine's 32 bytes: coins (u16), rank, attunements (u16, a bit a rune), then the slots as rune
 * kind + 1 and quality, then the level whose coins were last taken (u16) and which; the rest spare */
static void shrine_pack(uint8_t *p) {
    memset(p, 0, SAVE_V5_SHRINE);
    put16(p, clampi(RUN.coins, 0, 65535)); p[2] = (uint8_t)RUN.shrine; put16(p + 3, RUN.attuned);
    for (int i = 0; i < SHRINE_SLOTS; i++) { p[5 + 2 * i] = RUN.slot_def[i]; p[6 + 2 * i] = RUN.slot_q[i]; }
    put16(p + 23, clampi(RUN.coin_level, 0, 65535)); p[25] = (uint8_t)RUN.coin_bits;   /* the coins taken on that level */
}
static void shrine_unpack(const uint8_t *p) {                      /* anything out of range comes back empty */
    RUN.coins = get16(p); RUN.shrine = clampi(p[2], 0, NQUAL); RUN.attuned = get16(p + 3) & ((1 << NDEF) - 1);
    for (int i = 0; i < SHRINE_SLOTS; i++) {
        int d = p[5 + 2 * i], q = p[6 + 2 * i];
        if (d < 1 || d > NDEF || q < 1 || q > RUNE_EXQ) d = q = 0;
        RUN.slot_def[i] = (uint8_t)d; RUN.slot_q[i] = (uint8_t)q;
    }
    RUN.coin_level = get16(p + 23); RUN.coin_bits = p[25];
}

/* A version 6 page goes on past the fixed part in sections, behind the marker "LDG1": a tag, a length
 * (u16) and that many bytes, ending at a tag of 0. A reader takes what it knows and steps over the rest.
 * LG_PAGE: whose run (u32), its signatures (u32), when it was written (u64) and why (PG_*). LG_VISIT: the
 * merchant's visit it was signed in, its level (u16), pedestals, wares left and bought, the shrine's
 * takes, the ware held this visit (+1), then each pedestal's ware, gone or not and back or not.
 * LG_STONES: the grave plot, a count, then each stone (STONE_BYTES): when (u64), the level (u16), what
 * took it, gold, souls (u32 each), coins (u16), the spells' tiers and runes' qualities, the worn pieces,
 * the bag's size and quality and its pieces. LG_PEDS: the pedestals bought (a byte). LG_COMP: the Corpus
 * Compendium, a count for each kind (bracer to bag) and quality, the pieces in that order, then each
 * spell's tier. LG_REPO: the Repository, its places (a byte), gold (u32) and coins (u16), then each place.
 * LG_CODEX: what the Codex knows, four u32 of bits: abilities, runes, runes at Exquisite, stats; then (1.0.0b11)
 * a u32 of the stats past the 32nd, and one of what else has been met (CXM_*).
 * LG_CASTER (1.0.0b9, docs/PLAN-b9.md): a byte of flags (1 the map), u16 the Level of Atonement, u32 the
 * experience, u16 the points not spent, a u16 of steps bought in each line, a byte of overlevels chosen in each,
 * a byte of cornerstones, a byte for Second Wind spent, u16 the run it last came back on. Twelve lines before 1.0.0b11,
 * thirteen since (Resist Spirit's): a reader takes its count from the length.
 * LG_ELEM (version 7, docs/PLAN-b11.md): the record by element, a byte each by kind and element: kills, hits
 * landed, harms felt. LG_ALIGN (version 7): the Alignment, the points in each line's nodes (a byte each, by element
 * and node), then u16 the Alignment Points not spent. A page without one is older: its spells are forgotten and it
 * gets a point for every Level of Atonement it holds. */
#define LG_CASTER_BYTES (1 + 2 + 4 + 2 + NLINE * 2 + NLINE + 1 + 1 + 2)
#define LEDGER_MAGIC 0x3147444Cu            /* "LDG1" */
#define PROGRESS_MAGIC 0x31505648u          /* "HVP1": a progress that names the page to start from */
enum { LG_END, LG_PAGE, LG_VISIT, LG_STONES, LG_PEDS, LG_COMP, LG_REPO, LG_CODEX, LG_CASTER, LG_ELEM, LG_ALIGN, LG_HALL2, LG_DEEDS, LG_WAYS,
       LG_CONF,                                                    /* 1.0.0b20: the Confessions' points and overlevels, a byte each, and
                                                                      the old overlevels whose Alignment Points were given */
       LG_AUG,                                                     /* 1.0.0b21: each line's augments, AUG_LINE_BYTES a line */
       LG_PORTRAIT };                                              /* the portraits of the slain (docs/PLAN-portraits.md), below */
/* LG_PORTRAIT: a byte of how her room pays this life (POR_*), u32 the tapestries drawn back, then for each kind the run each gem
 * was unlocked in (1 + its set, a byte each), each kind's gems taken this life (a byte, a bit a gem), each boss's (a byte, a bit a
 * deed); then (step 5) each kind's own count, u16 (RUN.por_feat). A page without one is older: each rank it unlocked is shown in
 * the first run that rank is common in, its tapestries are down where its kills are counted, and all its gold waits. */
#define LG_PORTRAIT_B4 (1 + 4 + REC_KINDS * POR_GEMS + REC_KINDS + BOSS_REC)   /* as step 4 wrote it */
#define LG_PORTRAIT_BYTES (LG_PORTRAIT_B4 + 2 * REC_KINDS)
#define AUG_LINE_BYTES (NAUGT * 4 + 2)      /* a line's augments: three choices, their order, their credits (u16), its bank (u16) */
#define LG_WAYS_BYTES 20                    /* 1.0.0b18: the dead's ways met and named this run of ten (u32 each), that run (u16), the five
                                               deeds against them (u16 each) */
#define LG_DEEDS_BYTES 18                   /* 1.0.0b13 (docs/PLAN-b13.md): every hazard met, a bit each (the hall keeps only eight);
                                               puzzles solved, trials won and lost (u16 each); the kinds of puzzle solved, of trial won */
#define LG_HALL2_BYTES 12                   /* the bell and the font cursed, the levels they wait for, the secrets, a replay */
#define LG_ALIGN_B11 (NELEM * NNODE + 2 + 4)     /* the nodes, the points; the class, the Witchfinder's points and those spent, the gift openings */
#define LG_ALIGN_BYTES (LG_ALIGN_B11 + 2)         /* 1.0.0b16: the Lampstand's gifts taken, and the lines they opened */
enum { PG_SIGNED, PG_DEATH, PG_CLAIM };
#define LG_PAGE_BYTES 17
#define LG_VISIT_BYTES (8 + NPED * (2 + OFFER_BYTES))
#define STONE_BYTES (8 + 2 + 3 + 4 + 4 + 2 + NATK + NDEF + NSLOT * PIECE_BYTES + 2 + BAG_MAX * PIECE_BYTES)
#define COMP_BYTES ((NIK - 1) * NQUAL * (1 + COMP_MAX * PIECE_BYTES) + NATK)   /* at the most */
#define REPO_BYTES (1 + 4 + 2 + REPO_MAX * PIECE_BYTES)
#define SAVE_END6 (SAVE_V3 + REC_BYTES + SAVE_V4_EXTRA + SAVE_V5_SHRINE + SAVE_V6_EXT)   /* save_end(6) */
#define PAGE_BYTES (SAVE_END6 + 4 + 3 + LG_PAGE_BYTES + 3 + LG_VISIT_BYTES + 3 + 1 + STONES * STONE_BYTES + 3 + 1 + 3 + COMP_BYTES + 3 + REPO_BYTES \
                    + 3 + 24 + 3 + LG_CASTER_BYTES + 3 + REC_ELEM_BYTES + 3 + LG_ALIGN_BYTES + 3 + LG_HALL2_BYTES + 3 + LG_DEEDS_BYTES + 3 + LG_WAYS_BYTES \
                    + 3 + 2 * NCONF + 1 + 3 + NELEM * AUG_LINE_BYTES + 3 + LG_PORTRAIT_BYTES + 1)   /* the most a page takes so far */
_Static_assert(PAGE_BYTES <= SLOT_MAX, "a page fits its slot");
static void stone_pack(uint8_t *p, const Stone *st) {
    uint64_t w = (uint64_t)st->when;
    put32(p, (int32_t)(uint32_t)(w & 0xffffffffu)); put32(p + 4, (int32_t)(uint32_t)(w >> 32)); put16(p + 8, st->level);
    p[10] = st->by.what; p[11] = st->by.type; p[12] = st->by.rank;
    put32(p + 13, st->gold); put32(p + 17, st->souls); put16(p + 21, clampi(st->coins, 0, 65535)); p += 23;
    for (int k = 0; k < NATK; k++) {                               /* since the Alignment: a line's rank, its path in the top bits; */
        int e = ATK_ELEM[k];                                       /* the class where Hellfire's tier was, marked */
        if (st->lined && k == AT_HELL) *p++ = (uint8_t)(0x80 | clampi(st->cls, 0, NELEM));
        else if (st->lined && e >= 0 && LINE_ATK[e] == k && st->atk[k] >= 0) *p++ = (uint8_t)(clampi(st->atk[k], 0, 63) | (st->path[e] & 3) << 6);
        else *p++ = (uint8_t)st->atk[k];
    }
    for (int k = 0; k < NDEF; k++) *p++ = (uint8_t)st->def[k];
    for (int i = 0; i < NSLOT; i++, p += PIECE_BYTES) piece_pack(p, &st->worn[i]);
    *p++ = st->bag_size; *p++ = st->bag_quality;
    for (int i = 0; i < BAG_MAX; i++, p += PIECE_BYTES) piece_pack(p, &st->bag[i]);
}
static void stone_unpack(const uint8_t *p, Stone *st) {
    memset(st, 0, sizeof *st);
    st->when = (int64_t)((uint64_t)(uint32_t)get32(p) | (uint64_t)(uint32_t)get32(p + 4) << 32); st->level = clampi(get16(p + 8), 1, 999);
    st->by = (Blame){ p[10], p[11], p[12] };
    st->gold = imax(0, get32(p + 13)); st->souls = imax(0, get32(p + 17)); st->coins = get16(p + 21); p += 23;
    st->lined = (p[AT_HELL] & 0xF8) == 0x80;                       /* written since the Alignment */
    st->cls = st->lined ? p[AT_HELL] & 7 : 0;
    for (int k = 0; k < NATK; k++, p++) {
        int e = ATK_ELEM[k];
        if (st->lined && k == AT_HELL) st->atk[k] = -1;
        else if (st->lined && e >= 0 && LINE_ATK[e] == k && *p != 0xFF) { st->atk[k] = (int8_t)(*p & 63); st->path[e] = (uint8_t)(*p >> 6 & 3); }
        else st->atk[k] = (int8_t)clampi((int8_t)*p, -1, 15);
    }
    for (int k = 0; k < NDEF; k++) st->def[k] = (int8_t)clampi((int8_t)*p++, 0, RUNE_EXQ);
    for (int i = 0; i < NSLOT; i++, p += PIECE_BYTES) st->worn[i] = piece_unpack(p);
    st->bag_size = (uint8_t)clampi(p[0], 0, BAG_MAX); st->bag_quality = (uint8_t)clampi(p[1], 0, NQUAL - 1); p += 2;
    for (int i = 0; i < BAG_MAX; i++, p += PIECE_BYTES) st->bag[i] = i < st->bag_size ? piece_unpack(p) : (Piece){ 0 };
}
static const uint8_t *page_section(const uint8_t *buf, int len, int tag, int *n) {
    int at = save_end(6);
    if (save_version(buf, len) < 6 || len < at + 4 || (uint32_t)get32(buf + at) != LEDGER_MAGIC) return NULL;
    for (at += 4; at + 3 <= len && buf[at] != LG_END; at += 3 + get16(buf + at + 1)) {
        int k = get16(buf + at + 1);
        if (at + 3 + k > len) return NULL;
        if (buf[at] == tag) { *n = k; return buf + at + 3; }
    }
    return NULL;
}
static uint8_t *section_begin(uint8_t *p, int tag) { p[0] = (uint8_t)tag; return p + 3; }
static uint8_t *section_end(uint8_t *at, uint8_t *end) { put16(at - 2, (int)(end - at)); return end; }

static int run_pack(uint8_t *buf, int cap, int why) {
    int n = save_end(6);
    if (cap < PAGE_BYTES || CON.used) return 0;                    /* the dead took note: nothing is kept */
    int level = G.level, hp = G.dead || G.hp <= 0 ? gear_max_hp(&RUN.gear) : G.hp, gold = RUN.treasure, souls = RUN.souls, shop = 0;
    if (why == PG_DEATH) { level = RUN.level; shop = 1; hp = gear_max_hp(&RUN.gear); }   /* woken by the merchant, before level 1 */
    else if (G.bchallenge && !G.shop) { level = RUN.chal_from; shop = 1; }   /* a challenge: back to the room it was begun from */
    else if (G.replay && RUN.replay_from && !G.shop) { level = RUN.replay_from; shop = 1; }   /* and so a replay */
    else if (G.tally || G.exiting) { level = G.level + 1; shop = 1; }   /* the level is won */
    else if (G.shop) shop = 1;                                     /* her room, with the visit as it stands */
    else { gold -= G.level_treasure; souls -= G.level_souls; }
    put32(buf, (int32_t)SAVE_MAGIC);
    put32(buf + 4, SAVE_VERSION);
    put32(buf + 8, level); put32(buf + 12, hp); put32(buf + 16, gold); put32(buf + 24, shop);
    put32(buf + 20, souls * 10 + clampi((int)(RUN.soul_frac * 10.0f), 0, 9));   /* spirit, in tenths */
    gear_pack(buf + 28, &RUN.gear);
    rec_pack(buf + SAVE_V3, &RUN.rec);
    uint8_t *x = buf + SAVE_V3 + REC_BYTES;
    x[0] = (uint8_t)RUN.wisp_kind; x[1] = (uint8_t)RUN.wisp_color; x[2] = (uint8_t)(RUN.wisp_auto != 0); x[3] = 0;
    shrine_pack(x + SAVE_V4_EXTRA);
    rec_pack_extra(x + SAVE_V4_EXTRA + SAVE_V5_SHRINE, &RUN.rec);
    hall_pack(x + SAVE_V4_EXTRA + SAVE_V5_SHRINE + REC_EXTRA);
    uint8_t *p = buf + n, *c;
    put32(p, (int32_t)LEDGER_MAGIC); p += 4;
    c = section_begin(p, LG_PAGE);
    uint64_t now = (uint64_t)(int64_t)time(NULL);
    put32(c, (int32_t)RUN.run_id); put32(c + 4, (int32_t)RUN.signs);
    put32(c + 8, (int32_t)(uint32_t)(now & 0xffffffffu)); put32(c + 12, (int32_t)(uint32_t)(now >> 32)); c[16] = (uint8_t)why;
    p = section_end(c, c + LG_PAGE_BYTES);
    c = section_begin(p, LG_PEDS);
    c[0] = (uint8_t)clampi(RUN.peds, 2, NPED);
    p = section_end(c, c + 1);
    c = section_begin(p, LG_COMP);
    uint8_t *q = c;
    for (int k = 1; k < NIK; k++) for (int ql = 0; ql < NQUAL; ql++) *q++ = RUN.ncomp[k][ql];
    for (int k = 1; k < NIK; k++) for (int ql = 0; ql < NQUAL; ql++) for (int i = 0; i < RUN.ncomp[k][ql]; i++, q += PIECE_BYTES) piece_pack(q, &RUN.comp[k][ql][i]);
    for (int k = 0; k < NATK; k++) *q++ = 0;                       /* its spells, kept until 1.0.0b11 */
    p = section_end(c, q);
    c = section_begin(p, LG_REPO);
    c[0] = (uint8_t)clampi(RUN.repo_n, 2, REPO_MAX); put32(c + 1, imax(0, RUN.repo_gold)); put16(c + 5, clampi(RUN.repo_coins, 0, 65535));
    q = c + 7;
    for (int i = 0; i < c[0]; i++, q += PIECE_BYTES) piece_pack(q, &RUN.repo[i]);
    p = section_end(c, q);
    c = section_begin(p, LG_CASTER);
    {
        uint8_t *q = c;
        *q++ = (uint8_t)((RUN.has_map ? 1 : 0) | (RUN.lantern_off ? 2 : 0));   /* 2: the lantern shuttered (1.0.0b15) */
        put16(q, clampi(RUN.atone, 1, 65535)); q += 2;
        put32(q, imax(0, RUN.xp)); q += 4;
        put16(q, clampi(RUN.points, 0, 65535)); q += 2;
        for (int l = 0; l < NLINE; l++, q += 2) put16(q, RUN.steps[l]);
        for (int l = 0; l < NLINE; l++) *q++ = RUN.over[l];
        *q++ = (uint8_t)RUN.corner; *q++ = (uint8_t)(RUN.wind_spent != 0);
        put16(q, clampi(RUN.wind_run, 0, 65535)); q += 2;
        p = section_end(c, q);
    }
    c = section_begin(p, LG_CONF);
    memcpy(c, RUN.conf, NCONF); memcpy(c + NCONF, RUN.conf_over, NCONF); c[2 * NCONF] = (uint8_t)clampi(RUN.ap_overs, 0, 255);
    p = section_end(c, c + 2 * NCONF + 1);
    c = section_begin(p, LG_AUG);                                  /* 1.0.0b21: a line at a time, its three choices, their order, their
                                                                      credits (u16), its bank (u16) */
    for (int e = 0; e < NELEM; e++) {
        uint8_t *q = c + e * AUG_LINE_BYTES;
        for (int t = 0; t < NAUGT; t++) { q[t] = RUN.aug[e][t]; q[NAUGT + t] = RUN.aug_ord[e][t]; put16(q + 2 * NAUGT + 2 * t, RUN.aug_cr[e][t]); }
        put16(q + 4 * NAUGT, RUN.aug_bank[e]);
    }
    p = section_end(c, c + NELEM * AUG_LINE_BYTES);
    c = section_begin(p, LG_ALIGN);
    memcpy(c, RUN.al, sizeof RUN.al); put16(c + NELEM * NNODE, clampi(RUN.al_points, 0, 65535));
    c[NELEM * NNODE + 2] = (uint8_t)RUN.cls; c[NELEM * NNODE + 3] = (uint8_t)clampi(RUN.cls_pts, 0, 255);
    c[NELEM * NNODE + 4] = (uint8_t)clampi(RUN.cls_spent, 0, 255); c[NELEM * NNODE + 5] = RUN.al_gift;
    c[LG_ALIGN_B11] = (uint8_t)clampi(RUN.gifts, 0, 255); c[LG_ALIGN_B11 + 1] = RUN.gifted;
    p = section_end(c, c + LG_ALIGN_BYTES);
    c = section_begin(p, LG_HALL2);                                /* the Dead Bell and the Weeping Font (1.0.0b11, step 12) */
    memset(c, 0, LG_HALL2_BYTES);
    c[0] = (uint8_t)(RUN.bell_cursed != 0); c[1] = (uint8_t)(RUN.font_cursed != 0); put16(c + 2, clampi(RUN.bell_next, 0, 65535));
    c[4] = (uint8_t)(RUN.bell_secrets != 0); put16(c + 5, clampi(RUN.font_next, 0, 65535)); put16(c + 7, clampi(RUN.replay_from, 0, 65535));
    p = section_end(c, c + LG_HALL2_BYTES);
    c = section_begin(p, LG_DEEDS);                                /* the runs' hazards met, the puzzles and trials (1.0.0b13) */
    put32(c, RUN.omens);
    put16(c + 4, RUN.deed_pz); put16(c + 6, RUN.deed_tri); put16(c + 8, RUN.deed_lost); put32(c + 10, RUN.pz_kinds); put32(c + 14, RUN.tri_kinds);
    p = section_end(c, c + LG_DEEDS_BYTES);
    c = section_begin(p, LG_WAYS);                                 /* the dead's ways (1.0.0b18) */
    put32(c, (int32_t)RUN.ways); put32(c + 4, (int32_t)RUN.way_said); put16(c + 8, clampi(RUN.way_run, 0, 65535));
    put16(c + 10, RUN.deed_amb); put16(c + 12, RUN.deed_shaken); put16(c + 14, RUN.deed_heads); put16(c + 16, RUN.deed_hush); put16(c + 18, RUN.deed_file);
    p = section_end(c, c + LG_WAYS_BYTES);
    c = section_begin(p, LG_PORTRAIT);                             /* the portraits of the slain */
    c[0] = (uint8_t)RUN.por_open; put32(c + 1, (int32_t)RUN.por_drawn);
    memcpy(c + 5, RUN.por_run, sizeof RUN.por_run); memcpy(c + 5 + sizeof RUN.por_run, RUN.por_taken, sizeof RUN.por_taken);
    memcpy(c + 5 + sizeof RUN.por_run + sizeof RUN.por_taken, RUN.por_btaken, sizeof RUN.por_btaken);
    for (int kd = 0; kd < REC_KINDS; kd++) put16(c + LG_PORTRAIT_B4 + 2 * kd, RUN.por_feat[kd]);
    p = section_end(c, c + LG_PORTRAIT_BYTES);
    c = section_begin(p, LG_ELEM);
    memcpy(c, RUN.rec.el_kills, sizeof RUN.rec.el_kills); memcpy(c + REC_KINDS * NELEM, RUN.rec.el_hits, sizeof RUN.rec.el_hits);
    memcpy(c + 2 * REC_KINDS * NELEM, RUN.rec.harms, sizeof RUN.rec.harms);
    p = section_end(c, c + REC_ELEM_BYTES);
    c = section_begin(p, LG_CODEX);
    put32(c, (int32_t)RUN.cx_abil); put32(c + 4, (int32_t)RUN.cx_rune); put32(c + 8, (int32_t)RUN.cx_exq); put32(c + 12, (int32_t)(uint32_t)RUN.cx_stat);
    put32(c + 16, (int32_t)(uint32_t)(RUN.cx_stat >> 32));         /* the stats past the 32nd (1.0.0b11) */
    put32(c + 20, (int32_t)RUN.cx_more);                           /* and what else has been met */
    p = section_end(c, c + 24);
    if (RUN.nstones) {
        c = section_begin(p, LG_STONES);
        c[0] = (uint8_t)RUN.nstones;
        for (int i = 0; i < RUN.nstones; i++) stone_pack(c + 1 + i * STONE_BYTES, &RUN.stones[i]);
        p = section_end(c, c + 1 + RUN.nstones * STONE_BYTES);
    }
    if (G.shop && why != PG_DEATH) {
        c = section_begin(p, LG_VISIT);
        put16(c, G.level); c[2] = (uint8_t)G.ped_n; c[3] = (uint8_t)clampi(G.shop_left, 0, 255); c[4] = (uint8_t)clampi(G.shop_bought, 0, 255);
        c[5] = (uint8_t)clampi(G.sh_takes, 0, 255); c[6] = (uint8_t)(G.shop_held + 1); c[7] = 0;
        uint8_t *q = c + 8;
        for (int i = 0; i < G.ped_n; i++, q += 2 + OFFER_BYTES) { q[0] = (uint8_t)G.offer_gone[i]; q[1] = (uint8_t)G.offer_back[i]; offer_pack(q + 2, &G.offers[i]); }
        p = section_end(c, q);
    }
    *p++ = LG_END;
    return (int)(p - buf);
}

static uint32_t new_run_id(uint32_t seed) {
    uint32_t id = (uint32_t)time(NULL) * 2654435761u ^ seed * 40503u;
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_RUNID")) id = (uint32_t)strtoul(getenv("CRAWL_RUNID"), NULL, 0);   /* the same run again, for a fault found */
    fprintf(stderr, "RUNID %u\n", id ? id : 1u);
#endif
    return id ? id : 1u;
}

static void run_unpack(const uint8_t *buf, int len) {
    int v = save_version(buf, len), k;
    if (!v) return;
    memset(&RUN, 0, sizeof RUN);
    RUN.active = 1; RUN.loaded = 1;
    if (v >= 2) rec_unpack(buf + (v >= 3 ? SAVE_V3 : SAVE_V1), &RUN.rec);
    TOASTS.n = 0;                                                  /* another run's news is no news */
    RUN.level = clampi(get32(buf + 8), 1, 999); RUN.treasure = get32(buf + 16); RUN.souls = get32(buf + 20);
    RUN.shop_next = get32(buf + 24) != 0;
    if (v >= 3) { gear_unpack(buf + 28, &RUN.gear, v >= 5); RUN.soul_frac = (float)(RUN.souls % 10) / 10.0f; RUN.souls /= 10; }
    else gear_from_old(buf + 28, &RUN.gear, (uint32_t)RUN.level * 2654435761u);
    RUN.hp = clampi(get32(buf + 12), 1, gear_max_hp(&RUN.gear));
    if (v >= 4) {
        const uint8_t *x = buf + SAVE_V3 + REC_BYTES;
        RUN.wisp_kind = clampi(x[0], WK_EXIT, WK_ALTAR); RUN.wisp_color = clampi(x[1], 0, NKEYS - 1); RUN.wisp_auto = x[2] != 0;
        if (v >= 5) shrine_unpack(x + SAVE_V4_EXTRA);
        if (v >= 6) { rec_unpack_extra(x + SAVE_V4_EXTRA + SAVE_V5_SHRINE, &RUN.rec); hall_unpack(x + SAVE_V4_EXTRA + SAVE_V5_SHRINE + REC_EXTRA); }
    }
    if (v < 6) rec_from_old(&RUN.rec);
    if (RUN.gear.has_held && RUN.gear.held.kind == GK_DEF) {        /* the merchant sells no runes now: the hold is paid back */
        RUN.treasure += (RUN.gear.held.price + 3) / 4;
        RUN.gear.has_held = 0; memset(&RUN.gear.held, 0, sizeof RUN.gear.held);
    }
    if (RUN.treasure < 0) RUN.treasure = 0;
    if (RUN.souls < 0 || RUN.soul_frac < 0.0f) { RUN.souls = RUN.souls < 0 ? 0 : RUN.souls; RUN.soul_frac = 0.0f; }
    const uint8_t *c = page_section(buf, len, LG_PAGE, &k);
    if (c && k >= LG_PAGE_BYTES) { RUN.run_id = (uint32_t)get32(c); RUN.signs = (uint32_t)get32(c + 4); }
    if (!RUN.run_id) {                                             /* an old save: a run of its own, the same each time it is read */
        uint32_t h = 2166136261u;
        for (int i = 0; i < len; i++) h = (h ^ buf[i]) * 16777619u;
        RUN.run_id = h ? h : 1u;
    }
    c = page_section(buf, len, LG_PEDS, &k);
    RUN.peds = c && k >= 1 ? clampi(c[0], 2, NPED) : gear_shop_pedestals(RUN.level);   /* an old save keeps what its level gave it */
    c = page_section(buf, len, LG_COMP, &k);
    if (c && k >= (NIK - 1) * NQUAL + NATK) {                      /* the Compendium; anything that doesn't fit comes back empty */
        const uint8_t *q = c, *end = c + k;
        int need = (NIK - 1) * NQUAL + NATK;
        for (int kd = 1; kd < NIK; kd++) for (int ql = 0; ql < NQUAL; ql++) { RUN.ncomp[kd][ql] = (uint8_t)imin(*q++, COMP_MAX); need += RUN.ncomp[kd][ql] * PIECE_BYTES; }
        if (need > k) memset(RUN.ncomp, 0, sizeof RUN.ncomp);
        for (int kd = 1; kd < NIK; kd++) for (int ql = 0; ql < NQUAL; ql++) {
            int m = 0;
            for (int i = 0; i < RUN.ncomp[kd][ql]; i++, q += PIECE_BYTES) {
                Piece pc = piece_unpack(q);
                if (pc.kind == kd && pc.quality == ql) RUN.comp[kd][ql][m++] = pc;   /* only what belongs on its page */
            }
            RUN.ncomp[kd][ql] = (uint8_t)m;
        }
        (void)end;                                                 /* then its spells, which it keeps no more */
    } else comp_note(RUN.level);                                   /* an old save: what it wears and knows goes in as it loads */
    c = page_section(buf, len, LG_ELEM, &k);
    if (c && k >= REC_ELEM_BYTES) {                                /* version 7: the record by element */
        memcpy(RUN.rec.el_kills, c, sizeof RUN.rec.el_kills); memcpy(RUN.rec.el_hits, c + REC_KINDS * NELEM, sizeof RUN.rec.el_hits);
        memcpy(RUN.rec.harms, c + 2 * REC_KINDS * NELEM, sizeof RUN.rec.harms);
    }
    c = page_section(buf, len, LG_ALIGN, &k);
    if (c && k >= NELEM * NNODE + 2) {                             /* version 7: the Alignment */
        for (int e = 0; e < NELEM; e++) for (int n = 0; n < NNODE; n++) RUN.al[e][n] = (uint8_t)imin(c[e * NNODE + n], ALIGN[e].n[n].cost);
        RUN.al_points = get16(c + NELEM * NNODE);
        if (k >= LG_ALIGN_BYTES) { RUN.gifts = c[LG_ALIGN_B11]; RUN.gifted = c[LG_ALIGN_B11 + 1] & ((1u << NELEM) - 1); }   /* 1.0.0b16 */
        if (k >= LG_ALIGN_B11) {                                   /* and the class (step 10); before it, asked at the next visit */
            RUN.cls = c[NELEM * NNODE + 2] <= NELEM ? c[NELEM * NNODE + 2] : 0;
            RUN.cls_pts = c[NELEM * NNODE + 3]; RUN.cls_spent = c[NELEM * NNODE + 4]; RUN.al_gift = c[NELEM * NNODE + 5] & ((1u << NELEM) - 1);
        }
        RUN.old_spells = 0;
        int fire_more = 0;                                         /* before 1.0.0b16 Fire was open to all, its opening never paid: */
        for (int n = ND_EFF; n < NNODE; n++) fire_more |= RUN.al[EL_FIRE][n] != 0;   /* what was bought in it stays, as a gift */
        if (fire_more && RUN.cls != 1 + EL_FIRE && RUN.al[EL_FIRE][ND_OPEN] < ALIGN[EL_FIRE].n[ND_OPEN].cost) {
            RUN.al[EL_FIRE][ND_OPEN] = (uint8_t)ALIGN[EL_FIRE].n[ND_OPEN].cost; RUN.al_gift |= (uint8_t)(1u << EL_FIRE);
        }
    } else RUN.old_spells = 1;                                     /* older: points for its Levels of Atonement, below */
    c = page_section(buf, len, LG_DEEDS, &k);
    if (c && k >= 4) RUN.omens |= get32(c) & ((1 << NHZ) - 1);
    if (c && k >= 18) {
        RUN.deed_pz = get16(c + 4); RUN.deed_tri = get16(c + 6); RUN.deed_lost = get16(c + 8);
        RUN.pz_kinds = get32(c + 10) & ((1u << NPZK) - 1); RUN.tri_kinds = get32(c + 14) & ((1u << NTRI) - 1);
    }
    c = page_section(buf, len, LG_WAYS, &k);
    if (c && k >= LG_WAYS_BYTES) {
        RUN.ways = (uint32_t)get32(c) & ((1u << NWAY) - 1); RUN.way_said = (uint32_t)get32(c + 4) & ((1u << NWAY) - 1); RUN.way_run = get16(c + 8);
        RUN.deed_amb = get16(c + 10); RUN.deed_shaken = get16(c + 12); RUN.deed_heads = get16(c + 14); RUN.deed_hush = get16(c + 16); RUN.deed_file = get16(c + 18);
    }
    c = page_section(buf, len, LG_HALL2, &k);
    if (c && k >= 7) {
        RUN.bell_cursed = c[0] != 0; RUN.font_cursed = c[1] != 0; RUN.bell_next = get16(c + 2);
        RUN.bell_secrets = c[4] != 0; RUN.font_next = get16(c + 5);
        if (k >= 9) RUN.replay_from = get16(c + 7);
    }
    c = page_section(buf, len, LG_CODEX, &k);
    if (c && k >= 16) {
        RUN.cx_abil = (uint32_t)get32(c) & ((1u << NABIL) - 2); RUN.cx_rune = (uint32_t)get32(c + 4) & ((1u << NDEF) - 1);
        RUN.cx_exq = (uint32_t)get32(c + 8) & ((1u << NDEF) - 1); RUN.cx_stat = (uint64_t)(uint32_t)get32(c + 12);
        if (k >= 20) RUN.cx_stat |= (uint64_t)(uint32_t)get32(c + 16) << 32;
        if (k >= 24) RUN.cx_more = (uint32_t)get32(c + 20);
        RUN.cx_stat &= (1ull << NSTAT) - 1;
    }
    c = page_section(buf, len, LG_CASTER, &k);
    RUN.atone = 1;                                                 /* an older page: a fresh caster, and no map */
    int nl = c && k >= LG_CASTER_BYTES - 3 * (NLINE - NLINE_B9) ? imin(NLINE, (k - (LG_CASTER_BYTES - 3 * NLINE)) / 3) : 0;   /* the lines, by its length */
    if (nl) {
        const uint8_t *q = c;
        RUN.has_map = *q & 1; RUN.lantern_off = *q++ >> 1 & 1;
        RUN.atone = clampi(get16(q), 1, 9999); q += 2;
        RUN.xp = imax(0, get32(q)); q += 4;
        RUN.points = get16(q); q += 2;
        for (int l = 0; l < nl; l++, q += 2) RUN.steps[l] = (uint16_t)get16(q);
        for (int l = 0; l < nl; l++) RUN.over[l] = *q++;
        RUN.corner = *q++; RUN.wind_spent = *q++ != 0;
        RUN.wind_run = get16(q);
    }
    c = page_section(buf, len, LG_AUG, &k);                        /* 1.0.0b21: the augments; an older page has none */
    for (int e = 0; e < NELEM; e++) aug_forget(e);
    if (c && k >= NELEM * AUG_LINE_BYTES)
        for (int e = 0; e < NELEM; e++) {
            const uint8_t *q = c + e * AUG_LINE_BYTES;
            for (int t = 0; t < NAUGT; t++) {
                RUN.aug[e][t] = q[t] <= 2 ? q[t] : 0; RUN.aug_ord[e][t] = q[NAUGT + t];
                RUN.aug_cr[e][t] = (uint16_t)imin(get16(q + 2 * NAUGT + 2 * t), AUG_CR[2]);
            }
            RUN.aug_bank[e] = get16(q + 4 * NAUGT);
        }
    c = page_section(buf, len, LG_CONF, &k);                       /* 1.0.0b20 */
    memset(RUN.conf, 0, sizeof RUN.conf); memset(RUN.conf_over, 0, sizeof RUN.conf_over); RUN.ap_overs = 0;
    if (c && k >= 2 * NCONF + 1) {
        for (int cf = 0; cf < NCONF; cf++) { RUN.conf[cf] = (uint8_t)imin(c[cf], CONF_TOP); RUN.conf_over[cf] = c[NCONF + cf]; }
        RUN.ap_overs = c[2 * NCONF];
    }
    lines_payback();                                               /* an older page's lines come back as points */
    if (RUN.old_spells) RUN.al_points = imax(0, RUN.atone - 1);    /* 1.0.0b10 and before: a point for every Level of Atonement (O1) */
    c = page_section(buf, len, LG_REPO, &k);
    RUN.repo_n = 2;
    if (c && k >= 7) {
        RUN.repo_n = clampi(c[0], 2, REPO_MAX); RUN.repo_gold = imax(0, get32(c + 1)); RUN.repo_coins = get16(c + 5);
        for (int i = 0; i < RUN.repo_n && 7 + (i + 1) * PIECE_BYTES <= k; i++) RUN.repo[i] = piece_unpack(c + 7 + i * PIECE_BYTES);
    }
    c = page_section(buf, len, LG_STONES, &k);
    if (c && k >= 1) {
        RUN.nstones = imin(imin(c[0], STONES), (k - 1) / STONE_BYTES);
        for (int i = 0; i < RUN.nstones; i++) stone_unpack(c + 1 + i * STONE_BYTES, &RUN.stones[i]);
    }
    LEDGER.has_visit = 0;
    c = page_section(buf, len, LG_VISIT, &k);
    if (c && k >= 8 && RUN.shop_next) {                            /* the merchant's visit, as it was when signed */
        Visit *vs = &LEDGER.visit;
        memset(vs, 0, sizeof *vs);
        vs->level = get16(c); vs->ped_n = clampi(c[2], 0, NPED); vs->left = c[3]; vs->bought = c[4]; vs->takes = c[5]; vs->held = c[6] - 1;
        vs->ped_n = imin(vs->ped_n, (k - 8) / (2 + OFFER_BYTES));
        const uint8_t *q = c + 8;
        int ok = vs->level == RUN.level;
        for (int i = 0; i < vs->ped_n; i++, q += 2 + OFFER_BYTES) { vs->gone[i] = q[0] != 0; vs->back[i] = q[1] != 0; ok &= offer_unpack(q + 2, &vs->offers[i]); }
        if (vs->held >= vs->ped_n || !RUN.gear.has_held) vs->held = -1;
        LEDGER.has_visit = ok;
    }
    c = page_section(buf, len, LG_PORTRAIT, &k);                   /* the portraits of the slain */
    if (c && k >= LG_PORTRAIT_B4) {
        RUN.por_open = c[0] & (POR_CLEARED | POR_WOKEN); RUN.por_drawn = (uint32_t)get32(c + 1) & (((1u << NKINDS) - 1) | (((1u << NBOSS) - 1) << POR_BOSSBIT));
        if (k >= LG_PORTRAIT_BYTES) for (int kd = 0; kd < REC_KINDS; kd++) RUN.por_feat[kd] = (uint16_t)get16(c + LG_PORTRAIT_B4 + 2 * kd);
        memcpy(RUN.por_run, c + 5, sizeof RUN.por_run); memcpy(RUN.por_taken, c + 5 + sizeof RUN.por_run, sizeof RUN.por_taken);
        memcpy(RUN.por_btaken, c + 5 + sizeof RUN.por_run + sizeof RUN.por_taken, sizeof RUN.por_btaken);
        for (int kd = 0; kd < REC_KINDS; kd++) for (int r = 0; r < POR_GEMS; r++) if (RUN.por_run[kd][r] > 4) RUN.por_run[kd][r] = 0;
    } else {                                                       /* older: shown where its ranks are common, the tapestries down where it
                                                                      has slain, every rank's gold waiting */
        RUN.por_open = POR_WOKEN;
        for (int kd = 0; kd < NKINDS; kd++) for (int r = 0; r < NRANK; r++) if (RUN.rec.kills[kd][r]) RUN.por_drawn |= 1u << kd;
    }
    por_fill();
    slayer_sweep();                                                /* the counts lowered since it was written */
#ifdef CRAWL_DEBUG
    por_trace(c ? "read" : "read, older");
#endif
}

/* the next start opens this page */
static void progress_point(void) {
    if (CON.used || LEDGER.page < 1) return;
    uint8_t p[13];
    put32(p, (int32_t)PROGRESS_MAGIC); p[4] = (uint8_t)LEDGER.page; put32(p + 5, (int32_t)RUN.run_id); put32(p + 9, (int32_t)RUN.signs);
    slot_write(SLOT_PROGRESS, p, sizeof p);
}

/* Her claim (docs/PLAN-b7.md, section 2). The mark, beside the pages: for each page (0 the unsigned run)
 * whether it is marked, and whose run and which signature it was marked at. Written the moment a person's
 * run comes into danger, cleared 8 s after the danger has passed; a page opened while its mark still
 * fits pays her claim. */
#define MARK_MAGIC 0x314D5648u              /* "HVM1" */
#define MARK_BYTES (4 + (SLOT_COUNT + 1) * 9)
static void mark_read(uint8_t *m) {
    if (slot_read(SLOT_MARK, m, MARK_BYTES) != MARK_BYTES || (uint32_t)get32(m) != MARK_MAGIC) { memset(m, 0, MARK_BYTES); put32(m, (int32_t)MARK_MAGIC); }
}
static void mark_write(int on) {
    uint8_t m[MARK_BYTES], *e = NULL;
    mark_read(m);
    e = m + 4 + clampi(LEDGER.page, 0, SLOT_COUNT) * 9;
    e[0] = (uint8_t)on; put32(e + 1, (int32_t)RUN.run_id); put32(e + 5, (int32_t)RUN.signs);
    slot_write(SLOT_MARK, m, MARK_BYTES);
    LEDGER.marked = on;
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT mark %s page %d run %08x signs %u t=%.2f\n", on ? "on" : "off", LEDGER.page, RUN.run_id, RUN.signs, G.t);
#endif
}
static int mark_fits(void) {                                       /* the run just opened was marked at this very signature */
    uint8_t m[MARK_BYTES];
    mark_read(m);
    const uint8_t *e = m + 4 + clampi(LEDGER.page, 0, SLOT_COUNT) * 9;
    return e[0] && (uint32_t)get32(e + 1) == RUN.run_id && (uint32_t)get32(e + 5) == RUN.signs;
}

/* the first time the crypt is entered: the progress names the page to start from, or holds a run that was
 * never signed (1.0.0b6's, or one written at a death before any signing) */
static void ledger_first(void) {
    static uint8_t buf[SLOT_MAX];
    if (LEDGER.read) return;
    LEDGER.read = 1;
    int n = slot_read(SLOT_PROGRESS, buf, sizeof buf);
    if (n >= 5 && (uint32_t)get32(buf) == PROGRESS_MAGIC) {
        int pg = buf[4];
        if (pg < 1 || pg > SLOT_COUNT) return;
        n = slot_read(pg - 1, buf, sizeof buf);
        if (!save_version(buf, n)) return;
        run_unpack(buf, n); LEDGER.page = pg;
    } else run_unpack(buf, n);
    LEDGER.claim_due = RUN.active && !CON.used && mark_fits();
    LEDGER.kept = RUN.active;
#ifdef CRAWL_DEBUG
    if (RUN.active) fprintf(stderr, "LEDGER start from %s%s: level %d gold %d run %08x signs %u\n", LEDGER.page ? "page " : "the unsigned run",
                            LEDGER.page ? RUN_ROMAN[LEDGER.page - 1] : "", RUN.level, RUN.treasure, RUN.run_id, RUN.signs);
#endif
}

