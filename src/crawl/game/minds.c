/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * How the dead think: senses and memory, packs (calls, turns, flanks, morale), roles and rounds, hunters on the trail, the dead's ways named and their deeds. */

/* ---- How the dead think (docs/PLAN-ai.md, Part A: senses and memory). A creature looks where it walks and sees
 * about a third of the way round, the rest only close by; out of the light it sees less far (the shade the other way
 * about); a noise has a place, and what hears it goes to look there; seen, it hunts with the live field for three
 * seconds after it loses you, then goes to where you were, a few cells on, and searches round there; a search that
 * finds nothing leaves it calm, and wary a while. The old minds stay behind CRAWL_MIND=0 in the debug build. ---- */
#define MIND_LIVE 3.0f                      /* S4 (b): the live field this long after sight is lost */
static int mind_on(void) {
#ifdef CRAWL_DEBUG
    if (g_mind < 0) g_mind = getenv("CRAWL_MIND") ? atoi(getenv("CRAWL_MIND")) != 0 : 1;
    return g_mind;
#else
    return 1;
#endif
}
static int minded(const Mob *m) {                                  /* who thinks this way: the swarm, the stone, the bosses don't */
    if (!mind_on() || !m->alive || m->state != MS_WALK || m->boss || m->add || m->ally || m->tri) return 0;
    switch (m->type) {
    case MOB_GHOST: case MOB_WSKULL: case MOB_SPIDER: case MOB_WRAITH: case MOB_SAC: case MOB_LING: return 0;
    default: return 1;
    }
}
/* Cunning by rank (docs/PLAN-ai.md, section 13; step 7): each rank keeps what those below it do. Green falls back to its
 * pack when alone and hurt, and a thrower of it steps behind a corner between throws; blue sees a slow spell coming and
 * steps aside, keeps out of hazards, and won't walk into a doorway you hold; red flanks and spreads out against a splash;
 * black feints and reads a charge. The gilded only run, as they always have. CRAWL_CUNNING=r: every creature as rank r. */
static int cunning(const Mob *m) {
#ifdef CRAWL_DEBUG
    static int forced = -2;
    if (forced == -2) forced = getenv("CRAWL_CUNNING") ? clampi(atoi(getenv("CRAWL_CUNNING")) - 1, RK_I, RK_V) : -1;
    if (forced >= 0) return forced;
#endif
    return m->rank == RK_GILDED || m->wild ? RK_I : m->rank;       /* a hound whose huntsman fell: careless */
}
static int has_facing(const Mob *m) { return m->type != MOB_BAT && m->type != MOB_LIGHTLESS; }   /* bats hear their way round, all about
                                                                      them; the Lightless feels light, and has no eyes to turn */
static float turn_to(float a, float target, float rate) { return a + clampf(wrap_pi(target - a), -rate, rate); }
static int caster_dark(void) { return G.lit_here < 0.33f && G.flare_t <= 0.0f; }
static float sight_reach(const Mob *m) {                           /* S2: the dark shortens sight, but not a bat's or a hound's */
    int dark = caster_dark();
    if (m->type == MOB_SHADE) return dark ? 9.0f : 5.0f;           /* the dark is the shade's */
    if (m->type == MOB_BAT || m->type == MOB_HOUND) return 9.0f;
    return dark ? 5.0f : 9.0f;
}
static int in_view(const Mob *m, float px, float py) {             /* within the third of the way round it is looking */
    if (!has_facing(m)) return 1;
    return fabsf(wrap_pi(atan2f(py - m->y, px - m->x) - m->face - m->look)) < 1.05f;
}
static int mind_sees(const Mob *m, float pd) {
    if (G.dead || G.wwalk_t > 0.0f) return 0;
    float reach = sight_reach(m);
    if (!in_view(m, G.px, G.py)) reach = fminf(reach, m->type == MOB_HOUND ? 4.0f : 2.5f);   /* behind it: felt, or smelt */
    return pd < reach && line_clear(m->x, m->y, G.px, G.py);
}

/* steps to a place, for going to look: a few kept, each good for a second */
#define NMF 6
#define MF_FAR 160                              /* the whole level: a hunt starts far off (step 5) */
static struct { int x, y, key; double t; int16_t f[MAPN][MAPN]; } MF[NMF];
static int mf_key(void) { return G.level * 64 + PK.depth * 2 + G.shop; }
static int16_t (*mind_field(int tx, int ty))[MAPN] {
    int old = 0, key = mf_key();
    for (int i = 0; i < NMF; i++) {
        if (MF[i].key == key && MF[i].x == tx && MF[i].y == ty && G.t >= MF[i].t && G.t - MF[i].t < 1.0) return MF[i].f;
        if (MF[i].t < MF[old].t || MF[i].key != key) old = i;
    }
    static int q[MAPN * MAPN];
    int16_t (*f)[MAPN] = MF[old].f;
    MF[old].x = tx; MF[old].y = ty; MF[old].key = key; MF[old].t = G.t;
    memset(f, 0xff, sizeof MF[old].f);
    if (tx < 1 || ty < 1 || tx >= MAPN - 1 || ty >= MAPN - 1) return f;
    int qh = 0, qt = 0;
    f[ty][tx] = 0; q[qt++] = ty * MAPN + tx;
    while (qh < qt) {
        int x = q[qh] % MAPN, y = q[qh] / MAPN; qh++;
        if (f[y][x] >= MF_FAR) continue;
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (nx < 1 || ny < 1 || nx >= MAPN - 1 || ny >= MAPN - 1 || f[ny][nx] >= 0 || !hunt_pass(nx, ny)) continue;
            f[ny][nx] = (int16_t)(f[y][x] + 1); q[qt++] = ny * MAPN + nx;
        }
    }
    return f;
}

static void mind_tell(Mob *m) {                                    /* a wary creature's sound: you know it heard */
    int sfx = SFX_RATTLE;                                          /* the Order's dead: a head turned, the jaw's click */
    switch (m->type) {
    case MOB_HOUND: sfx = SFX_SNUFF; break;                        /* it scents the air */
    case MOB_GHOUL: case MOB_SHAMBLER: case MOB_DROWNED: sfx = SFX_GROAN; break;
    case MOB_HUNTER: case MOB_BROOD: sfx = SFX_SKITTER; break;
    case MOB_BAT: sfx = SFX_BAT; break;
    case MOB_SHADE: sfx = SFX_WHISPER; break;
    case MOB_IMP: case MOB_GARGOYLE: sfx = SFX_SHATTER; break;
    case MOB_SEXTON: sfx = SFX_GROAN; break;
    case MOB_LIGHTLESS: sfx_at(SFX_GLASS, m->x, m->y, 0.25f); return;   /* it makes no sound but its glass */
    default: break;
    }
    sfx_at(sfx, m->x, m->y, 0.45f);
}
static void mind_wary(Mob *m, float x, float y) {                  /* it heard something there */
    if (m->aware == MA_CALM) { mind_tell(m); DBG_COUNT(wary); }
    m->aware = MA_WARY; m->aw_t = 0.0f; m->kx = x; m->ky = y;
}
static void mind_track(Mob *m, float x, float y) {                 /* where you were, or a sound of you since */
    m->aware = MA_TRACK; m->aw_t = 0.0f; m->kx = x; m->ky = y;
}

/* ---- Packs (docs/PLAN-ai.md, sections 5-6; step 3). Kinds that live together: what one of them sees, all of it knows
 * after its call (cut short if the caller dies first); its melee members take turns, one at a time (two from rank III),
 * the rest circling just out of reach; when its leader falls or half of it is down it breaks, by its kind: the Order's
 * dead fight on, the living scatter (and the scavengers), hounds run for help, the buried don't notice. The packs: the file of the
 * Order (from 5), the hounds (from 8), the roost (from 11), the wake (from 13). ---- */
#ifdef CRAWL_DEBUG
static const char *PACK_NAME[] = { "none", "file", "hounds", "roost", "wake", "scavengers", "spiders", "imps", "sentinels", "band", "huntsman" };
#endif
static Pack *pack_of(const Mob *m) { return m->pack > 0 && m->pack <= G.npacks ? &G.packs[m->pack - 1] : NULL; }
static int pack_new(int kind) {
    if (!mind_on() || G.npacks >= MAXPACKS) return -1;
    Pack *p = &G.packs[G.npacks];
    memset(p, 0, sizeof *p);
    p->kind = kind; p->leader = p->caller = p->last = -1; p->on[0] = p->on[1] = -1;
    return G.npacks++;
}
static void pack_join(int pk, Mob *m) {                           /* a pack has a rank: its leader's, the rest that or one under */
    if (pk < 0 || !m) return;
    Pack *p = &G.packs[pk];
    int i = (int)(m - G.mobs);
    m->pack = pk + 1; p->mem |= 1ull << i; p->n0++;
    if (p->leader < 0) { p->leader = i; return; }
    const Mob *L = &G.mobs[p->leader];
    int top = KINDS[m->type].top < RK_GILDED ? KINDS[m->type].top : RK_V;
    if (L->rank < RK_GILDED) set_rank(m, clampi(L->rank - rnd(2), RK_I, top));
}
static void pack_add(int pk, Mob *m) {                             /* joined as it is, its rank kept (a pocket's) */
    if (pk < 0 || !m) return;
    Pack *p = &G.packs[pk];
    int i = (int)(m - G.mobs);
    m->pack = pk + 1; p->mem |= 1ull << i; p->n0++;
    if (p->leader < 0) p->leader = i;
}
static void pack_tell(const Pack *p, float x, float y) {          /* its kind's call, as it sets on you */
    switch (p->kind) {
    case PACK_IMPS: sfx_at(SFX_IGNITE, x, y, 0.8f); sfx_at(SFX_EXPLODE, x, y, 0.35f); break;   /* hissing, a pop of embers */
    case PACK_SENTINELS: sfx_at(SFX_DIRT, x, y, 0.9f); break;
    case PACK_BAND: {
        int lt = p->leader >= 0 ? G.mobs[p->leader].type : MOB_SKEL;
        if (lt == MOB_KNIGHT) { sfx_at(SFX_SWORD, x, y, 0.8f); sfx_at(SFX_HOWL, x, y, 0.9f); }
        else if (lt == MOB_DEACON) { sfx_at(SFX_WHISPER, x, y, 1.0f); sfx_at(SFX_MOAN, x, y, 0.6f); }   /* a whispered chant */
        else if (lt == MOB_BROOD) sfx_at(SFX_SKITTER, x, y, 1.1f);
        else sfx_at(SFX_DRUM, x, y, 1.0f);
        break;
    }
    case PACK_FILE: sfx_at(SFX_DRUM, x, y, 1.0f); break;
    case PACK_HOUNDS: sfx_at(SFX_HOWL, x, y, 1.1f); break;
    case PACK_WHIP: {                                              /* a hound gives tongue, and his horn answers */
        const Mob *L = p->leader >= 0 ? &G.mobs[p->leader] : NULL;
        if (L && G.mobs[p->caller >= 0 ? p->caller : p->leader].type == MOB_HOUND) sfx_at(SFX_HOWL, x, y, 0.8f);
        if (L && L->alive) { sfx_at(SFX_HORN, L->x, L->y, 1.2f); G.mobs[p->leader].feat |= FT_HORN; }
        break;
    }
    case PACK_SCAV: sfx_at(SFX_GROAN, x, y, 1.0f); sfx_at(SFX_BONES, x, y, 0.6f); break;
    case PACK_SPIDERS: sfx_at(SFX_SKITTER, x, y, 1.1f); break;
    case PACK_ROOST: sfx_at(SFX_RUSTLE, x, y, 0.8f); sfx_at(SFX_BAT, x, y, 0.7f); break;
    default: break;
    }
}
static void pack_call(Mob *m) {                                    /* one of it has you: it calls the rest */
    Pack *p = pack_of(m);
    if (!p || p->broken || p->chill_t > 0.0f) return;              /* the Wraith's wail heard: it keeps to its posts */
    p->seen_t = (float)G.t; p->kx = G.px; p->ky = G.py;
    if (p->state != PAS_CALM || p->kind == PACK_WAKE) return;
    p->state = PAS_CALL; p->call_t = 0.4f; p->caller = (int)(m - G.mobs);
    dream_glance(m->x, m->y);                                      /* a call behind the dream: it looks round */
    if (p->kind == PACK_WHIP && p->leader >= 0 && G.mobs[p->leader].alive && G.mobs[p->leader].state == MS_WALK) {   /* the horn: */
        p->call_t = 1.1f; G.mobs[p->leader].call_t = 1.1f; DBG_COUNT(horns);   /* his hounds wait on it */
    }
    pack_tell(p, m->x, m->y);
    way_met(pack_way(p));
    DBG_COUNT(pack_calls);
}
static void pack_hunt(Pack *p) {                                   /* the call heard: all of it knows where you are */
    if (!p->hunted) { p->hunted = 1; p->hurts0 = G.hurts; }
    p->state = PAS_HUNT; p->kx = G.px; p->ky = G.py; p->seen_t = (float)G.t;
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *o = &G.mobs[i];
        if (!(p->mem >> i & 1) || !o->alive || o->state != MS_WALK) continue;
        if (o->sleep == 1) { o->sleep = 2; o->sub_t = 0.0f; }      /* the roost drops from the ceiling */
        o->aware = MA_HUNT; o->aw_t = 0.0f; o->alert = fmaxf(o->alert, MIND_LIVE); o->kx = G.px; o->ky = G.py;
    }
    if (p->kind == PACK_ROOST) { DBG_COUNT(roosts_woken); if (p->caller >= 0) alarm_at(&G.mobs[p->caller], 8); }   /* its cry */
    if (p->kind == PACK_WHIP && p->leader >= 0 && G.mobs[p->leader].alive) alarm_at(&G.mobs[p->leader], 10);   /* the horn, heard ten
                                                                      steps off: everything there is wary of the place */
    DBG_COUNT(pack_hunts);
}
/* broken: the leader down, or half of it. By kind (P3): the file holds its drill (it only loses its order); the roost
 * scatters to other rooms, to come back as wanderers; hounds run to the nearest other creature and bring it */
static void pack_break(Pack *p) {
    p->broken = 1; p->on[0] = p->on[1] = -1;
    p->state = PAS_CALM;                                           /* a call under way dies with it */
    int told = 0;
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *o = &G.mobs[i];
        if (!(p->mem >> i & 1) || !o->alive || o->state != MS_WALK) continue;
        o->hold = 0;
        if (!told) { sfx_at(SFX_ROUT, o->x, o->y, 1.0f); told = 1; }
        if (p->kind == PACK_WHIP) {                                /* the huntsman down: his hounds run wild, quicker, careless, each */
            if (o->type == MOB_HOUND && !o->wild) {               /* for itself, sharing nothing */
                o->wild = 1; o->speed *= 1.3f; o->pack = 0; o->engage = 0; o->flank = 0;
                sfx_at(SFX_HOWL, o->x, o->y, 0.7f);
                DBG_COUNT(wild_hounds);
            }
            continue;
        }
        int living = o->type == MOB_BAT || o->type == MOB_HUNTER || o->type == MOB_IMP || o->type == MOB_GHOUL || o->type == MOB_BROOD;
        if (living) { o->flee = 5.0f + 2.0f * rndf(); o->sleep = 0; o->attacking = 0; continue; }   /* the living, and the scavengers,
                                                                      scatter */
        if (o->type != MOB_HOUND) continue;                        /* the dead hold their drill; hounds run for help */
        int16_t (*f)[MAPN] = mind_field(o->cx, o->cy);
        int best = -1, bh = 31;
        for (int j = 0; j < MAXMOBS; j++) {                        /* the nearest that thinks and isn't of this pack */
            const Mob *q = &G.mobs[j];
            if (j == i || (p->mem >> j & 1) || !minded(q) || q->sleep || q->flee > 0.0f) continue;
            int h = f[q->cy][q->cx];
            if (h > 2 && h < bh) { bh = h; best = j; }
        }
        if (best >= 0) { o->fetch = best + 1; o->fetch_t = 0.0f; o->alert = 0.0f; o->aware = MA_CALM; o->engage = 0; }
    }
    DBG_COUNT(packs_broken);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT pack_broken %s t=%.2f\n", PACK_NAME[p->kind], G.t);
#endif
}
/* a hound gone for help: at the one it went to, it tells it where you were, and both go there */
static void fetch_tick(Mob *m, float dt) {
    Mob *o = &G.mobs[m->fetch - 1];
    m->fetch_t += dt;
    if (!o->alive || o->state != MS_WALK || m->fetch_t > 20.0f) { m->fetch = 0; return; }
    if (hypotf(o->x - m->x, o->y - m->y) > 1.8f) return;
    const Pack *p = pack_of(m);
    float kx = p ? p->kx : G.px, ky = p ? p->ky : G.py;
    Pack *op = pack_of(o);
    if (op && !op->broken)
        for (int j = 0; j < MAXMOBS; j++) {
            Mob *q = &G.mobs[j];
            if ((op->mem >> j & 1) && minded(q) && q->alert <= 0.0f) { if (q->sleep == 1) { q->sleep = 2; q->sub_t = 0.0f; } mind_track(q, kx, ky); }
        }
    if (o->alert <= 0.0f) mind_track(o, kx, ky);
    mind_track(m, kx, ky);
    sfx_at(SFX_HOWL, m->x, m->y, 0.8f);
    m->fetch = 0;
    DBG_COUNT(fetches);
}
/* a pack's turns (P2 b): its melee members engage one at a time, two from rank III, each for four to six seconds or
 * until it is hurt; the rest hold off and circle */
static void pack_turns(Pack *p, float dt) {
    if (p->kind == PACK_WAKE || p->kind == PACK_ROOST || p->broken) return;
    const Mob *L = &G.mobs[p->leader];
    if (p->kind == PACK_BAND && L->type == MOB_KNIGHT) {           /* the knight and his hounds (section 7): they hold you at bay */
        int far = hypotf(L->x - G.px, L->y - G.py) > 4.0f;         /* till he is within four cells; then everything goes in */
        for (int i = 0; i < MAXMOBS; i++) {
            Mob *o = &G.mobs[i];
            if ((p->mem >> i & 1) && o->alive && o->state == MS_WALK && o->type == MOB_HOUND && o->alert > 0.0f && far) o->hold = 2;
        }
        return;
    }
    int k = L->rank >= RK_III && L->rank != RK_GILDED ? 2 : 1;
    int changed = (p->turn_t -= dt) <= 0.0f;
    for (int s = 0; s < 2; s++) {
        if (p->on[s] < 0) continue;
        Mob *o = &G.mobs[p->on[s]];
        int busy = o->swing > 0.0f || o->throw_t > 0.0f || o->sub == 3;   /* a blow, a leap, under way: let it finish */
        if (!(p->mem >> p->on[s] & 1) || !o->alive || o->state != MS_WALK || o->alert <= 0.0f || s >= k
            || (!busy && (changed || o->hp < p->on_hp[s] - 0.12f * o->maxhp))) { p->last = p->on[s]; p->on[s] = -1; }
    }
    if (changed) p->turn_t = 4.0f + 2.0f * rndf();
    for (int s = 0; s < k; s++) {
        if (p->on[s] >= 0) continue;
        int best = -1, bh = 1 << 30;
        for (int i = 0; i < MAXMOBS; i++) {
            const Mob *o = &G.mobs[i];
            if (!(p->mem >> i & 1) || !o->alive || o->state != MS_WALK || !KIND_IS(o, KF_MELEE) || o->alert <= 0.0f) continue;
            if (i == p->on[0] || i == p->on[1]) continue;
            int h = hunt[o->cy][o->cx];
            if (h < 0) continue;
            if (i == p->last) h += 6;                              /* the one just relieved goes to the back */
            if (p->kind == PACK_HOUNDS && mind_on() && fabsf(wrap_pi(atan2f(o->y - G.py, o->x - G.px) - G.ang)) > 1.9f) h -= 8;   /* the dog
                                                                      at your back (section 30): the one you aren't facing */
            if (h < bh) { bh = h; best = i; }
        }
        if (best < 0) break;
        p->on[s] = best; p->on_hp[s] = G.mobs[best].hp;
        if (p->kind == PACK_WHIP) {                                /* his whip sends it in */
            Mob *hm = &G.mobs[p->leader];
            if (hm->alive && hm->state == MS_WALK && hm->swing <= 0.0f && hm->call_t <= 0.0f) { hm->throw_t = 0.35f; sfx_at(SFX_WHIPCRACK, hm->x, hm->y, 0.9f); DBG_COUNT(whip_cracks); }
        }
        DBG_COUNT(turns);
    }
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *o = &G.mobs[i];
        if (!(p->mem >> i & 1) || !o->alive || o->state != MS_WALK || !KIND_IS(o, KF_MELEE) || o->alert <= 0.0f) continue;
        if (i != p->on[0] && i != p->on[1]) o->hold = 2;
    }
}
/* Flanking (section 13, red; step 7): the way the nearest of a hunting pack comes to you, blocked from two steps out to ten
 * and a cell either side, and the steps to you round it; those of red and black after the first that have such a way within
 * six steps more of their own take it. Made again each second, a field a pack */
static int16_t FL[MAXPACKS][MAPN][MAPN];
static double fl_t[MAXPACKS];
static int fl_key[MAXPACKS];
static int16_t (*flank_field(int k))[MAPN] { return k >= 0 && k < MAXPACKS && fl_key[k] == mf_key() && G.t >= fl_t[k] && G.t - fl_t[k] < 2.0 ? FL[k] : NULL; }
static void pack_flank(Pack *p, int k) {
    if (fl_key[k] == mf_key() && G.t >= fl_t[k] && G.t - fl_t[k] < 1.0) return;
    fl_t[k] = G.t; fl_key[k] = mf_key();
    int lead = -1, lh = 1 << 30, red = 0;
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *o = &G.mobs[i];
        if (!(p->mem >> i & 1)) continue;
        if (!minded(o) || o->alert <= 0.0f) continue;
        int h = hunt[o->cy][o->cx];
        if (h >= 0 && h < lh) { lh = h; lead = i; }
        red += cunning(o) >= RK_IV;
    }
    int16_t (*f)[MAPN] = FL[k];
    memset(f, 0xff, sizeof FL[k]);
    if (lead < 0 || red == 0 || lh < 6) {
        for (int i = 0; i < MAXMOBS; i++) if ((p->mem >> i & 1) && G.mobs[i].flank == k + 1) G.mobs[i].flank = 0;
        return;
    }
    static uint8_t blk[MAPN][MAPN];
    memset(blk, 0, sizeof blk);
    int x = G.mobs[lead].cx, y = G.mobs[lead].cy;
    for (int g = 0; g < 200 && hunt[y][x] > 0; g++) {             /* the lead's way to you */
        int h = hunt[y][x], nx = x, ny = y;
        if (h >= 2 && h <= 10) {
            blk[y][x] = 1;
            for (int d = 0; d < 4; d++) { int ax = x + DX[d], ay = y + DY[d]; if (hunt[ay][ax] >= 2) blk[ay][ax] = 1; }
        }
        for (int d = 0; d < 4; d++) if (hunt[y + DY[d]][x + DX[d]] >= 0 && hunt[y + DY[d]][x + DX[d]] < h) { nx = x + DX[d]; ny = y + DY[d]; break; }
        if (nx == x && ny == y) break;
        x = nx; y = ny;
    }
    static int q[MAPN * MAPN];
    int qh = 0, qt = 0, cx = (int)G.px, cy = (int)G.py;
    f[cy][cx] = 0; q[qt++] = cy * MAPN + cx;
    while (qh < qt) {                                              /* the steps to you, round it */
        int px = q[qh] % MAPN, py = q[qh] / MAPN; qh++;
        if (f[py][px] >= HUNT_FAR) continue;
        for (int d = 0; d < 4; d++) {
            int nx = px + DX[d], ny = py + DY[d];
            if (nx < 1 || ny < 1 || nx >= MAPN - 1 || ny >= MAPN - 1 || f[ny][nx] >= 0 || blk[ny][nx] || !hunt_pass(nx, ny)) continue;
            f[ny][nx] = (int16_t)(f[py][px] + 1); q[qt++] = ny * MAPN + nx;
        }
    }
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *o = &G.mobs[i];
        if (!(p->mem >> i & 1)) continue;
        int was = o->flank, h1 = hunt[o->cy][o->cx], h2 = f[o->cy][o->cx];
        o->flank = i != lead && minded(o) && o->alert > 0.0f && cunning(o) >= RK_IV && h1 > 4 && h2 > h1 && h2 <= h1 + 6 ? k + 1 : 0;
        if (o->flank && !was) DBG_COUNT(flank);
    }
}
/* once a frame, before the creatures move: each pack's call, its shared sight, its morale, its turns */
static void pack_frame(float dt) {
    for (int k = 0; k < G.npacks; k++) {
        Pack *p = &G.packs[k];
        uint64_t live = 0; int n = 0;
        for (int i = 0; i < MAXMOBS; i++) {
            const Mob *o = &G.mobs[i];
            if ((p->mem >> i & 1) && o->alive && o->state == MS_WALK && o->pack == k + 1) { live |= 1ull << i; n++; }
        }
        p->mem = live;
        if (!n) continue;
        if (!p->broken && p->kind != PACK_WAKE && p->leader >= 0 && !(live >> p->leader & 1) && G.mobs[p->leader].state == MS_DIE)
            deed_note(DEED_HEADS);                                 /* broken by its leader's fall */
        if (p->chill_t > 0.0f) { p->chill_t -= dt; p->state = PAS_CALM; continue; }   /* the Wraith near: it keeps to its posts */
        if (!p->broken && p->kind != PACK_WAKE && (p->leader < 0 || !(live >> p->leader & 1) || (n * 2 <= p->n0 && p->kind != PACK_WHIP))) { pack_break(p); continue; }   /* the
                                                                      huntsman's hounds break only when he falls */
        if (p->broken) continue;
        if (p->state == PAS_CALL) {
            if (!(live >> p->caller & 1)) { p->state = PAS_CALM; DBG_COUNT(pack_cuts); }   /* the caller slain: the call dies with it */
            else if ((p->call_t -= dt) <= 0.0f) pack_hunt(p);
        }
        if (p->state != PAS_HUNT) continue;
        int any = 0;
        for (int i = 0; i < MAXMOBS; i++) {
            Mob *o = &G.mobs[i];
            if (!(live >> i & 1)) continue;
            if ((float)G.t - p->seen_t < 0.05f && minded(o) && !o->fetch) {   /* what one sees, all know */
                o->alert = fmaxf(o->alert, MIND_LIVE); o->aware = MA_HUNT; o->kx = G.px; o->ky = G.py;
            }
            any |= o->aware != MA_CALM || o->alert > 0.0f;
        }
        if (!any) { p->state = PAS_CALM; continue; }
        pack_turns(p, dt);
        pack_flank(p, k);
    }
}
/* The Wraith's chill (section 7; from 9): packs that hear its wail, within sixteen cells, stop hunting and keep to their
 * posts for fifteen seconds; the dead fear it too. A blow ends it for the pack struck */
static void wraith_chill(const Mob *w) {
    if (!mind_on() || PK.depth) return;
    int n = 0;
    for (int k = 0; k < G.npacks; k++) {
        Pack *p = &G.packs[k];
        if (p->broken || p->kind == PACK_WAKE) continue;
        int near = 0;
        for (int i = 0; i < MAXMOBS && !near; i++) near = (p->mem >> i & 1) && G.mobs[i].alive && hypotf(G.mobs[i].x - w->x, G.mobs[i].y - w->y) < 16.0f;
        if (!near) continue;
        p->chill_t = 15.0f; p->state = PAS_CALM; n++;
        for (int i = 0; i < MAXMOBS; i++) {
            Mob *o = &G.mobs[i];
            if (!(p->mem >> i & 1) || !o->alive || o->hunter) continue;
            o->alert = 0.0f; o->aware = MA_CALM; o->wait = 0.0f; o->engage = 0;
        }
    }
    if (n) { DBG_COUNT(chills); if (hypotf(w->x - G.px, w->y - G.py) < 20.0f) way_met(WY_CHILL); }
}
/* a roost asleep on the ceiling: a noise reaching it, a light or a step close by, or a blow, and it calls */
static void roost_doze(Mob *m, float dt) {
    m->t += dt;
    float top = 1.0f - 0.64f * KINDS[MOB_BAT].scale * RANK_SIZE[m->rank];   /* the bat fills the upper part of its frame: turned over, the lower */
    if (m->sleep == 2) {                                           /* dropping, and on */
        m->sub_t += dt;
        m->z = lerpf(top, 0.2f, fminf(1.0f, m->sub_t / 0.5f));
        if (m->sub_t >= 0.5f) { m->sleep = 0; m->alert = fmaxf(m->alert, MIND_LIVE); m->aware = MA_HUNT; }
        return;
    }
    m->z = top;
    if (G.dead || G.level_t < 1.5 || G.wwalk_t > 0.0f) return;
    float pd = hypotf(G.px - m->x, G.py - m->y);
    int near = pd < 2.5f || (pd < 7.0f && cell((int)G.px, (int)G.py)->region == cell(m->cx, m->cy)->region && !caster_dark());
    if (near && line_clear(m->x, m->y, G.px, G.py)) pack_call(m);
}
/* the wake: when one of it rises, the rest of the yard or the water stirs, the nearest to you first */
static void wake_stir(Mob *m) {
    Pack *p = pack_of(m);
    if (!p || p->kind != PACK_WAKE) return;
    int idx[64], n = 0;
    for (int i = 0; i < MAXMOBS; i++) {
        const Mob *o = &G.mobs[i];
        if ((p->mem >> i & 1) && o != m && o->alive && o->sub == 1 && o->sub_t <= 0.0f) idx[n++] = i;
    }
    for (int i = 1; i < n; i++)
        for (int j = i; j > 0 && hypotf(G.mobs[idx[j]].x - G.px, G.mobs[idx[j]].y - G.py) < hypotf(G.mobs[idx[j - 1]].x - G.px, G.mobs[idx[j - 1]].y - G.py); j--) {
            int t = idx[j]; idx[j] = idx[j - 1]; idx[j - 1] = t;
        }
    float at = 0.0f;
    for (int i = 0; i < n; i++) { at += 0.6f + 0.6f * rndf(); G.mobs[idx[i]].sub_t = at; }
    if (n) { DBG_COUNT(wakes_rise); way_met(WY_WAKE); }
}

/* The cap (P6 a): any number may hunt, but only so many are in the fight at once (within eight cells and in sight):
 * three in run I, four in II, five in III, six in IV, one more for a person. The nearest come in, and those in stay in
 * while they are; a blow or a throw under way is let finish. The rest hold at the fight's edge. The buried come as they
 * will, outside it. */
static int engage_cap(void) { return 2 + imin(crawl_set(G.level), 3) + 2 * (G.player != 0); }   /* the dream's one less (A3, Part G):
                                                                      the cap gives way before the minds do (D5) */
/* G2 (section 27): a person doing exceptionally well (in the last two minutes under a fifth of their health lost and six or
 * more slain, above three quarters now, unhurt twenty seconds) may find one fight in three takes one more than the cap, until
 * they are hurt or a minute passes. The dream keeps the cap */
static int doing_well(void) {
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_WELL")) return G.player != 0;                /* a test: as if a person were doing exceptionally well */
#endif
    float lost = 0.0f; int k = 0;
    for (int i = 0; i < 12; i++) { lost += WELL.hp[i]; k += WELL.k[i]; }
    return G.player && lost < 0.2f * (float)G.max_hp && k >= 6 && G.hp * 4 > G.max_hp * 3 && WELL.calm > 20.0f;
}
static void well_tick(float dt) {
    if (!G.player) { memset(&WELL, 0, sizeof WELL); return; }
    WELL.calm += dt;
    if ((WELL.t += dt) >= 10.0f) { WELL.t = 0.0f; WELL.i = (WELL.i + 1) % 12; WELL.hp[WELL.i] = 0.0f; WELL.k[WELL.i] = 0; }
    if (WELL.over_t > 0.0f) WELL.over_t -= dt;
}
static void engage_pick(void) {
    int cap = engage_cap() + (WELL.over_t > 0.0f), n = 0;
    static int key[MAXMOBS];
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *m = &G.mobs[i];
        key[i] = 0;
        int busy = m->swing > 0.0f || m->throw_t > 0.0f || m->sub == 3;
        if (!minded(m) || m->alert <= 0.0f || m->sleep || m->fetch || m->fell > 0 || m->flee > 0.0f || m->type == MOB_SHAMBLER || m->type == MOB_DROWNED) { m->engage = 0; continue; }
        float d = hypotf(m->x - G.px, m->y - G.py);
        if (d > 8.0f || !line_clear(m->x, m->y, G.px, G.py)) { if (!busy) m->engage = 0; n += m->engage; continue; }
        if (m->hold == 2) { m->engage = 0; continue; }             /* its pack's turn says */
        key[i] = 1 + (int)(d * 16.0f);
        n += m->engage;
    }
    while (n > cap) {                                              /* the cap fell (a person gave over): the farthest go back */
        int worst = -1;
        for (int i = 0; i < MAXMOBS; i++) {
            const Mob *m = &G.mobs[i];
            if (m->engage && key[i] && !(m->swing > 0.0f || m->throw_t > 0.0f || m->sub == 3) && (worst < 0 || key[i] > key[worst])) worst = i;
        }
        if (worst < 0) break;
        G.mobs[worst].engage = 0; n--;
    }
    while (n < cap) {
        int best = -1;
        for (int i = 0; i < MAXMOBS; i++) if (key[i] && !G.mobs[i].engage && (best < 0 || key[i] < key[best])) best = i;
        if (best < 0) break;
        G.mobs[best].engage = 1; n++;
    }
    for (int i = 0; i < MAXMOBS; i++) if (key[i] && !G.mobs[i].engage) G.mobs[i].hold = 1;
    if (n > 0 && WELL.n0 == 0 && WELL.quiet > 5.0f && doing_well() && rnd(3) == 0) {   /* a fight begins, after a quiet */
        WELL.over_t = 60.0f;
        DBG_COUNT(cap_over);
    }
    WELL.n0 = n;
}
/* What you passed by gathers in the exit's room once you have seen it (section 23, level 39's open place; step 8): the calm
 * that your trail came within four cells of go there, and search it */
static void gather_exit(void) {
    int n = 0;
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *m = &G.mobs[i];
        if (!minded(m) || m->aware != MA_CALM || m->alert > 0.0f || m->amb || m->hunter || m->sleep || m->bait_st || buried(m)) continue;
        int passed = 0;
        for (int y = m->cy - 4; y <= m->cy + 4 && !passed; y++)
            for (int x = m->cx - 4; x <= m->cx + 4 && !passed; x++)
                passed = x > 0 && y > 0 && x < MAPN && y < MAPN && G.scent[y][x] > 0.0f;
        if (!passed) continue;
        mind_track(m, G.exit_fx + 0.5f, G.exit_fy + 0.5f); n++;
    }
    if (n >= 2) way_met(WY_GATHER);
    DBG_COUNT(gathered);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "GATHER level %d: %d go to the exit's room\n", G.level, n);
#endif
}
static void minds_frame(float dt) {
    if (!mind_on()) return;
    well_tick(dt);
    ways_ends();
    if (since(39) && !G.gathered && !PK.depth && !G.shop && G.exit_fx >= 0 && hypotf(G.exit_fx + 0.5f - G.px, G.exit_fy + 0.5f - G.py) < 7.0f
        && line_clear(G.px, G.py, G.exit_fx + 0.5f, G.exit_fy + 0.5f)) { G.gathered = 1; gather_exit(); }
    WELL.quiet = WELL.n0 ? 0.0f : WELL.quiet + dt;
    for (int i = 0; i < MAXMOBS; i++) G.mobs[i].hold = 0;
    pack_frame(dt);
    engage_pick();
    hunt_tick();
    amb_tick();
}

/* Posts, patrols and roamers (docs/PLAN-ai.md, section 9): what a calm creature does. Given the first time it thinks:
 * one that keeps to a room (a resident, a vault's guard) has a post where it stands, facing the room's way in, its head
 * turning now and then; the living that wander (bats, hunting spiders) go from room to room with a purpose; the rest
 * walk a round of three or four waypoints near where they came, pausing at each, one come with another on its round. */
enum { RL_NONE, RL_POST, RL_PATROL, RL_ROAM, RL_FOLLOW };
static const Mob *follow_of(const Mob *m) {                        /* the one it walks behind: the one before it in its pack, or the leader */
    const Pack *p = pack_of(m);
    if (!p || p->broken || p->leader < 0) return NULL;
    int me = (int)(m - G.mobs), best = -1;
    for (int i = 0; i < me; i++) if ((p->mem >> i & 1) && G.mobs[i].alive && G.mobs[i].state == MS_WALK) best = i;
    if (best < 0 && p->leader != me && G.mobs[p->leader].alive && G.mobs[p->leader].state == MS_WALK) best = p->leader;
    return best >= 0 ? &G.mobs[best] : NULL;
}
static int room_of_region(int rg) { for (int r = 0; r < G.nrooms; r++) if (G.rooms[r].region == rg) return r; return -1; }
static int room_way_in(int r, float *ex, float *ey) {             /* where what is open on the ring about a room lies, on average */
    if (r < 0 || r >= G.nrooms) return 0;
    const Room *R = &G.rooms[r];
    float sx = 0.0f, sy = 0.0f; int n = 0;
    for (int y = R->y - 1; y <= R->y + R->h; y++)
        for (int x = R->x - 1; x <= R->x + R->w; x++) {
            if (y > R->y - 1 && y < R->y + R->h && x > R->x - 1 && x < R->x + R->w) continue;
            int k = cell(x, y)->kind;
            if (k == K_FLOOR || k == K_DOOR) { sx += (float)x + 0.5f; sy += (float)y + 0.5f; n++; }
        }
    if (!n) return 0;
    *ex = sx / (float)n; *ey = sy / (float)n;
    return 1;
}
static int route_make(int sx, int sy) {                            /* a round of lane cells five to sixteen steps about (sx, sy) */
    if (G.nroute >= 24) return -1;
    int16_t (*f)[MAPN] = mind_field(sx, sy);
    int cx[64], cy[64], nc = 0;
    for (int tries = 0; tries < 400 && nc < 64; tries++) {
        int x = clampi(sx + rnd(33) - 16, 1, MAPN - 2), y = clampi(sy + rnd(33) - 16, 1, MAPN - 2);
        int h = f[y][x];
        if (h < 5 || h > 16 || !apass(x, y) || in_band(y)) continue;
        cx[nc] = x; cy[nc] = y; nc++;
    }
    if (nc < 2) return -1;
    int r = G.nroute, n = 0, used[64] = { 0 };
    for (int k = 0; k < 4 && k < nc; k++) {                        /* spread about it: each the farthest from those already chosen */
        int best = -1; float bd = -1.0f;
        for (int i = 0; i < nc; i++) {
            if (used[i]) continue;
            float d = 1e9f;
            for (int j = 0; j < n; j++) d = fminf(d, hypotf((float)(cx[i] - G.route[r].x[j]), (float)(cy[i] - G.route[r].y[j])));
            if (!n) d = (float)rnd(100);
            if (d > bd) { bd = d; best = i; }
        }
        if (best < 0 || (n && bd < 3.0f)) break;
        used[best] = 1; G.route[r].x[n] = cx[best]; G.route[r].y[n] = cy[best]; n++;
    }
    for (int i = 1; i < n; i++)                                    /* round about the place: in order of their bearing from it */
        for (int j = i; j > 0 && atan2f((float)(G.route[r].y[j] - sy), (float)(G.route[r].x[j] - sx))
                                  < atan2f((float)(G.route[r].y[j - 1] - sy), (float)(G.route[r].x[j - 1] - sx)); j--) {
            int tx = G.route[r].x[j], ty = G.route[r].y[j];
            G.route[r].x[j] = G.route[r].x[j - 1]; G.route[r].y[j] = G.route[r].y[j - 1]; G.route[r].x[j - 1] = tx; G.route[r].y[j - 1] = ty;
        }
    if (n < 2) return -1;
    G.route[r].n = n; G.nroute++;
    DBG_COUNT(routes);
    return r;
}
static void roam_next(Mob *m) {                                    /* the middle of another room */
    for (int tries = 0; tries < 8 && G.nrooms > 0; tries++) {
        const Room *R = &G.rooms[rnd(G.nrooms)];
        int x = R->x + R->w / 2, y = R->y + R->h / 2;
        if (R->region == cell(m->cx, m->cy)->region || cell(x, y)->kind != K_FLOOR || in_band(y)) continue;
        m->wx = x; m->wy = y; return;
    }
    m->wx = m->cx; m->wy = m->cy;
}
static void role_give(Mob *m) {                                    /* the first time it thinks */
    m->route = -1;
    if (m->home >= 0 || m->guard) {                                /* it keeps a room: a post, facing the way in */
        m->role = RL_POST; m->hx = m->cx; m->hy = m->cy;
        float ex, ey;
        int r = room_of_region(m->home >= 0 ? m->home : cell(m->cx, m->cy)->region);
        m->pf = room_way_in(r, &ex, &ey) ? atan2f(ey - m->y, ex - m->x) : m->face;
        DBG_COUNT(posted);
        return;
    }
    if (m->type == MOB_BAT || m->type == MOB_HUNTER || m->type == MOB_LIGHTLESS) { m->role = RL_ROAM; roam_next(m); return; }
    const Pack *p = pack_of(m);
    if (p && p->leader != (int)(m - G.mobs) && follow_of(m)) { m->role = RL_FOLLOW; return; }   /* a pack walks its leader's round */
    m->role = RL_PATROL;
    for (int i = 0; i < MAXMOBS; i++) {                            /* one come with another walks its round */
        const Mob *o = &G.mobs[i];
        if (o != m && o->alive && o->role == RL_PATROL && o->route >= 0 && hypotf(o->x - m->x, o->y - m->y) < 1.6f) { m->route = o->route; m->wp = o->wp; return; }
    }
    m->route = route_make(m->cx, m->cy);
    m->wp = 0;
}

/* ---- Hunters (docs/PLAN-ai.md, section 11; step 5). The caster leaves a trail (G.scent, when it was in each cell; none in
 * water). On some levels a hunt waits at the far end: a quarter of run I's from level 5, a third of run II's, half of run
 * III's, two thirds of run IV's, every corrupted level, never a boss's (H4). At 60-150 s in, sooner deeper in the run, it
 * is let loose with a far-off tell: hounds (from 9) and hunting spiders (from 15) pick the trail up where you were a minute
 * ago and come the way you came; within a few seconds of you they hunt as anything does, and when they lose you they
 * take the trail again rather than search. Run I's hunt is the Order's watch, a file that goes to your last loud noise. ---- */
#define SCENT_LIFE 120.0f
enum { HK_NONE, HK_WATCH, HK_HOUNDS, HK_SPIDERS };
#ifdef CRAWL_DEBUG
static const char *HK_NAME[] = { "none", "watch", "hounds", "spiders" };
#endif
static int on_trail(const Mob *m) { float s = G.scent[m->cy][m->cx]; return s > 0.0f && (float)G.level_t - s < SCENT_LIFE; }
static int hunt_sfx(int kind) { return kind == HK_HOUNDS ? (G.hunt_whip ? SFX_HORN : SFX_HOWL) : kind == HK_SPIDERS ? SFX_SKITTER : SFX_DRUM; }
static int place_hunt(void) {
    G.hunt_kind = G.hunt_on = G.hunt_said = G.hunt_whip = 0;
    if (!mind_on() || PK.depth || G.boss >= 0 || G.level < 5) return 0;
    int set = crawl_set(G.level), lv = G.level > DESCENT_LEN ? 99 : G.level;
    float chance = G.corrupt ? 1.0f : set == 0 ? 0.25f : set == 1 ? 0.334f : set == 2 ? 0.5f : 0.667f;
    float at = 150.0f - 10.0f * (float)crawl_step(G.level) + (rndf() - 0.5f) * 30.0f;
    int kind = 0;
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_HUNT")) { chance = 1.0f; at = (float)atof(getenv("CRAWL_HUNT")); }
    if (getenv("CRAWL_HUNTK")) kind = clampi(atoi(getenv("CRAWL_HUNTK")), HK_WATCH, HK_SPIDERS);
#endif
    if (rndf() >= chance) return 0;
    if (!kind) {                                                   /* the watch alone till the hounds come; then the newer more often */
        int w[4] = { 0, 1, lv >= 9 ? 2 : 0, lv >= 15 ? 2 : 0 }, tot = w[1] + w[2] + w[3], r = rnd(tot);
        kind = 1;
        while (r >= w[kind]) r -= w[kind++];
    }
    int x, y, behind = since(37) && G.namb > 0 && kind != HK_WATCH;   /* a hunt that drives you into an ambush (section 23; from 37): */
    if (behind) {                                                  /* it waits behind you, near the start, and comes on the trail */
        if (!pick_cell_at_depth(G.cx, G.cy, 10, 18, 1, &x, &y)) behind = 0;
        else at = 60.0f + 20.0f * rndf();                          /* never before a minute (rule 2) */
    }
    if (!behind && !pick_cell_at_depth(G.cx, G.cy, 24, 400, 1, &x, &y) && !pick_cell_at_depth(G.cx, G.cy, 16, 400, 1, &x, &y)) return 0;
    if (in_band(y) || in_wing(x, y) || in_trial(x, y) || vault_of_room(x, y) >= 0) return 0;
    int whip = kind == HK_HOUNDS && set >= 2 && may_spawn(MOB_WHIPPER) && rnd(5) < 3;   /* a hunt in the Hollows is often the
                                                                      Whipper-In's, his hounds on the trail before him (step 9) */
    int pk = pack_new(kind == HK_WATCH ? PACK_FILE : kind == HK_HOUNDS ? (whip ? PACK_WHIP : PACK_HOUNDS) : PACK_SPIDERS);
    if (pk < 0) return 0;
    int n = imin(kind == HK_WATCH ? 3 + (set >= 1) : kind == HK_HOUNDS ? 2 + (set >= 2) + (set >= 3) + whip : 2 + (set >= 2), set_cap), made = 0;
    if (n < 2) { G.npacks--; return 0; }
    for (int i = 0; i < n; i++) {
        int mx = x, my = y;
        if (i && !free_near(x, y, &mx, &my)) break;
        int type = kind == HK_HOUNDS ? (whip && i == 0 ? MOB_WHIPPER : MOB_HOUND) : kind == HK_SPIDERS ? MOB_HUNTER : (i == 0 && may_spawn(MOB_SHIELD) ? MOB_SHIELD : MOB_SKEL);
        Mob *m = spawn_mob(mx, my, type);
        if (!m) break;
        m->hunter = 1; m->home = G.map[my][mx].region;             /* it waits where it is till the hunt */
        pack_join(pk, m); made++;
    }
    if (!made) return 0;
    G.hunt_kind = kind; G.hunt_at = fmaxf(at, 0.0f); G.hunt_whip = whip;
#ifdef CRAWL_DEBUG
    fprintf(stderr, "PACKS level %d: hunt of %d %s%s at (%d,%d), loose at %.0f s%s\n", G.level, made, HK_NAME[kind], whip ? " and their huntsman" : "", x, y, G.hunt_at, behind ? ", behind you, to drive you on" : "");
#endif
    return 1;
}
/* once a frame: the hunt let loose at its time; the watch sent to each new loud noise; over when all of it is gone */
static void hunt_tick(void) {
    if (!G.hunt_kind || PK.depth || G.dead) return;
    int alive = 0;
    if (!G.hunt_on) {
        if (G.level_t < G.hunt_at) return;
        G.hunt_on = 1;
        float want = (float)G.level_t - 60.0f, bd = 1e9f;          /* where you were a minute ago, or as near it as the trail goes */
        G.hunt_nx = G.cx; G.hunt_ny = G.cy;
        for (int y = 1; y < MAPN - 1; y++)
            for (int x = 1; x < MAPN - 1; x++) {
                float sc = G.scent[y][x];
                if (sc > 0.0f && fabsf(sc - want) < bd) { bd = fabsf(sc - want); G.hunt_nx = x; G.hunt_ny = y; }
            }
        G.hunt_heard = G.noise_t;
        for (int i = 0; i < MAXMOBS; i++) {
            Mob *m = &G.mobs[i];
            if (!m->alive || m->state != MS_WALK || m->hunter != 1) continue;
            m->hunter = G.hunt_kind == HK_WATCH ? 3 : 2; m->home = -1; m->role = RL_ROAM; m->wx = m->cx; m->wy = m->cy;
            if (m->type == MOB_WHIPPER && G.hunt_whip) m->feat |= FT_HORN;   /* the hunt's horn is his */
            if (m->hunter == 3 && G.noise_t > 0.0f && m->alert <= 0.0f) mind_track(m, G.noise_x, G.noise_y);
            alive++;
        }
        snd_sfx_ex(hunt_sfx(G.hunt_kind), 0.35f, 0.5f);           /* far off */
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT hunt_begin %s %d trail from (%d,%d) t=%.2f\n", HK_NAME[G.hunt_kind], alive, G.hunt_nx, G.hunt_ny, G.t);
#endif
        if (!alive) G.hunt_kind = 0; else way_met(WY_HUNT);
        return;
    }
    int noise = G.hunt_kind == HK_WATCH && G.noise_t > G.hunt_heard;
    if (noise) G.hunt_heard = G.noise_t;
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *m = &G.mobs[i];
        if (!m->alive || m->state != MS_WALK || !m->hunter) continue;
        alive++;
        if (noise && m->hunter == 3 && m->alert <= 0.0f) mind_track(m, G.noise_x, G.noise_y);
    }
    if (!alive) G.hunt_kind = 0;
}
/* a hunter on the trail: within a few seconds of you it has you; a trail lost (water, a pocket, gone cold) ends its hunt */
static void hunter_tick(Mob *m) {
    if (m->hunter != 2 || m->alert > 0.0f) return;
    float sc = G.scent[m->cy][m->cx];
    if (sc > 0.0f && (float)G.level_t - sc < 4.0f) {
        m->alert = MIND_LIVE; m->aware = MA_HUNT; m->kx = G.px; m->ky = G.py;
        if (!G.hunt_said) { G.hunt_said = 1; sfx_at(hunt_sfx(G.hunt_kind ? G.hunt_kind : HK_HOUNDS), m->x, m->y, 1.0f); dream_glance(m->x, m->y); }   /* nearer */
        DBG_COUNT(hunt_scent);
    }
}
static void hunter_lost(Mob *m) {
    m->hunter = 0; mind_track(m, m->x, m->y);
    if (!G.hunt_shaken && G.hunt_on) { G.hunt_shaken = 1; deed_note(DEED_SHAKEN); }   /* the trail broken: water, a door, a pocket */
    DBG_COUNT(hunt_lost);
}

/* ---- The dead's ways shown (docs/PLAN-ai.md, Part H; 1.0.0b18). Each set piece is named the first time it shows itself
 * on each run of ten, as the hazards are; once met it has a page in the Codex, THE WAYS OF THE DEAD. The record keeps
 * what was done about them, for five achievements. ---- */
static const char *const WAY_NAME[NWAY] = { "THE FILE OF THE ORDER", "THE HOUNDS' PACK", "THE ROOST", "THE WAKE", "THE SCAVENGERS",
    "THE CALLERS", "THE HUNT", "THE AMBUSH", "THE WATCH AND ITS DEAD", "THE NEST", "THE BAND", "THE IMPS' BROOD", "THE SENTINELS",
    "THE WHIPPER-IN'S HOUNDS", "THE SEXTON'S BELL", "THE WRAITH'S CHILL", "THE GATHERING" };
static const char *const WAY_SAY[NWAY] = { "THE FILE OF THE ORDER IS ON YOU", "THE PACK GIVES TONGUE", "THE ROOST WAKES",
    "THE WAKE RISES", "SOMETHING COMES TO FEED", "IT CALLS THE OTHERS", "SOMETHING IS SET ON YOUR TRAIL", "AN AMBUSH",
    "THE WATCH CALLS ITS DEAD", "THE NEST STIRS ABOVE YOU", "A BAND OF THE DEAD", "THE BROOD COMES OUT OF THE HEAT",
    "THE SENTINELS WAKE", "THE HUNTSMAN'S HORN", "THE SEXTON RINGS FOR THEM", "THE DEAD FALL STILL AT THE WAIL",
    "THEY HAVE GATHERED AT THE WAY OUT" };
static void way_met(int w) {
    if (w < 0 || w >= NWAY || !mind_on()) return;
    RUN.ways |= 1u << w;
    int run = crawl_run(G.level);
    if (RUN.way_run != run) { RUN.way_run = run; RUN.way_said = 0; }
    if (RUN.way_said >> w & 1) return;
    RUN.way_said |= 1u << w;
    say("%s", WAY_SAY[w]);
#ifdef CRAWL_DEBUG
    fprintf(stderr, "EVENT way %s t=%.2f\n", WAY_NAME[w], G.t);
#endif
}
static int pack_way(const Pack *p) {
    int lset = p->leader >= 0 ? G.mobs[p->leader].set : 0;
    if (lset == SET_AMBUSH) return WY_AMBUSH;                       /* an ambush's teeth are the ambush */
    switch (p->kind) {
    case PACK_FILE: return lset == SET_WATCH ? WY_WATCH : WY_FILE;   /* the watch's dead are a file of two or three */
    case PACK_HOUNDS: return WY_HOUNDS;
    case PACK_ROOST: return WY_ROOST;
    case PACK_WAKE: return WY_WAKE;
    case PACK_SCAV: return WY_SCAV;
    case PACK_SPIDERS: return lset == SET_HUNT ? -1 : WY_NEST;     /* a hunt's spiders are the hunt's */
    case PACK_IMPS: return WY_IMPS;
    case PACK_SENTINELS: return WY_SENTINELS;
    case PACK_BAND: return WY_BAND;
    case PACK_WHIP: return WY_WHIP;
    default: return -1;
    }
}
/* the record's five deeds, and their achievements (bits from ACH_WAY0) */
static const int AMB_STEP[3] = { 1, 5, 20 }, SHAKE_STEP[2] = { 1, 10 }, HEAD_STEP[3] = { 1, 10, 50 }, HUSH_STEP[2] = { 1, 10 };
static const char *const AMB_ACH[3] = { "THE TEETH CLOSED ON NOTHING", "WISE TO THE BAIT", "NO ROOM TO KILL IN" };
static const char *const SHAKE_ACH[2] = { "OFF THE SCENT", "A COLD TRAIL" };
static const char *const HEAD_ACH[3] = { "CUT OFF THE HEAD", "SHEPHERD OF NONE", "THE LEADERLESS DEAD" };
static const char *const HUSH_ACH[2] = { "BEFORE THE CRY", "A HOUSE OF SILENCE" };
#define FILE_ACH "OUT OF STEP"
static void ach_earn(int bit, int spr, int kind, int rank, const char *title, const char *detail);
static void deed_note(int which) {
    char d[48];
    switch (which) {
    case DEED_AMB:
        if (RUN.deed_amb < 65535) RUN.deed_amb++;
        for (int k = 0; k < 3; k++) if (RUN.deed_amb >= AMB_STEP[k]) { snprintf(d, sizeof d, "%d AMBUSH%s COME THROUGH", AMB_STEP[k], AMB_STEP[k] > 1 ? "ES" : ""); ach_earn(ACH_WAY0 + k, SP_SKEL_A, -1, 0, AMB_ACH[k], d); }
        break;
    case DEED_SHAKEN:
        if (RUN.deed_shaken < 65535) RUN.deed_shaken++;
        for (int k = 0; k < 2; k++) if (RUN.deed_shaken >= SHAKE_STEP[k]) { snprintf(d, sizeof d, "%d HUNT%s SHAKEN OFF", SHAKE_STEP[k], SHAKE_STEP[k] > 1 ? "S" : ""); ach_earn(ACH_WAY0 + 3 + k, SP_HOUND_A, -1, 0, SHAKE_ACH[k], d); }
        break;
    case DEED_HEADS:
        if (RUN.deed_heads < 65535) RUN.deed_heads++;
        for (int k = 0; k < 3; k++) if (RUN.deed_heads >= HEAD_STEP[k]) { snprintf(d, sizeof d, "%d PACK%s BROKEN AT THE HEAD", HEAD_STEP[k], HEAD_STEP[k] > 1 ? "S" : ""); ach_earn(ACH_WAY0 + 5 + k, SP_SHIELD_A, -1, 0, HEAD_ACH[k], d); }
        break;
    case DEED_HUSH:
        if (RUN.deed_hush < 65535) RUN.deed_hush++;
        for (int k = 0; k < 2; k++) if (RUN.deed_hush >= HUSH_STEP[k]) { snprintf(d, sizeof d, "%d CALLER%s SILENCED", HUSH_STEP[k], HUSH_STEP[k] > 1 ? "S" : ""); ach_earn(ACH_WAY0 + 8 + k, SP_WSKULL1, -1, 0, HUSH_ACH[k], d); }
        break;
    default:
        if (RUN.deed_file < 65535) RUN.deed_file++;
        ach_earn(ACH_WAY0 + 10, SP_SHIELD_A, -1, 0, FILE_ACH, "A FILE BEATEN UNTOUCHED");
        break;
    }
#ifdef CRAWL_DEBUG
    static const char *const DN[5] = { "ambush_through", "hunt_shaken", "pack_headless", "caller_hushed", "file_untouched" };
    fprintf(stderr, "EVENT deed %s t=%.2f\n", DN[clampi(which, 0, 4)], G.t);
#endif
}
/* a caller slain before its call went out: the wall skull in its fire's first moment, the deacon drawing breath, the Sexton's
 * bell raised, a pack's caller mid-call */
static int caller_hushed(const Mob *m) {
    if (!mind_on() || (!minded(m) && m->type != MOB_WSKULL)) return 0;
    if (m->type == MOB_WSKULL) return m->sub == 1 && !m->called;
    if (m->type == MOB_DEACON && m->call_t > 0.0f && !m->called) return 1;
    if (m->type == MOB_SEXTON && m->rings == 1) return 1;
    const Pack *p = m->pack ? &G.packs[m->pack - 1] : NULL;
    return p && p->state == PAS_CALL && p->caller == (int)(m - G.mobs);
}
/* the end of a set piece, once each: an ambush whose teeth are all down, a file of the Order down to the last */
static void ways_ends(void) {
    for (int k = 0; k < G.namb; k++) {
        Ambush *a = &G.amb[k];
        if (a->done || a->sprung != 1) continue;
        int left = 0;
        for (int i = 0; i < MAXMOBS && !left; i++) left = G.mobs[i].alive && G.mobs[i].amb == k + 1 && G.mobs[i].bait_st != B_GILD;
        if (!left) { a->done = 1; if (G.retries == a->deaths0 && !G.dead) deed_note(DEED_AMB); }
    }
    for (int k = 0; k < G.npacks; k++) {
        Pack *p = &G.packs[k];
        if (p->done || p->kind != PACK_FILE || !p->hunted || p->mem) continue;
        p->done = 1;
        if (G.hurts == p->hurts0 && !G.dead) deed_note(DEED_FILE);
    }
}

