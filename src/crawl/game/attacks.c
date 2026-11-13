/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The pockets' state (PK, G_LEVEL); a creature's blow by level; ATTACKS; footprints; what a cell blocks (light, a body, a step). */

/* pockets (see the pockets' code below): the level as it waits, and where in a chain you are */
#define POCKET_LINKS 8
typedef __typeof__(G) GState;
static GState G_LEVEL, G_KEEP;                                     /* the level as it waits; the caster on the way between */
static struct {
    int level, depth, chain;                                       /* depth: 0 in the level, else the link you are in (pockets are 2 on) */
    int back_x, back_y, on_x, on_y;                                /* this pocket's doorways: the way back, and on (x -1: none) */
    float since;                                                   /* since the last doorway, for the fade */
    uint8_t reached[MAXSECRETS];                                   /* the deepest link found in each chain */
    uint32_t taken[MAXSECRETS][POCKET_LINKS];                      /* the things picked up in each pocket, a bit each */
    uint8_t vis_used[MAXSECRETS];                                  /* each chain's visportal, once it has given */
    uint32_t slain[MAXSECRETS][POCKET_LINKS];                      /* the creatures killed in each pocket, a bit each */
    uint8_t auto_done[MAXSECRETS];                                 /* the autopilot: the chains it has been into, */
    int auto_back; float auto_t;                                   /* whether it is on its way out, and its time in them */
} PK;
/* the pockets' creatures: a quarter stronger in the Backrooms, 30% in Severance (whose also strike sooner) */
static float theme_boost(void) { return G.ptheme == PT_BACKROOMS ? 1.25f : G.ptheme == PT_SEVERANCE ? 1.3f : 1.0f; }

static uint8_t ramp_at(int ramp, int k) { return (uint8_t)(ramp + clampi(k, 0, ramp == R_GOLD ? 12 : 15)); }   /* the gold ramp is short */
static uint8_t qual_text(int q) { return q == Q_COMMON ? (uint8_t)C_WHITE : ramp_at(QUAL_RAMP[q], 13); }   /* white, then green, blue, purple, gold */
static int rnd(int n) { return (int)(rng_next(&G.rs) % (uint32_t)n); }
static float decay(void) { return crawl_decay(G.level); }        /* rot and ruin: 0 as a set begins, 1 on its last level */
static float depth(void) { return crawl_depth(G.level); }         /* how far down: 0 in the Crypts, 1 from the Deep on */
/* a creature's blow: a little more each level, more by its rank, less after each death on the level */
static float theme_boost(void);
static float mob_damage(const Mob *m) {
    float d = (4.0f + 0.8f * (float)G.level) * m->dmgk * theme_boost();
    int r = G.retries < 3 ? G.retries : 3;
    return d * (1.0f - 0.15f * (float)r);
}
/* CRAWL_OLDHARM=1 (the debug build): C1 of docs/PLAN-combat.md set aside, harm as 1.0.0b18 dealt it (fire's resist on every
 * spell, armour taken off flat, the throws with no floor), so the other track's balance runs can leave it out */
static int old_harm(void) {
#ifdef CRAWL_DEBUG
    static int v = -1;
    if (v < 0) v = getenv("CRAWL_OLDHARM") && atoi(getenv("CRAWL_OLDHARM")) ? 1 : 0;
    return v;
#else
    return 0;
#endif
}
/* 5% quicker each level, never quicker than 0.6 s (P1: by level 40 it had come to a throw every 0.22 s, and kept on past it) */
static float throw_interval(void) { return fmaxf(old_harm() ? 0.0f : 0.6f, 1.5f / powf(1.05f, (float)(G.level - 1))); }
/* What the dead do to you (docs/PLAN-combat.md, section 3): every attack as a row, so the blows in update_mob, the claws at a
 * web or an ally (mob_blow) and the balance sheet (make combat) read the same numbers. k: its share of the creature's blow
 * (mob_damage), and add on top; wind: the wind-up (for a leap its flight, for a bat the time it must cling); cool: the wait
 * after it (below 0, throw_interval's), cool_rnd up to that much more; go: how near you must be for it to begin (go2 the far
 * edge, for throws and leaps); reach: how near you must still be as it lands; slow: how long it holds you after */
enum { AK_BLOW, AK_BITE, AK_THROW, AK_LEAP, AK_TOUCH, AK_CAST, AK_SPEW, AK_CHARGE };
typedef struct { int type, ak, el; float k, add, wind, cool, cool_rnd, go, go2, reach, slow; } FoeAtk;
enum { FA_BONE, FA_SWORD, FA_GRAB, FA_DGRAB, FA_HBITE, FA_HLEAP, FA_GLEAP, FA_KNIGHT, FA_BROOD, FA_LING, FA_SHADE, FA_EMBER,
       FA_FIREBALL, FA_CLAW, FA_SWOOP, FA_BAT, FA_LASH, FA_GLASS, FA_WTOUCH, FA_WBOSS, FA_WEB,
       FA_VOLLEY, FA_CHARGE, FA_SPEW, NFA };                       /* C4's: after each kind's usual, so fa_main finds that first */
static const FoeAtk ATTACKS[NFA] = {
    /*               kind            what      element    k      add   wind   cool  rnd   go    go2   reach  slow */
    [FA_BONE]     = { MOB_SKEL,      AK_THROW, EL_PHYS,   1.0f,  0.0f, 0.35f, -1.0f, 0.0f, 0.0f, 99.0f, 0.36f, 0.0f },
    [FA_SWORD]    = { MOB_SHIELD,    AK_BLOW,  EL_PHYS,   1.0f,  1.0f, 0.45f, 1.4f, 0.0f, 1.35f, 0.0f, 1.6f,  0.0f },
    [FA_GRAB]     = { MOB_SHAMBLER,  AK_BLOW,  EL_PHYS,   1.0f,  0.0f, 0.6f,  1.8f, 0.0f, 1.25f, 0.0f, 1.45f, 1.2f },
    [FA_DGRAB]    = { MOB_DROWNED,   AK_BLOW,  EL_PHYS,   1.0f,  0.0f, 0.6f,  1.8f, 0.0f, 1.25f, 0.0f, 1.45f, 2.5f },
    [FA_HBITE]    = { MOB_HOUND,     AK_BITE,  EL_PHYS,   0.8f,  0.0f, 0.25f, 1.3f, 0.0f, 1.15f, 0.0f, 1.3f,  0.0f },
    [FA_HLEAP]    = { MOB_HUNTER,    AK_LEAP,  EL_PHYS,   0.6f,  0.0f, 0.45f, 3.0f, 0.0f, 0.9f,  2.4f, 1.2f,  0.0f },
    [FA_GLEAP]    = { MOB_GHOUL,     AK_LEAP,  EL_PHYS,   0.85f, 0.0f, 0.45f, 2.5f, 0.0f, 0.9f,  2.6f, 1.2f,  0.0f },
    [FA_KNIGHT]   = { MOB_KNIGHT,    AK_BLOW,  EL_PHYS,   1.5f,  0.0f, 0.9f,  2.0f, 0.0f, 1.5f,  0.0f, 1.7f,  0.0f },
    [FA_BROOD]    = { MOB_BROOD,     AK_BITE,  EL_PHYS,   1.2f,  0.0f, 0.0f,  1.6f, 0.0f, 1.4f,  0.0f, 1.4f,  0.0f },
    [FA_LING]     = { MOB_LING,      AK_BITE,  EL_PHYS,   0.4f,  0.0f, 0.0f,  1.0f, 0.0f, 0.8f,  0.0f, 0.8f,  0.0f },
    [FA_SHADE]    = { MOB_SHADE,     AK_TOUCH, EL_SPIRIT, 1.0f,  0.0f, 0.5f,  2.2f, 0.0f, 0.95f, 0.0f, 1.1f,  0.0f },   /* half while it takes souls */
    [FA_EMBER]    = { MOB_IMP,       AK_THROW, EL_FIRE,   0.8f,  0.0f, 0.4f,  2.4f, 1.0f, 1.5f,  8.0f, 0.36f, 0.0f },
    [FA_FIREBALL] = { MOB_WSKULL,    AK_CAST,  EL_FIRE,   2.0f,  0.0f, 3.0f,  3.0f, 0.0f, 0.0f,  9.0f, 0.36f, 0.0f },
    [FA_CLAW]     = { MOB_GARGOYLE,  AK_BLOW,  EL_PHYS,   1.1f,  0.0f, 0.3f,  1.6f, 0.0f, 1.2f,  0.0f, 1.3f,  0.0f },
    [FA_SWOOP]    = { MOB_GARGOYLE,  AK_LEAP,  EL_PHYS,   0.85f, 0.0f, 0.45f, 2.5f, 0.0f, 1.4f,  3.2f, 1.2f,  0.0f },
    [FA_BAT]      = { MOB_BAT,       AK_BITE,  EL_PHYS,   1.0f,  0.0f, 3.0f,  0.0f, 0.0f, 1.25f, 0.0f, 1.25f, 0.0f },
    [FA_LASH]     = { MOB_WHIPPER,   AK_BLOW,  EL_PHYS,   0.9f,  0.0f, 0.4f,  1.8f, 0.0f, 2.4f,  0.0f, 2.6f,  0.0f },
    [FA_GLASS]    = { MOB_LIGHTLESS, AK_BLOW,  EL_PHYS,   1.3f,  0.0f, 0.4f,  1.6f, 0.0f, 1.3f,  0.0f, 1.5f,  0.0f },
    [FA_WTOUCH]   = { MOB_WRAITH,    AK_TOUCH, EL_FROST,  1.5f,  0.0f, 0.0f,  2.0f, 0.0f, 1.1f,  0.0f, 1.1f,  2.0f },
    [FA_WBOSS]    = { MOB_WRAITH,    AK_TOUCH, EL_FROST,  1.2f,  0.0f, 0.0f,  2.5f, 0.0f, 1.1f,  0.0f, 1.1f,  2.0f },   /* the Wraith Unbound's */
    [FA_WEB]      = { MOB_SPIDER,    AK_BITE,  EL_POISON, 0.0f,  0.0f, 0.25f, 3.0f, 0.0f, 2.0f,  0.0f, 1.05f, 4.0f },   /* poison, no blow */
    /* C4 (section 8): the Bone Thrower's three (each a bone's), the shield-bearer's charge, the Drowned's spew */
    [FA_VOLLEY]   = { MOB_SKEL,      AK_THROW, EL_PHYS,   1.0f,  0.0f, 0.6f,  12.0f, 6.0f, 2.5f, 9.0f, 0.9f,  0.0f },
    [FA_CHARGE]   = { MOB_SHIELD,    AK_CHARGE,EL_PHYS,   1.2f,  0.0f, 0.5f,  8.0f, 8.0f, 2.0f,  5.0f, 0.95f, 0.0f },
    [FA_SPEW]     = { MOB_DROWNED,   AK_SPEW,  EL_POISON, 0.8f,  0.0f, 0.5f,  6.0f, 3.0f, 1.2f,  2.0f, 2.2f,  0.0f },
};
static int fa_main(int type) {                                     /* a kind's usual attack, -1 none (the deacon, the Sexton) */
    for (int i = 0; i < NFA; i++) if (ATTACKS[i].type == type) return i;
    return -1;
}
static float fa_every(int fa) {                                    /* how often it comes, wind-up and wait together */
    const FoeAtk *a = &ATTACKS[fa];
    return a->cool < 0.0f ? throw_interval() : a->ak == AK_LEAP ? a->cool : a->wind + a->cool;
}
static float rndf(void) { return rng_f(&G.rs); }
static uint32_t hash2(int x, int y) {
    uint32_t h = (uint32_t)x * 73856093u ^ (uint32_t)y * 19349663u ^ (uint32_t)G.level * 83492791u;
    h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
    return h;
}

static Cell *cell(int x, int y) {
    static Cell solid = { K_WALL, T_STONE, 0, 0, -1, -1, 0, 0, { 0 }, { 0 }, 0, 0, 0, 0, 0 };
    if (x < 0 || y < 0 || x >= MAPN || y >= MAPN) return &solid;
    return &G.map[y][x];
}
static int is_open(int x, int y) { int k = cell(x, y)->kind; return k == K_FLOOR || k == K_DOOR; }
static int blocks_light(int x, int y) {
    Cell *c = cell(x, y);
    return c->kind == K_WALL || (c->kind == K_DOOR && G.doors[c->door].open < 0.5f);
}
static int slide_covers(int x, int y);
/* Footprints (1.0.0b18.1, docs/PLAN-b18-1.md section 8): a prop dress_level lays takes the part of its cell its art gives it.
 * The cell keeps its kind, so whatever tests for floor keeps its old caution; what walks reads the quarters taken. */
static int foot_of(int x, int y) {
    if ((unsigned)x >= MAPN || (unsigned)y >= MAPN || G.map[y][x].kind != K_PROP) return 0;
    return G.foot[y][x];
}
static int foot_hit(float px, float py) {                          /* a point in what a part-filled cell's prop takes */
    int x = (int)floorf(px), y = (int)floorf(py), m = foot_of(x, y);
    return m && (m >> ((px - (float)x >= 0.5f) | (py - (float)y >= 0.5f) << 1) & 1);
}
static void cell_pass(int x, int y, float *gx, float *gy) {        /* where a creature passing through a cell aims: its middle, */
    int m = foot_of(x, y);                                         /* or the middle of what a prop leaves free of it */
    *gx = (float)x + 0.5f; *gy = (float)y + 0.5f;
    if (!m) return;
    if (m == 1 || m == 2 || m == 4 || m == 8) {                     /* a quarter: across from it */
        int q = m == 1 ? 0 : m == 2 ? 1 : m == 4 ? 2 : 3;
        *gx = (float)x + ((q & 1) ? 0.3f : 0.7f); *gy = (float)y + ((q & 2) ? 0.3f : 0.7f);
        return;
    }
    if (m == 3) *gy = (float)y + 0.75f; else if (m == 12) *gy = (float)y + 0.25f;   /* a half: the other */
    else if (m == 5) *gx = (float)x + 0.75f; else if (m == 10) *gx = (float)x + 0.25f;
}
static void foot_settle(void) {                                    /* one born by a prop (raised from remains there, a sac laid,
                                                                      spiderlings) stands on its cell's middle: into what is free */
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *m = &G.mobs[i];
        if (m->alive && foot_of(m->cx, m->cy) && m->x == (float)m->cx + 0.5f && m->y == (float)m->cy + 0.5f) cell_pass(m->cx, m->cy, &m->x, &m->y);
    }
}
static int blocks_move(int x, int y) {
    Cell *c = cell(x, y);
    if (G.slide >= 0 && slide_covers(x, y)) return 1;
    return c->kind == K_WALL || c->kind == K_PILLAR || (c->kind == K_PROP && !foot_of(x, y)) || (c->kind == K_DOOR && G.doors[c->door].open < 0.9f);
}
/* where the caster may walk: a vault door only once its key is in hand */
static int passable(int x, int y) {
    Cell *c = cell(x, y);
    if (c->kind == K_FLOOR) return 1;
    if (c->kind != K_DOOR) return 0;
    const Door *d = &G.doors[c->door];
    return d->locked == SEALED || d->locked == WINGED || d->locked == TRIALED || d->locked == GATED ? 0 : d->locked != 1 || G.has_key[d->color];
}
static int vault_of_room(int x, int y);
static Mob *spawn_mob(int x, int y, int type);
static int align_rank(int e);
static int may_spawn(int type);
static int occupied(int x, int y, const Mob *self);
static void name_newcomers(void);
static void garg_wake(Mob *m);
static float heading_angle_raw(int h) { return h * (PI_F * 0.5f) + PI_F; }   /* facing back the other way */
static float wrap_pi(float a) { while (a > PI_F) a -= TAU_F; while (a < -PI_F) a += TAU_F; return a; }

