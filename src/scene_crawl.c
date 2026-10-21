/* CATACOMB - a Catacomb 3-D / Wolfenstein 3-D style crawl through generated
 * crypts, in VGA mode 13h (320x200, 256 colours).
 *
 * A grid raycaster with sliding doors, torch-lit walls (light spreads through
 * open space, not through walls), billboard sprites, and an autopilot that
 * explores with hard 90-degree turns.  Each level opens in the merchant's
 * antechamber (the owner's cat on a chest), where the caster buys one spell,
 * rune or piece of gear with its gold; crawl_gear.c holds the wares.  In the
 * crypt it detours for treasure it has seen or a key it can hear, opens the
 * gold, silver and crimson vaults, and leaves through the exit for a tally.
 * Skeletons throw bones, bats bite, wall skulls gather fire, shield-bearers
 * swing swords and spiders drop from their webs; wraiths only loom.  The hand
 * casts whichever spell suits the target, and its tattooed runes defend it on
 * their own.  HP, gold, souls and the face show only in pop-ups that slide in
 * when something changes, so nothing sits still on an OLED. */
#include "crawl.h"
#include "crawl_gear.h"
#include "gfx.h"
#include "snd.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#define W 320
#define H 200
#define MAPN 33
#define LMR 4                               /* light-map texels per cell */
#define LMN (MAPN * LMR)
#define PLANE_LEN 0.75f
#define FOCAL ((W / 2) / PLANE_LEN)
#define CRUISE 2.1f                         /* cells per second */

enum { K_FLOOR, K_WALL, K_DOOR, K_PILLAR };
enum { TH_CORRIDOR, TH_OSSUARY, TH_CRYPT, TH_BRICK, TH_MOSS, TH_COUNT };
enum { PS_MOVE, PS_TURN, PS_DOOR, PS_AIM, PS_CAST, PS_UNAIM, PS_HOLD, PS_EXIT, PS_SHOP, PS_PLAYER, PS_SETTLE };
enum { MOB_SKEL, MOB_GHOST, MOB_BAT, MOB_WSKULL, MOB_SHIELD, MOB_SPIDER };
enum { MS_WALK, MS_DIE, MS_RECOIL, MS_FLEE };   /* the last two: wraiths only */
enum { IT_COINS, IT_CHALICE, IT_GEMS, IT_CROWN, IT_CHEST, IT_KEY, IT_POTION, IT_URN, IT_RING };
static const int ITEM_VALUE[] = { 10, 25, 50, 100, 250, 0, 0, 0, 0 };
enum { MIS_BONE, MIS_FIRE };                /* what the dead throw: bones, and a wall skull's fireball */
enum { DM_PIERCE = 1, DM_ARC = 2, DM_SLOW = 4 };   /* how a hit lands: through shields, round them, chilling */
enum { HURT_BLOW, HURT_MAGIC, HURT_POISON };
#define NKEYS 3                             /* gold, silver, crimson */
#define SHOT_REFLECT NATK                   /* a shot kind: something the Mirror Veil sent back */

typedef struct { uint8_t kind, tex, theme, visits; int16_t region, door; } Cell;
typedef struct { int x, y, along_x; float open, idle; int opening, locked, color; } Door;   /* locked: 1 locked, 2 unlocked */
typedef struct { float x, y, power, radius, phase; int cx, cy, first, count; } Light;
typedef struct { float x, y; int spr; } Deco;
typedef struct {
    int type, state, alive; float x, y, z, t, anim, speed; int cx, cy, tx, ty;
    int seen, attacking, hits, misses; float cool, throw_t, contact, orbit, stagger, ignore;   /* skeleton throws, bat bites */
    float fade, st;                                                /* wraiths: how far gone, time in state */
    float hp, maxhp, shield, face;                                 /* face: where a shield-bearer is looking */
    float frozen, feared, slowed, swing;                           /* frost nova, dread, frost shard; a sword raised */
    int sub, guard; float ax, ay, bx, by, sub_t;                   /* wall skulls and spiders: a phase, an anchor, a way back */
} Mob;
typedef struct { int alive, kind, target; float x, y, z, vx, vy, life, dmg; uint32_t pierced; } Shot;
typedef struct { int alive, kind; float x, y, vx, vy, life, spin, dmg; } Bone;
typedef struct { int alive, type, cx, cy, seen, color; float x, y, phase; } Item;
typedef struct { int alive; float x, y, t, r; int big; } Boom;
typedef struct { int alive, kind; float x0, y0, z0, x1, y1, z1, t; } Beam;   /* lightning and soul drain, for a moment */
typedef struct { int alive; float x, y, t, dps; } Burn;                    /* hellfire on the floor */
typedef struct { int x, y, w, h, region, theme; } Room;

#define MAXDOORS 64
#define MAXLIGHTS 64
#define MAXLC 140000
#define MAXDECO 400
#define MAXMOBS 20
#define MAXSHOTS 16
#define MAXBOOMS 8
#define MAXBONES 10
#define MAXITEMS 64
#define MAXBEAMS 8
#define MAXBURNS 4
#define BONE_SPEED 5.5f
#define FIRE_SPEED 4.5f
#define READOUT_T 3.6f
#define FACE_T 2.6f

static const int DX[4] = { 1, 0, -1, 0 }, DY[4] = { 0, 1, 0, -1 };
static const uint8_t BAYER[4][4] = { {0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5} };

static const char *const LEVEL_NAMES[] = {
    "THE OSSUARY", "HALL OF TORCHES", "CRYPT OF THE NAMELESS", "THE BONE GALLERY",
    "THE DROWNED VAULT", "CATACOMB OF SAINTS", "THE CHARNEL HOUSE", "WARRENS OF THE DEAD",
};

/* The run: what carries from level to level, through death and across scene switches */
static struct {
    int active, level, hp, treasure, souls;
    float soul_frac;
    Gear gear;
    int goal;                               /* gold it is saving towards (0: not saving) */
    int shop_next;                          /* the merchant comes first when the level is (re)entered */
    int loaded;                             /* just read from a save: nothing in G to carry over */
} RUN;

static struct {
    uint32_t rs;
    double t, level_t;
    int level;
    Cell map[MAPN][MAPN];
    Room rooms[12]; int nrooms;
    Door doors[MAXDOORS]; int ndoors;
    Light lights[MAXLIGHTS]; int nlights;
    uint16_t lc_idx[MAXLC]; float lc_w[MAXLC]; int nlc;
    float lm[LMN * LMN];
    Deco decos[MAXDECO]; int ndecos;
    uint8_t bulky[MAPN][MAPN];          /* cells holding something big to walk around */
    float fog[MAPN][MAPN];                  /* fog banks, 0..1, thicker the deeper the level */
    Mob mobs[MAXMOBS];
    Shot shots[MAXSHOTS];
    Boom booms[MAXBOOMS];
    Bone bones[MAXBONES];
    Beam beams[MAXBEAMS];
    Burn burns[MAXBURNS];
    Item items[MAXITEMS]; int nitems, items_total, items_got, value_total, value_got;
    int nvaults, vault_room[NKEYS], vault_door[NKEYS];   /* one locked room per key colour, in order */
    int ncells, explored;

    /* player */
    float px, py, ang, spd, bob, bobph, camz;
    int cx, cy, h, tx, ty, next_h, state, fired, target, spell;
    float u, st, ang0, ang1, adur, hand, cooldown, calm, cast_len;
    int resume_state;
    float blocked;                      /* seconds spent waiting on a creature */

    float pvx, pvy;                     /* the caster's velocity, for skeletons leading their throws */
    int hp, max_hp, level_treasure, level_souls, has_key[NKEYS];
    double unlock_t;

    /* what is on the caster: poison, slow, and the defensive spells running */
    float poison_t, poison_dps, poison_acc, slow_t, purify_t, regen_acc;
    float ward_hp, ward_t, mirror_t, slip_t, reap_t, mend_t, mend_rate;
    float def_cd[NDEF], def_wait[NDEF], def_hush[NDEF], rune_glow[NDEF];   /* cooldown; a cast on its way; a missed cue; glow on the hand */
    float bfs_t;                        /* the BFS9000's implosion, playing */
    float nova_t, dread_t, rune_lift;   /* rune_lift: the hand comes up to show a rune burning */              /* the frost nova bursting; Dread's shadow going out */
    float face_t, grin_t; int face_side;    /* the portrait popping in, the grin, which way it flinched */

    /* the way out: a door set in a wall, opened from the floor cell in front of it */
    int exit_wx, exit_wy, exit_fx, exit_fy, exit_dir, exit_in_vault, exit_seen;
    int leaving, exiting;                   /* heading for the exit; stepping through it */
    float exit_open, exit_t;
    int kills, kills_total;
    int tally;                              /* the level-complete screen */
    double tally_t, clear_time;
    int tally_shown[4], tally_done;
    float hurt, gold, readout, kick, heartbeat, chill;              /* chill: the cold flash when a wraith shrieks */
    int dead;
    double dead_t;

    /* the merchant's antechamber, before each level */
    int shop, shop_step, shop_pick, shop_bought, shop_look, shop_gold, shop_saving, shop_heal;
    Offer offers[NPOOL];
    int offer_gone[NPOOL];
    double shop_t;
    float sa0, sa1, sdur, sx0, sy0, sx1, sy1, smove, sparkle;    /* turning and walking: from, to, how long */
    float cat_blink, cat_slow, cat_ear, cat_trill, cat_look, purr_t, cat_next_blink, cat_next_ear, cat_tail_k;
    int cat_ear_side;

    /* a person at the controls */
    int player, ui, ui_sel, quit_line, status_ret;
    int graves_mode, graves_arm, graves_msg;    /* the save slots: carving (0) or rising (1); a second Enter's slot; why */
    float pc_t, pc_len, grab_t, hint_t, msg_t;     /* their cast; reaching for a ware; the hints; a message */
    int pc_fired;
    char msg[64];
    Gear inv0;                              /* the gear as it was when the inventory opened */

    float fade, card;
    int ending;
    double end_t;
    float mood_t;
} G;
static float zbuf[W];
enum { UI_NONE, UI_INVENTORY, UI_QUIT, UI_GRAVES };   /* the menus a person can open */
static Input IN;                            /* their keys and mouse, this step */

static uint8_t ramp_at(int ramp, int k) { return (uint8_t)(ramp + clampi(k, 0, ramp == R_GOLD ? 12 : 15)); }   /* the gold ramp is short */
static int rnd(int n) { return (int)(rng_next(&G.rs) % (uint32_t)n); }
static float decay(void) { return clampf((G.level - 1) / 7.0f, 0.0f, 1.0f); }   /* 0 on level 1, 1 by level 8 */
static int enemy_damage(void) { return 4 + G.level; }                            /* 5 on level 1, one more each level */
static float throw_interval(void) { return 1.5f / powf(1.05f, (float)(G.level - 1)); }   /* 5% quicker each level */
static float rndf(void) { return rng_f(&G.rs); }
static uint32_t hash2(int x, int y) {
    uint32_t h = (uint32_t)x * 73856093u ^ (uint32_t)y * 19349663u ^ (uint32_t)G.level * 83492791u;
    h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
    return h;
}

static Cell *cell(int x, int y) {
    static Cell solid = { K_WALL, T_STONE, 0, 0, -1, -1 };
    if (x < 0 || y < 0 || x >= MAPN || y >= MAPN) return &solid;
    return &G.map[y][x];
}
static int is_open(int x, int y) { int k = cell(x, y)->kind; return k == K_FLOOR || k == K_DOOR; }
static int blocks_light(int x, int y) {
    Cell *c = cell(x, y);
    return c->kind == K_WALL || (c->kind == K_DOOR && G.doors[c->door].open < 0.5f);
}
static int blocks_move(int x, int y) {
    Cell *c = cell(x, y);
    return c->kind == K_WALL || c->kind == K_PILLAR || (c->kind == K_DOOR && G.doors[c->door].open < 0.9f);
}
/* where the caster may walk: a vault door only once its key is in hand */
static int passable(int x, int y) {
    Cell *c = cell(x, y);
    if (c->kind == K_FLOOR) return 1;
    if (c->kind != K_DOOR) return 0;
    const Door *d = &G.doors[c->door];
    return d->locked != 1 || G.has_key[d->color];
}
static int vault_of_room(int x, int y);
static Mob *spawn_mob(int x, int y, int type);
static float heading_angle_raw(int h) { return h * (PI_F * 0.5f) + PI_F; }   /* facing back the other way */
static float wrap_pi(float a) { while (a > PI_F) a -= TAU_F; while (a < -PI_F) a += TAU_F; return a; }

/* ================================================================== */
/* Level generation: rooms and mazes                                   */
/* ================================================================== */

static int uf[1024];
static int uf_find(int a) { while (uf[a] != a) a = uf[a] = uf[uf[a]]; return a; }

static void carve(int x, int y, int region, int theme) {
    Cell *c = &G.map[y][x];
    c->kind = K_FLOOR; c->region = (int16_t)region; c->theme = (uint8_t)theme;
}

static void grow_maze(int sx, int sy, int region) {
    static int st[600][2];
    int sp = 0, last = -1;
    carve(sx, sy, region, TH_CORRIDOR);
    st[sp][0] = sx; st[sp][1] = sy; sp++;
    while (sp) {
        int x = st[sp - 1][0], y = st[sp - 1][1], avail[4], na = 0;
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d] * 2, ny = y + DY[d] * 2;
            if (nx >= 1 && ny >= 1 && nx < MAPN - 1 && ny < MAPN - 1 && G.map[ny][nx].kind == K_WALL) avail[na++] = d;
        }
        if (!na) { sp--; last = -1; continue; }
        int d = avail[rnd(na)];
        for (int i = 0; i < na; i++) if (avail[i] == last && rndf() < 0.45f) d = last;
        carve(x + DX[d], y + DY[d], region, TH_CORRIDOR);
        carve(x + DX[d] * 2, y + DY[d] * 2, region, TH_CORRIDOR);
        if (sp < 600) { st[sp][0] = x + DX[d] * 2; st[sp][1] = y + DY[d] * 2; sp++; }
        last = d;
    }
}

static int is_room_region(int r) {
    for (int i = 0; i < G.nrooms; i++) if (G.rooms[i].region == r) return 1;
    return 0;
}

static int theme_tex(int theme, uint32_t h) {
    int k = (int)(h % 10);
    switch (theme) {
    case TH_OSSUARY: return k < 6 ? T_SKULLS : (k < 9 ? T_NICHE : T_STONE);
    case TH_CRYPT:   return k < 5 ? T_STONE : (k < 8 ? T_NICHE : T_BANNER);
    case TH_BRICK:   return k < 8 ? T_BRICK : T_BANNER;
    case TH_MOSS:    return k < 6 ? T_MOSS : T_RUBBLE;
    default:         return k < 6 ? T_STONE : (k < 9 ? T_RUBBLE : T_SKULLS);
    }
}

static void add_deco(float x, float y, int spr) {
    if (G.ndecos < MAXDECO) G.decos[G.ndecos++] = (Deco){ x, y, spr };
    if (spr == SP_SKULLPILE || spr == SP_BONES2 || spr == SP_URN || spr == SP_CANDLES || spr == SP_CHAINS)
        G.bulky[clampi((int)y, 0, MAPN - 1)][clampi((int)x, 0, MAPN - 1)] = 1;
}

/* static light: record its contribution to every light-map texel it reaches
 * through open space (and the faces of the walls around that space) */
static void light_static(float lx, float ly, float power, float radius) {
    if (G.nlights >= MAXLIGHTS) return;
    Light *L = &G.lights[G.nlights++];
    L->x = lx; L->y = ly; L->power = power; L->radius = radius; L->phase = rndf() * 10.0f;
    L->cx = (int)lx; L->cy = (int)ly; L->first = G.nlc; L->count = 0;
    static int stamp[MAPN][MAPN], qx[MAPN * MAPN], qy[MAPN * MAPN], dist[MAPN][MAPN];
    static int gen;
    gen++;
    int qh = 0, qt = 0, maxd = (int)ceilf(radius) + 1;
    qx[qt] = L->cx; qy[qt] = L->cy; qt++; stamp[L->cy][L->cx] = gen; dist[L->cy][L->cx] = 0;
    while (qh < qt) {
        int x = qx[qh], y = qy[qh]; qh++;
        if (dist[y][x] >= maxd) continue;
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (nx < 0 || ny < 0 || nx >= MAPN || ny >= MAPN || stamp[ny][nx] == gen) continue;
            stamp[ny][nx] = gen; dist[ny][nx] = dist[y][x] + 1;
            if (G.map[ny][nx].kind != K_WALL) { qx[qt] = nx; qy[qt] = ny; qt++; }
            else { qx[qt] = nx; qy[qt] = ny; qt++; dist[ny][nx] = maxd; }   /* lit face, no further */
        }
    }
    for (int i = 0; i < qt; i++) {
        int x = qx[i], y = qy[i];
        for (int ty = 0; ty < LMR; ty++)
            for (int tx = 0; tx < LMR; tx++) {
                float wx = x + (tx + 0.5f) / LMR, wy = y + (ty + 0.5f) / LMR;
                float dd = sqrtf((wx - lx) * (wx - lx) + (wy - ly) * (wy - ly));
                if (dd >= radius || G.nlc >= MAXLC) continue;
                float w = power / (1.0f + 0.9f * dd * dd) * (1.0f - smoothstepf(radius * 0.55f, radius, dd));
                G.lc_idx[G.nlc] = (uint16_t)((y * LMR + ty) * LMN + x * LMR + tx);
                G.lc_w[G.nlc] = w;
                G.nlc++; L->count++;
            }
    }
}

static void fill_dead_ends(void) {
    for (int changed = 1; changed; ) {
        changed = 0;
        for (int y = 1; y < MAPN - 1; y++)
            for (int x = 1; x < MAPN - 1; x++) {
                if (!is_open(x, y)) continue;
                int n = 0;
                for (int d = 0; d < 4; d++) n += is_open(x + DX[d], y + DY[d]);
                if (n <= 1) { G.map[y][x].kind = K_WALL; G.map[y][x].region = -1; changed = 1; }
            }
    }
}

static int in_room(const Room *R, int x, int y) { return x >= R->x && y >= R->y && x < R->x + R->w && y < R->y + R->h; }

/* Make one room a vault with a single way in. Its other entrances are walled
 * up, but only if the rest of the level stays connected without the room. */
static int choose_vault(int *kx, int *ky, int *room) {
    int order[12], no = 0;
    for (int r = 1; r < G.nrooms; r++) {
        int taken = 0;
        for (int k = 0; k < G.nvaults; k++) taken |= G.vault_room[k] == r;
        if (!taken) order[no++] = r;
    }
    for (int i = no - 1; i > 0; i--) { int j = rnd(i + 1), t = order[i]; order[i] = order[j]; order[j] = t; }
    int sx = G.rooms[0].x, sy = G.rooms[0].y;
    for (int oi = 0; oi < no; oi++) {
        Room *R = &G.rooms[order[oi]];
        if (R->w * R->h < 9) continue;
        int ex[24], ey[24], ed[24], ne = 0;
        for (int y = R->y; y < R->y + R->h; y++)
            for (int x = R->x; x < R->x + R->w; x++)
                for (int d = 0; d < 4; d++) {
                    int nx = x + DX[d], ny = y + DY[d];
                    if (in_room(R, nx, ny) || !is_open(nx, ny) || ne >= 24) continue;
                    ex[ne] = nx; ey[ne] = ny; ed[ne] = d; ne++;
                }
        if (!ne || ne > 8) continue;
        int keep = -1;
        for (int i = 0; i < ne; i++) {
            int d = ed[i], p = (d + 1) & 3;
            if (is_open(ex[i] + DX[p], ey[i] + DY[p]) || is_open(ex[i] - DX[p], ey[i] - DY[p])) continue;
            if (!is_open(ex[i] + DX[d], ey[i] + DY[d])) continue;
            if (keep < 0 || G.map[ey[i]][ex[i]].kind == K_DOOR) keep = i;
        }
        if (keep < 0) continue;
        uint8_t saved[24];
        for (int i = 0; i < ne; i++) { saved[i] = G.map[ey[i]][ex[i]].kind; if (i != keep) G.map[ey[i]][ex[i]].kind = K_WALL; }
        /* everything outside the vault must still be reachable from the start */
        static uint8_t seen[MAPN][MAPN]; static int qx[MAPN * MAPN], qy[MAPN * MAPN];
        memset(seen, 0, sizeof seen);
        int qh = 0, qt = 0, total = 0;
        for (int y = 0; y < MAPN; y++)
            for (int x = 0; x < MAPN; x++)
                if (is_open(x, y) && !in_room(R, x, y) && !(x == ex[keep] && y == ey[keep])) total++;
        seen[sy][sx] = 1; qx[qt] = sx; qy[qt] = sy; qt++;
        while (qh < qt) {
            int x = qx[qh], y = qy[qh]; qh++;
            for (int d = 0; d < 4; d++) {
                int nx = x + DX[d], ny = y + DY[d];
                if (seen[ny][nx] || !is_open(nx, ny) || in_room(R, nx, ny) || (nx == ex[keep] && ny == ey[keep])) continue;
                seen[ny][nx] = 1; qx[qt] = nx; qy[qt] = ny; qt++;
            }
        }
        if (qt == total) {
            for (int i = 0; i < ne; i++) if (i != keep) G.map[ey[i]][ex[i]].region = -1;
            G.map[ey[keep]][ex[keep]].kind = K_DOOR;
            *room = order[oi];
            *kx = ex[keep]; *ky = ey[keep];
            return 1;
        }
        for (int i = 0; i < ne; i++) G.map[ey[i]][ex[i]].kind = saved[i];
    }
    return 0;
}

static void gen_level(void) {
    memset(G.map, 0, sizeof G.map);
    for (int y = 0; y < MAPN; y++)
        for (int x = 0; x < MAPN; x++) { G.map[y][x].kind = K_WALL; G.map[y][x].region = -1; G.map[y][x].door = -1; }
    G.nrooms = 0; G.ndoors = 0; G.nlights = 0; G.nlc = 0; G.ndecos = 0;
    memset(G.bulky, 0, sizeof G.bulky);
    int nreg = 0;

    for (int tries = 0; tries < 300 && G.nrooms < 9; tries++) {
        int w = 3 + 2 * rnd(3), h = 3 + 2 * rnd(3);
        if (rnd(4) == 0) { w = 7; h = 5 + 2 * rnd(2); }
        int x = 1 + 2 * rnd((MAPN - w - 1) / 2 + 1), y = 1 + 2 * rnd((MAPN - h - 1) / 2 + 1);
        if (x + w >= MAPN - 1 || y + h >= MAPN - 1) continue;
        int ok = 1;
        for (int yy = y - 1; yy <= y + h && ok; yy++)
            for (int xx = x - 1; xx <= x + w; xx++) if (G.map[yy][xx].kind != K_WALL) { ok = 0; break; }
        if (!ok) continue;
        int theme = 1 + rnd(TH_COUNT - 1);
        for (int yy = y; yy < y + h; yy++) for (int xx = x; xx < x + w; xx++) carve(xx, yy, nreg, theme);
        G.rooms[G.nrooms++] = (Room){ x, y, w, h, nreg, theme };
        nreg++;
    }
    for (int y = 1; y < MAPN - 1; y += 2)
        for (int x = 1; x < MAPN - 1; x += 2)
            if (G.map[y][x].kind == K_WALL && nreg < 1000) grow_maze(x, y, nreg++);

    /* connect every region, mostly as a tree, with a few extra loops */
    static int con[2000][4];
    int nc = 0;
    for (int y = 1; y < MAPN - 1; y++)
        for (int x = 1; x < MAPN - 1; x++) {
            if (G.map[y][x].kind != K_WALL || nc >= 2000) continue;
            int a = -1, b = -1;
            if (G.map[y][x - 1].kind == K_FLOOR && G.map[y][x + 1].kind == K_FLOOR) { a = G.map[y][x - 1].region; b = G.map[y][x + 1].region; }
            else if (G.map[y - 1][x].kind == K_FLOOR && G.map[y + 1][x].kind == K_FLOOR) { a = G.map[y - 1][x].region; b = G.map[y + 1][x].region; }
            if (a >= 0 && b >= 0 && a != b) { con[nc][0] = x; con[nc][1] = y; con[nc][2] = a; con[nc][3] = b; nc++; }
        }
    for (int i = nc - 1; i > 0; i--) {
        int j = rnd(i + 1);
        for (int k = 0; k < 4; k++) { int t = con[i][k]; con[i][k] = con[j][k]; con[j][k] = t; }
    }
    for (int i = 0; i < nreg; i++) uf[i] = i;
    for (int pass = 0; pass < 2; pass++)
        for (int i = 0; i < nc; i++) {
            int x = con[i][0], y = con[i][1], a = con[i][2], b = con[i][3];
            if (G.map[y][x].kind != K_WALL) continue;
            int ra = uf_find(a), rb = uf_find(b);
            int extra = pass == 1 && rnd(100) < 7;
            if (ra == rb && !extra) continue;
            if (extra) {
                int near = 0;
                for (int d = 0; d < 4; d++) if (cell(x + DX[d] * 2, y + DY[d] * 2)->kind == K_DOOR) near = 1;
                if (near) continue;
            }
            uf[ra] = rb;
            int room_side = is_room_region(a) || is_room_region(b);
            G.map[y][x].kind = (room_side && rnd(10) < 8) ? K_DOOR : K_FLOOR;
            G.map[y][x].region = (int16_t)(is_room_region(a) ? b : a);
            G.map[y][x].theme = TH_CORRIDOR;
        }

    fill_dead_ends();                                             /* so the walk rarely has to turn back */
    /* the locked rooms: gold always, silver from level 3, crimson from level 5 */
    int want = G.level >= 5 ? 3 : (G.level >= 3 ? 2 : 1), vkx[NKEYS], vky[NKEYS];
    G.nvaults = 0;
    for (int k = 0; k < NKEYS; k++) G.vault_room[k] = G.vault_door[k] = -1;
    for (int k = 0; k < want; k++) {
        int room;
        if (!choose_vault(&vkx[k], &vky[k], &room)) break;
        G.vault_room[k] = room; G.nvaults++;
        fill_dead_ends();
    }

    /* doors: only where they sit between two walls, never next to each other */
    for (int y = 1; y < MAPN - 1; y++)
        for (int x = 1; x < MAPN - 1; x++) {
            Cell *c = &G.map[y][x];
            if (c->kind != K_DOOR) continue;
            int ax = is_open(x - 1, y) && is_open(x + 1, y) && !is_open(x, y - 1) && !is_open(x, y + 1);
            int ay = is_open(x, y - 1) && is_open(x, y + 1) && !is_open(x - 1, y) && !is_open(x + 1, y);
            int adj = 0, vault = -1;
            for (int k = 0; k < G.nvaults; k++) if (x == vkx[k] && y == vky[k]) vault = k;
            for (int d = 0; d < 4; d++) if (cell(x + DX[d], y + DY[d])->kind == K_DOOR) adj = 1;
            if (vault < 0 && ((!ax && !ay) || adj || G.ndoors >= MAXDOORS - 1)) { c->kind = K_FLOOR; continue; }
            c->door = (int16_t)G.ndoors;
            G.doors[G.ndoors++] = (Door){ x, y, ax, 0.0f, 0.0f, 0, vault >= 0, vault >= 0 ? vault : 0 };
            if (vault >= 0) G.vault_door[vault] = G.ndoors - 1;
        }

    /* wall textures from the neighbouring room's theme, or the corridor zone */
    for (int y = 0; y < MAPN; y++)
        for (int x = 0; x < MAPN; x++) {
            Cell *c = &G.map[y][x];
            if (c->kind != K_WALL) continue;
            int theme = -1;
            for (int d = 0; d < 4; d++) {
                Cell *n = cell(x + DX[d], y + DY[d]);
                if (n->kind == K_FLOOR && n->theme != TH_CORRIDOR) theme = n->theme;
            }
            if (theme < 0) {
                uint32_t z = hash2(x / 7 + 101, y / 7 + 57) % 10;
                theme = z < 6 ? TH_CORRIDOR : (z < 8 ? TH_MOSS : TH_OSSUARY);
            }
            c->tex = (uint8_t)theme_tex(theme, hash2(x, y));
        }

    /* torches along the halls */
    static int order[MAPN * MAPN];
    int no = 0;
    for (int i = 0; i < MAPN * MAPN; i++) order[no++] = i;
    for (int i = no - 1; i > 0; i--) { int j = rnd(i + 1), t = order[i]; order[i] = order[j]; order[j] = t; }
    for (int i = 0; i < no && G.nlights < MAXLIGHTS - 12; i++) {
        int x = order[i] % MAPN, y = order[i] / MAPN;
        if (G.map[y][x].kind != K_FLOOR) continue;
        int near = 0;
        for (int l = 0; l < G.nlights; l++) if (abs(G.lights[l].cx - x) + abs(G.lights[l].cy - y) < 5) near = 1;
        if (near || rndf() > 0.55f - 0.2f * decay()) continue;      /* fewer torches the deeper you go */
        int d = rnd(4);
        for (int k = 0; k < 4; k++, d = (d + 1) & 3) {
            int wx = x + DX[d], wy = y + DY[d];
            Cell *w = cell(wx, wy);
            if (w->kind != K_WALL || wx <= 0 || wy <= 0 || wx >= MAPN - 1 || wy >= MAPN - 1) continue;
            int bydoor = 0;
            for (int e = 0; e < 4; e++) if (cell(wx + DX[e], wy + DY[e])->kind == K_DOOR) bydoor = 1;
            if (bydoor) continue;
            w->tex = T_TORCH0;
            light_static(x + 0.5f + DX[d] * 0.42f, y + 0.5f + DY[d] * 0.42f, 1.55f, 4.6f);
            break;
        }
    }

    /* decorations */
    for (int y = 1; y < MAPN - 1; y++)
        for (int x = 1; x < MAPN - 1; x++) {
            Cell *c = &G.map[y][x];
            if (c->kind != K_FLOOR) continue;
            int o[4];
            for (int d = 0; d < 4; d++) o[d] = is_open(x + DX[d], y + DY[d]);
            int n = o[0] + o[1] + o[2] + o[3];
            float cx = x + 0.5f, cy = y + 0.5f;
            if (c->theme == TH_CORRIDOR) {
                if (n == 2 && o[0] && o[2]) {                         /* east-west hall */
                    float side = rnd(2) ? 0.31f : -0.31f;
                    if (rndf() < 0.11f + 0.1f * decay()) add_deco(cx + (rndf() - 0.5f) * 0.3f, cy + side, rnd(4) ? SP_BONES1 : SP_BONES2);
                    else if (rndf() < 0.03f) add_deco(cx, cy + side * 1.1f, SP_CHAINS);
                } else if (n == 2 && o[1] && o[3]) {                  /* north-south hall */
                    float side = rnd(2) ? 0.31f : -0.31f;
                    if (rndf() < 0.11f + 0.1f * decay()) add_deco(cx + side, cy + (rndf() - 0.5f) * 0.3f, rnd(4) ? SP_BONES1 : SP_BONES2);
                    else if (rndf() < 0.03f) add_deco(cx + side * 1.1f, cy, SP_CHAINS);
                } else if (n == 2 && rndf() < 0.5f + 0.35f * decay()) { /* a corner: cobweb in the dead corner */
                    float ox = o[0] ? -0.34f : 0.34f, oy = o[1] ? -0.34f : 0.34f;
                    add_deco(cx + ox, cy + oy, SP_COBWEB);
                }
            }
        }
    for (int r = 0; r < G.nrooms; r++) {
        Room *R = &G.rooms[r];
        if (R->w >= 5 && R->h >= 5) {                                 /* pillars */
            int px[4] = { R->x + 1, R->x + R->w - 2, R->x + 1, R->x + R->w - 2 };
            int py[4] = { R->y + 1, R->y + 1, R->y + R->h - 2, R->y + R->h - 2 };
            for (int i = 0; i < 4; i++) {                              /* corner pillars never cut a room in two */
                G.map[py[i]][px[i]].kind = K_PILLAR;
                add_deco(px[i] + 0.5f, py[i] + 0.5f, SP_PILLAR);
            }
        }
        int candles = rnd(3) == 0;
        for (int y = R->y; y < R->y + R->h; y++)
            for (int x = R->x; x < R->x + R->w; x++) {
                if (G.map[y][x].kind != K_FLOOR) continue;
                for (int d = 0; d < 4; d++) {
                    int wx = x + DX[d], wy = y + DY[d];
                    if (cell(wx, wy)->kind != K_WALL || rndf() > 0.16f + 0.1f * decay()) continue;
                    int beside_way = 0;
                    for (int e = 0; e < 4; e++) {
                        int nx = x + DX[e], ny = y + DY[e];
                        if (is_open(nx, ny) && (nx < R->x || ny < R->y || nx >= R->x + R->w || ny >= R->y + R->h)) beside_way = 1;
                    }
                    if (beside_way) continue;
                    int spr;
                    int k = rnd(10);
                    if (candles && k == 0) {
                        spr = SP_CANDLES; candles = 0;
                        light_static(x + 0.5f + DX[d] * 0.3f, y + 0.5f + DY[d] * 0.3f, 0.95f, 3.0f);
                    } else if (R->theme == TH_OSSUARY) spr = k < 5 ? SP_SKULLPILE : SP_BONES2;
                    else spr = k < 3 ? SP_URN : (k < 6 ? SP_BONES2 : (k < 8 ? SP_SKULLPILE : SP_BONES1));
                    add_deco(x + 0.5f + DX[d] * 0.3f, y + 0.5f + DY[d] * 0.3f, spr);
                    break;
                }
            }
    }

    G.ncells = 0;
    for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) if (G.map[y][x].kind == K_FLOOR) G.ncells++;

    /* fog banks: smooth blobs from a coarse random lattice; a few wisps at first, thick by level 8 */
    float lat[8][8];
    for (int j = 0; j < 8; j++) for (int i = 0; i < 8; i++) lat[j][i] = rndf();
    float dk = decay(), thr = 0.72f - 0.32f * dk, amt = 0.4f + 0.45f * dk;
    for (int y = 0; y < MAPN; y++)
        for (int x = 0; x < MAPN; x++) {
            float fx = x / 4.8f, fy = y / 4.8f;
            int ix = (int)fx, iy = (int)fy;
            float ax = fx - ix, ay = fy - iy;
            ax = ax * ax * (3 - 2 * ax); ay = ay * ay * (3 - 2 * ay);
            float n = lerpf(lerpf(lat[iy][ix], lat[iy][ix + 1], ax), lerpf(lat[iy + 1][ix], lat[iy + 1][ix + 1], ax), ay);
            G.fog[y][x] = clampf((n - thr) * 3.2f, 0.0f, 1.0f) * amt;
        }
}

/* ================================================================== */
/* Treasure, the key, a potion                                         */
/* ================================================================== */

static int item_at(int x, int y) {
    for (int i = 0; i < G.nitems; i++) if (G.items[i].alive && G.items[i].cx == x && G.items[i].cy == y) return 1;
    return 0;
}

static int free_floor(int x, int y) {
    if (cell(x, y)->kind != K_FLOOR || G.bulky[y][x] || item_at(x, y)) return 0;
    for (int d = 0; d < 4; d++) if (cell(x + DX[d], y + DY[d])->kind == K_DOOR) return 0;
    return 1;
}

/* treasure is worth more the deeper it lies */
static int treasure_value(int type) { return (int)(ITEM_VALUE[type] * (1.0f + 0.2f * (float)(G.level - 1)) + 0.5f); }

static int vault_of_room(int x, int y) {
    for (int k = 0; k < G.nvaults; k++) if (G.vault_room[k] >= 0 && in_room(&G.rooms[G.vault_room[k]], x, y)) return k;
    return -1;
}

static Item *add_item(int x, int y, int type) {
    if (G.nitems >= MAXITEMS) return NULL;
    Item *it = &G.items[G.nitems++];
    memset(it, 0, sizeof *it);
    it->alive = 1; it->type = type; it->cx = x; it->cy = y;
    it->x = x + 0.5f + (rndf() - 0.5f) * 0.16f; it->y = y + 0.5f + (rndf() - 0.5f) * 0.16f;
    it->phase = rndf() * 10.0f;
    if (type < IT_KEY) { G.items_total++; G.value_total += treasure_value(type); }
    return it;
}

static int random_treasure(void) { int k = rnd(20); return k < 9 ? IT_COINS : (k < 14 ? IT_CHALICE : (k < 18 ? IT_GEMS : IT_CROWN)); }

/* The final door: set in a wall at the far end of the level, or now and then inside the vault. */
static void place_exit(int sx, int sy) {
    static int dist[MAPN][MAPN], qx[MAPN * MAPN], qy[MAPN * MAPN];
    for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) dist[y][x] = -1;
    int qh = 0, qt = 0, maxd = 0;
    dist[sy][sx] = 0; qx[qt] = sx; qy[qt] = sy; qt++;
    while (qh < qt) {
        int x = qx[qh], y = qy[qh]; qh++;
        if (dist[y][x] > maxd) maxd = dist[y][x];
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (!is_open(nx, ny) || dist[ny][nx] >= 0) continue;
            dist[ny][nx] = dist[y][x] + 1; qx[qt] = nx; qy[qt] = ny; qt++;
        }
    }
    int want_vault = G.nvaults > 0 && rnd(100) < 45, xv = G.nvaults > 0 ? rnd(G.nvaults) : -1;
    for (int pass = 0; pass < 2; pass++) {
        int best = -1, bd = 0; float bs = -1e9f;
        for (int y = 1; y < MAPN - 1; y++)
            for (int x = 1; x < MAPN - 1; x++) {
                if (G.map[y][x].kind != K_FLOOR || dist[y][x] < 0 || G.bulky[y][x] || (x == sx && y == sy)) continue;
                int inv = vault_of_room(x, y);
                if (want_vault ? inv != xv : inv >= 0) continue;
                for (int d = 0; d < 4; d++) {
                    int wx = x + DX[d], wy = y + DY[d];
                    Cell *w = cell(wx, wy);
                    if (w->kind != K_WALL || (w->tex >= T_TORCH0 && w->tex <= T_TORCH2) || wx < 0 || wy < 0 || wx >= MAPN || wy >= MAPN) continue;
                    int open = 0;
                    for (int e = 0; e < 4; e++) open += is_open(wx + DX[e], wy + DY[e]) || cell(wx + DX[e], wy + DY[e])->kind == K_PILLAR;
                    if (open != 1) continue;                           /* only one face, so it can't be seen from behind */
                    float sc = want_vault ? rndf() : (float)dist[y][x] + rndf() * 5.0f;
                    if (sc > bs) { bs = sc; best = (y * MAPN + x) * 4 + d; bd = d; }
                }
            }
        if (best < 0) { want_vault = 0; continue; }
        int x = (best / 4) % MAPN, y = (best / 4) / MAPN;
        G.exit_fx = x; G.exit_fy = y; G.exit_dir = bd;
        G.exit_wx = x + DX[bd]; G.exit_wy = y + DY[bd];
        G.exit_in_vault = want_vault ? xv : -1;
        G.map[G.exit_wy][G.exit_wx].tex = T_EXIT0;
        light_static(x + 0.5f + DX[bd] * 0.35f, y + 0.5f + DY[bd] * 0.35f, 0.9f, 3.2f);   /* its runes light the stone */
        return;
    }
    G.exit_fx = G.exit_fy = -1;
}

static int room_cell(int room, int *ox, int *oy) {                 /* a free floor cell in a room */
    const Room *R = &G.rooms[room];
    for (int tries = 0; tries < 80; tries++) {
        int x = R->x + rnd(R->w), y = R->y + rnd(R->h);
        if (G.map[y][x].kind != K_FLOOR || G.bulky[y][x] || item_at(x, y)) continue;
        *ox = x; *oy = y;
        return 1;
    }
    return 0;
}

static void place_items(int sx, int sy) {
    G.nitems = 0; G.items_total = 0; G.items_got = 0; G.value_total = G.value_got = 0;
    static int dist[MAPN][MAPN], qx[MAPN * MAPN], qy[MAPN * MAPN];
    for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) dist[y][x] = -1;
    int qh = 0, qt = 0, maxd = 0;
    dist[sy][sx] = 0; qx[qt] = sx; qy[qt] = sy; qt++;
    while (qh < qt) {                                              /* reachable without any key */
        int x = qx[qh], y = qy[qh]; qh++;
        if (dist[y][x] > maxd) maxd = dist[y][x];
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (!passable(nx, ny) || dist[ny][nx] >= 0) continue;
            dist[ny][nx] = dist[y][x] + 1; qx[qt] = nx; qy[qt] = ny; qt++;
        }
    }
    for (int r = 1; r < G.nrooms; r++) {                           /* a little in most rooms, more deeper down */
        if (vault_of_room(G.rooms[r].x, G.rooms[r].y) >= 0) continue;
        Room *R = &G.rooms[r];
        int n = rnd(3) + (G.level >= 4);
        for (int k = 0, tries = 0; k < n && tries < 30; tries++) {
            int x = R->x + rnd(R->w), y = R->y + rnd(R->h);
            if (dist[y][x] < 0 || !free_floor(x, y)) continue;
            add_item(x, y, random_treasure()); k++;
        }
    }
    for (int i = 0, placed = 0; i < 400 && placed < 7 + G.level / 2; i++) {   /* and along the halls */
        int x = 1 + rnd(MAPN - 2), y = 1 + rnd(MAPN - 2);
        if (G.map[y][x].theme != TH_CORRIDOR || dist[y][x] < 4 || !free_floor(x, y)) continue;
        add_item(x, y, rnd(3) ? IT_COINS : IT_CHALICE); placed++;
    }
    static const int HOARD[10] = { IT_CHEST, IT_CROWN, IT_GEMS, IT_COINS, IT_COINS, IT_CHALICE, IT_GEMS, IT_CHALICE, IT_CROWN, IT_GEMS };
    for (int k = 0; k < G.nvaults; k++) {                          /* the hoards: richer behind silver, richer still behind crimson */
        int n = k == 0 ? 6 : (k == 1 ? 8 : 10), x, y;
        for (int i = 0; i < n; i++) if (room_cell(G.vault_room[k], &x, &y)) add_item(x, y, HOARD[i]);
        for (int i = 0; i < k + 1 && k >= 1; i++) if (room_cell(G.vault_room[k], &x, &y)) add_item(x, y, IT_URN);
        if (k == 2 && rnd(4) == 0 && room_cell(G.vault_room[k], &x, &y)) add_item(x, y, IT_RING);
    }
    /* the keys: usually out in the halls, a short walk from their door; now and then locked in the vault before */
    static int ddist[MAPN][MAPN];
    for (int k = 0; k < G.nvaults; k++) {
        int x, y;
        if (k > 0 && rnd(100) < 35 && room_cell(G.vault_room[k - 1], &x, &y)) { Item *it = add_item(x, y, IT_KEY); if (it) it->color = k; continue; }
        const Door *vd = &G.doors[G.vault_door[k]];
        for (int yy = 0; yy < MAPN; yy++) for (int xx = 0; xx < MAPN; xx++) ddist[yy][xx] = -1;
        qh = qt = 0;
        for (int d = 0; d < 4; d++) {
            int nx = vd->x + DX[d], ny = vd->y + DY[d];
            if (passable(nx, ny) && dist[ny][nx] >= 0) { ddist[ny][nx] = 1; qx[qt] = nx; qy[qt] = ny; qt++; }
        }
        while (qh < qt) {
            int xx = qx[qh], yy = qy[qh]; qh++;
            for (int d = 0; d < 4; d++) {
                int nx = xx + DX[d], ny = yy + DY[d];
                if (!passable(nx, ny) || ddist[ny][nx] >= 0) continue;
                ddist[ny][nx] = ddist[yy][xx] + 1; qx[qt] = nx; qy[qt] = ny; qt++;
            }
        }
        int best = -1; float bs = -1e9f;
        for (int yy = 1; yy < MAPN - 1; yy++)
            for (int xx = 1; xx < MAPN - 1; xx++) {
                if (dist[yy][xx] < maxd / 5 || ddist[yy][xx] < 0 || !free_floor(xx, yy)) continue;
                float sc = -fabsf(ddist[yy][xx] - 20.0f) + (G.map[yy][xx].theme != TH_CORRIDOR ? 4.0f : 0.0f) + rndf() * 6.0f
                        - (dist[yy][xx] > maxd * 3 / 5 ? 10.0f : 0.0f);
                if (sc > bs) { bs = sc; best = yy * MAPN + xx; }
            }
        if (best >= 0) { Item *it = add_item(best % MAPN, best / MAPN, IT_KEY); if (it) it->color = k; }
    }
    for (int i = 0; i < 300; i++) {                                /* one healing potion */
        int x = 1 + rnd(MAPN - 2), y = 1 + rnd(MAPN - 2);
        if (dist[y][x] < 6 || !free_floor(x, y)) continue;
        add_item(x, y, IT_POTION);
        break;
    }
}

/* wall skulls, from level 2: on a wall the caster will walk straight at, at the end of a run of hall */
static void place_wall_skulls(int sx, int sy) {
    if (G.level < 2) return;
    int want = 2 + (G.level >= 5) + rnd(2), placed = 0, px[6], py[6];
    for (int tries = 0; tries < 3000 && placed < want; tries++) {
        int x = 1 + rnd(MAPN - 2), y = 1 + rnd(MAPN - 2), d = rnd(4);
        if (G.map[y][x].kind != K_FLOOR || G.bulky[y][x] || vault_of_room(x, y) >= 0 || abs(x - sx) + abs(y - sy) < 6) continue;
        Cell *w = cell(x + DX[d], y + DY[d]);
        if (w->kind != K_WALL || w->tex >= T_TORCH0) continue;
        int run = 1;
        for (int k = 1; k <= 3; k++) run &= cell(x - DX[d] * k, y - DY[d] * k)->kind == K_FLOOR;
        if (!run) continue;
        int far = 1;
        for (int i = 0; i < placed; i++) if (abs(px[i] - x) + abs(py[i] - y) < 6) far = 0;
        if (!far) continue;
        Mob *m = spawn_mob(x, y, MOB_WSKULL);
        if (!m) break;
        m->x = x + 0.5f + DX[d] * 0.42f; m->y = y + 0.5f + DY[d] * 0.42f; m->z = 0.3f;
        m->face = heading_angle_raw(d);
        px[placed] = x; py[placed] = y; placed++;
    }
}

/* spiders, from level 4: in some of the corner webs */
static void place_spiders(int sx, int sy) {
    if (G.level < 4) return;
    int want = 2 + (G.level >= 6) + rnd(2), placed = 0;
    for (int tries = 0; tries < 60 && placed < want && G.ndecos; tries++) {
        const Deco *dc = &G.decos[rnd(G.ndecos)];
        if (dc->spr != SP_COBWEB || hypotf(dc->x - sx, dc->y - sy) < 5.0f) continue;
        int clash = 0;
        for (int i = 0; i < MAXMOBS; i++) if (G.mobs[i].alive && G.mobs[i].type == MOB_SPIDER && hypotf(G.mobs[i].ax - dc->x, G.mobs[i].ay - dc->y) < 4.0f) clash = 1;
        if (clash) continue;
        Mob *m = spawn_mob((int)dc->x, (int)dc->y, MOB_SPIDER);
        if (!m) break;
        m->ax = m->x = dc->x; m->ay = m->y = dc->y; m->z = 0.62f;
        placed++;
    }
}

/* the silver and crimson rooms are guarded */
static void place_guardians(void) {
    for (int k = 1; k < G.nvaults; k++) {
        int x, y;
        if (!room_cell(G.vault_room[k], &x, &y)) continue;
        if (G.level >= 4 && rnd(5) < 2) {                          /* a spider in a corner of the room */
            const Room *R = &G.rooms[G.vault_room[k]];
            int cx = rnd(2) ? R->x : R->x + R->w - 1, cy = rnd(2) ? R->y : R->y + R->h - 1;
            if (G.map[cy][cx].kind != K_FLOOR) continue;
            float wx = cx + (cx == R->x ? 0.16f : 0.84f), wy = cy + (cy == R->y ? 0.16f : 0.84f);
            add_deco(wx, wy, SP_COBWEB);
            Mob *m = spawn_mob(cx, cy, MOB_SPIDER);
            if (m) { m->ax = m->x = wx; m->ay = m->y = wy; m->z = 0.62f; m->guard = 1; }
        } else {
            Mob *m = spawn_mob(x, y, G.level >= 3 ? MOB_SHIELD : MOB_SKEL);
            if (m) m->guard = 1;
        }
    }
}

/* ================================================================== */
/* Light map                                                           */
/* ================================================================== */

static void light_dynamic(float lx, float ly, float power, float radius) {
    int x0 = clampi((int)((lx - radius) * LMR), 0, LMN - 1), x1 = clampi((int)((lx + radius) * LMR), 0, LMN - 1);
    int y0 = clampi((int)((ly - radius) * LMR), 0, LMN - 1), y1 = clampi((int)((ly + radius) * LMR), 0, LMN - 1);
    int lcx = (int)lx, lcy = (int)ly;
    for (int ty = y0; ty <= y1; ty++)
        for (int tx = x0; tx <= x1; tx++) {
            float wx = (tx + 0.5f) / LMR, wy = (ty + 0.5f) / LMR;
            float d = sqrtf((wx - lx) * (wx - lx) + (wy - ly) * (wy - ly));
            if (d >= radius) continue;
            int cx = tx / LMR, cy = ty / LMR;
            if (abs(cx - lcx) + abs(cy - lcy) > 1) {               /* cheap occlusion: step towards the light */
                int blocked = 0;
                float sx = wx, sy = wy, vx = (lx - wx) / d * 0.35f, vy = (ly - wy) / d * 0.35f;
                for (float s = 0.35f; s < d - 0.35f; s += 0.35f) {
                    sx += vx; sy += vy;
                    if (blocks_light((int)sx, (int)sy)) { blocked = 1; break; }
                }
                if (blocked) continue;
            }
            G.lm[ty * LMN + tx] += power / (1.0f + 0.9f * d * d) * (1.0f - smoothstepf(radius * 0.55f, radius, d));
        }
}

static float torch_flicker(const Light *L) {
    float t = (float)G.t + L->phase;
    return 0.80f + 0.12f * sinf(t * 9.1f) * sinf(t * 5.3f + 1.0f) + 0.08f * sinf(t * 23.0f + L->phase * 3.0f);
}

static void build_lightmap(void) {
    for (int i = 0; i < LMN * LMN; i++) G.lm[i] = 0.05f;
    for (int l = 0; l < G.nlights; l++) {
        const Light *L = &G.lights[l];
        float f = torch_flicker(L);
        for (int k = L->first; k < L->first + L->count; k++) G.lm[G.lc_idx[k]] += G.lc_w[k] * f;
    }
    light_dynamic(G.px, G.py, 0.62f, 2.8f);                        /* the caster's own faint glow */
    for (int i = 0; i < MAXSHOTS; i++)
        if (G.shots[i].alive) {
            static const float POW[NATK + 1] = { 1.7f, 1.1f, 1.2f, 0.4f, 1.0f, 2.3f, 0.8f, 0.7f, 0.0f, 1.4f };
            light_dynamic(G.shots[i].x, G.shots[i].y, POW[G.shots[i].kind], G.shots[i].kind == AT_HELL ? 4.4f : 3.4f);
        }
    for (int i = 0; i < MAXBONES; i++)
        if (G.bones[i].alive && G.bones[i].kind == MIS_FIRE) light_dynamic(G.bones[i].x, G.bones[i].y, 1.6f, 3.6f);
    for (int i = 0; i < MAXBOOMS; i++)
        if (G.booms[i].alive) light_dynamic(G.booms[i].x, G.booms[i].y, (G.booms[i].big ? 3.6f : 2.8f) * (1.0f - G.booms[i].t / 0.6f), G.booms[i].big ? 5.5f : 4.2f);
    for (int i = 0; i < MAXBURNS; i++)
        if (G.burns[i].alive) light_dynamic(G.burns[i].x, G.burns[i].y, 1.5f * (1.0f - smoothstepf(1.8f, 2.5f, G.burns[i].t)) * (0.85f + 0.15f * sinf((float)G.t * 17.0f + i)), 3.2f);
    for (int i = 0; i < MAXBEAMS; i++)
        if (G.beams[i].alive) light_dynamic((G.beams[i].x0 + G.beams[i].x1) * 0.5f, (G.beams[i].y0 + G.beams[i].y1) * 0.5f, 1.6f * (1.0f - G.beams[i].t / 0.3f), 3.5f);
    for (int i = 0; i < MAXMOBS; i++) {                            /* a wall skull gathering its fire */
        const Mob *m = &G.mobs[i];
        if (m->alive && m->type == MOB_WSKULL && m->state == MS_WALK && m->sub == 1)
            light_dynamic(m->x, m->y, 0.3f + 1.2f * fminf(1.0f, m->sub_t / 3.0f), 3.0f);
    }
    for (int i = 0; i < G.nitems; i++)                             /* keys glow, and the urns faintly */
        if (G.items[i].alive && (G.items[i].type == IT_KEY || G.items[i].type == IT_URN))
            light_dynamic(G.items[i].x, G.items[i].y, (G.items[i].type == IT_KEY ? 0.75f : 0.35f) + 0.2f * sinf((float)G.t * 3.0f + i), 2.2f);
    if (G.state == PS_CAST && G.hand > 0.3f)
        light_dynamic(G.px + cosf(G.ang) * 0.3f, G.py + sinf(G.ang) * 0.3f, 0.9f * G.hand, 2.4f);
}

static float lm_at(float x, float y) {
    float fx = x * LMR - 0.5f, fy = y * LMR - 0.5f;
    int ix = (int)floorf(fx), iy = (int)floorf(fy);
    float ax = fx - ix, ay = fy - iy;
    if (ix < 0 || iy < 0 || ix >= LMN - 1 || iy >= LMN - 1) return 0.0f;
    const float *p = &G.lm[iy * LMN + ix];
    return (p[0] * (1 - ax) + p[1] * ax) * (1 - ay) + (p[LMN] * (1 - ax) + p[LMN + 1] * ax) * ay;
}

static inline float atten(float d) { return 1.0f / (1.0f + 0.010f * d * d); }

/* ================================================================== */
/* Mobs, shots, explosions                                             */
/* ================================================================== */

static int line_clear(float x0, float y0, float x1, float y1) {
    float dx = x1 - x0, dy = y1 - y0, d = sqrtf(dx * dx + dy * dy);
    int n = (int)(d / 0.1f);
    for (int i = 1; i < n; i++) {
        float t = (float)i / n;
        int cx = (int)(x0 + dx * t), cy = (int)(y0 + dy * t);
        Cell *c = cell(cx, cy);
        if (c->kind == K_WALL || c->kind == K_PILLAR || (c->kind == K_DOOR && G.doors[c->door].open < 0.7f)) return 0;
    }
    return 1;
}

/* would a fireball thrown from the caster get there? (finer than line_clear, and from where it leaves the hand) */
static int shot_clear(float x1, float y1) {
    float a = atan2f(y1 - G.py, x1 - G.px), x0 = G.px + cosf(a) * 0.35f, y0 = G.py + sinf(a) * 0.35f;
    float dx = x1 - x0, dy = y1 - y0, d = sqrtf(dx * dx + dy * dy);
    int n = (int)(d / 0.04f) + 1;
    for (int i = 0; i <= n; i++) {
        float t = (float)i / n;
        int cx = (int)(x0 + dx * t), cy = (int)(y0 + dy * t);
        if (blocks_light(cx, cy) || cell(cx, cy)->kind == K_PILLAR) return 0;
    }
    return 1;
}

static void sfx_at(int id, float x, float y, float gain) {
    float dx = x - G.px, dy = y - G.py, d = sqrtf(dx * dx + dy * dy);
    float rx = -sinf(G.ang), ry = cosf(G.ang);
    float lat = dx * rx + dy * ry;
    float g = gain / (1.0f + 0.05f * d * d);
    if (g > 0.03f) snd_sfx_ex(id, g, 0.5f + 0.4f * clampf(lat / (d + 0.5f), -1.0f, 1.0f));
}

/* creatures that stand in a cell and get in the way (not wraiths, wall skulls or spiders in their webs) */
static int solid_mob(const Mob *m) { return m->alive && m->type != MOB_GHOST && m->type != MOB_WSKULL && m->type != MOB_SPIDER; }

static int occupied(int x, int y, const Mob *self) {
    if ((x == G.cx && y == G.cy) || (x == G.tx && y == G.ty)) return 1;
    for (int i = 0; i < MAXMOBS; i++) {
        const Mob *m = &G.mobs[i];
        if (m != self && solid_mob(m) && ((m->tx == x && m->ty == y) || (m->cx == x && m->cy == y))) return 1;
    }
    return 0;
}

static float mob_base_hp(int type) {
    switch (type) {
    case MOB_SKEL: return 20.0f;
    case MOB_BAT: return 10.0f;
    case MOB_SPIDER: return 15.0f;
    case MOB_SHIELD: return 30.0f;
    case MOB_WSKULL: return 1.0f;                                  /* one hit of anything */
    default: return 1e9f;
    }
}
static float shield_hp(void) { return 10.0f + 6.0f * (float)(G.level - 3); }   /* grows tougher each level */

static Mob *spawn_mob(int x, int y, int type) {
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *m = &G.mobs[i];
        if (m->alive) continue;
        memset(m, 0, sizeof *m);
        m->alive = 1; m->type = type; m->state = MS_WALK;
        m->x = x + 0.5f; m->y = y + 0.5f; m->cx = m->tx = x; m->cy = m->ty = y;
        m->speed = type == MOB_BAT ? 1.7f : (type == MOB_GHOST ? 0.7f : (type == MOB_SHIELD ? 1.0f : 0.9f));
        m->hp = m->maxhp = type == MOB_WSKULL ? 1.0f : mob_base_hp(type) * powf(1.1f, (float)(G.level - 1));
        if (type == MOB_SHIELD) m->shield = shield_hp();
        m->face = rndf() * TAU_F;
        if (type != MOB_GHOST) G.kills_total++;
        m->t = rndf() * 10.0f; m->orbit = rndf() * TAU_F;
        return m;
    }
    return NULL;
}

/* what wanders the halls: skeletons, wraiths and bats, then the shield-bearers from level 3 */
static int random_type(void) {
    int k = rnd(20);
    if (G.level >= 3 && k < 4) return MOB_SHIELD;
    return k < 10 ? MOB_SKEL : (k < 15 ? MOB_GHOST : MOB_BAT);
}

/* cells reachable from (sx,sy) at a BFS depth in [dmin, dmax] */
static int pick_cell_at_depth(int sx, int sy, int dmin, int dmax, int need_hidden, int *ox, int *oy) {
    static int dist[MAPN][MAPN], qx[MAPN * MAPN], qy[MAPN * MAPN], cand[MAPN * MAPN];
    for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) dist[y][x] = -1;
    int qh = 0, qt = 0, nc = 0;
    dist[sy][sx] = 0; qx[qt] = sx; qy[qt] = sy; qt++;
    while (qh < qt) {
        int x = qx[qh], y = qy[qh]; qh++;
        int d = dist[y][x];
        if (d >= dmin && d <= dmax && G.map[y][x].kind == K_FLOOR && !occupied(x, y, NULL)
            && (!need_hidden || !line_clear(G.px, G.py, x + 0.5f, y + 0.5f))) cand[nc++] = y * MAPN + x;
        if (d >= dmax) continue;
        for (int k = 0; k < 4; k++) {
            int nx = x + DX[k], ny = y + DY[k];
            if (!passable(nx, ny) || dist[ny][nx] >= 0) continue;
            dist[ny][nx] = d + 1; qx[qt] = nx; qy[qt] = ny; qt++;
        }
    }
    if (!nc) return 0;
    int c = cand[rnd(nc)];
    *ox = c % MAPN; *oy = c / MAPN;
    return 1;
}

static void pop_readout(void) { G.readout = READOUT_T; }

#ifdef CRAWL_DEBUG
static int hits_bone, hits_bat, throws, shrieks, hits_fire, hits_sword, bites, casts[NATK], defs[NDEF], bfs_kills, dodges, wskull_wakes, spider_drops;
#define DBG_COUNT(v) ((v)++, fprintf(stderr, "EVENT %s t=%.2f\n", #v, G.t))
#else
#define DBG_COUNT(v) ((void)0)
#endif

static int in_combat(void);

/* The caster takes a blow. Shadow robes may make it miss, a ward soaks it up, armor blunts it.
 * Returns what got through. (sx, sy): where it came from, for the portrait's flinch. */
static int hurt_ex(float dmg, int kind, float sx, float sy) {
    if (G.dead || G.ending || G.exiting || G.tally || G.shop || G.level_t < 1.5) return 0;
    const Gear *g = &RUN.gear;
    if (kind != HURT_POISON) {
        if (rndf() < gear_dodge(g)) { snd_sfx_ex(SFX_WHOOSH, 0.5f, 0.5f); DBG_COUNT(dodges); return 0; }   /* it passed through a shadow */
        if (kind == HURT_MAGIC) dmg *= 1.0f - gear_magic_resist(g);
        dmg -= kind == HURT_MAGIC ? gear_armor(g) * 0.5f : (float)gear_armor(g);
        if (dmg < 1.0f) dmg = 1.0f;
        if (G.ward_hp > 0.0f) {                                    /* the bone ward takes it */
            float ab = fminf(G.ward_hp, dmg);
            G.ward_hp -= ab; dmg -= ab;
            sfx_at(SFX_BONES, sx, sy, 0.5f);
            if (G.ward_hp <= 0.0f) { G.ward_t = 0.0f; snd_sfx_ex(SFX_SHATTER, 0.8f, 0.5f); }
            if (dmg < 0.5f) return 0;
        }
    }
    int d = (int)(dmg + 0.5f);
    if (d < 1) d = 1;
    G.hp -= d;
    if (kind != HURT_POISON) {
        G.hurt = 1.0f;
        G.kick = (rnd(2) ? 1.0f : -1.0f) * 0.07f;
        float lat = (sx - G.px) * -sinf(G.ang) + (sy - G.py) * cosf(G.ang);
        G.face_side = fabsf(lat) < 0.3f ? 0 : (lat < 0.0f ? -1 : 1);
        G.face_t = FACE_T;
    } else G.hurt = fmaxf(G.hurt, 0.25f);
    pop_readout();
    if (G.hp > 0) { if (kind != HURT_POISON) snd_sfx(SFX_HURT); return d; }
    G.hp = 0; G.dead = 1; G.dead_t = 0.0;
    G.hand = 0.0f; G.spd = 0.0f; G.face_t = 1e9f;
    snd_sfx(SFX_DEATH);
    return d;
}

static void throw_missile(const Mob *m, int kind) {
    for (int i = 0; i < MAXBONES; i++) {
        Bone *b = &G.bones[i];
        if (b->alive) continue;
        float sp = kind == MIS_FIRE ? FIRE_SPEED : BONE_SPEED;
        float tx = G.px, ty = G.py, tt = hypotf(tx - m->x, ty - m->y) / sp;
        if (kind == MIS_BONE) { tx += G.pvx * tt * 0.6f; ty += G.pvy * tt * 0.6f; }   /* leads the caster a little */
        float a = atan2f(ty - m->y, tx - m->x) + (rndf() - 0.5f) * (kind == MIS_FIRE ? 0.05f : 0.14f);
        *b = (Bone){ 1, kind, m->x + cosf(a) * 0.3f, m->y + sinf(a) * 0.3f, cosf(a) * sp, sinf(a) * sp, 4.0f, rndf() * 4.0f,
                     (float)(kind == MIS_FIRE ? 2 * enemy_damage() : enemy_damage()) };
        sfx_at(kind == MIS_FIRE ? SFX_CAST : SFX_THROW, m->x, m->y, 1.0f);
        return;
    }
}
static void throw_bone(const Mob *m) { throw_missile(m, MIS_BONE); }

static void explode(float x, float y);
static Shot *new_shot(int kind, float x, float y, float a, float speed, float dmg);

static void update_bones(float dt) {
    for (int i = 0; i < MAXBONES; i++) {
        Bone *b = &G.bones[i];
        if (!b->alive) continue;
        float k = G.slip_t > 0.0f && hypotf(b->x - G.px, b->y - G.py) < 7.0f ? 0.3f : 1.0f;   /* Time Slip */
        b->life -= dt; b->spin += dt * 14.0f * k;
        for (int s = 0; s < 4 && b->alive; s++) {
            float nx = b->x + b->vx * dt * k / 4, ny = b->y + b->vy * dt * k / 4;
            if (blocks_light((int)nx, (int)ny) || cell((int)nx, (int)ny)->kind == K_PILLAR) {
                b->alive = 0;
                if (b->kind == MIS_FIRE) explode(b->x, b->y); else sfx_at(SFX_BONES, b->x, b->y, 0.4f);
                break;
            }
            b->x = nx; b->y = ny;
            float d = hypotf(b->x - G.px, b->y - G.py);
            if (G.mirror_t > 0.0f && d < 0.7f && !G.dead) {           /* the Mirror Veil sends it back */
                b->alive = 0;
                Shot *sh = new_shot(SHOT_REFLECT, b->x, b->y, atan2f(-b->vy, -b->vx), b->kind == MIS_FIRE ? 6.0f : 7.0f, b->dmg * 1.5f);
                if (sh) sh->z = (float)b->kind;
                snd_sfx_ex(SFX_SHIMMER, 0.35f, 0.5f);
                break;
            }
            if (!G.dead && d < 0.36f) {
                b->alive = 0;
                if (b->kind == MIS_FIRE) { hurt_ex(b->dmg, HURT_MAGIC, b->x - b->vx, b->y - b->vy); explode(b->x, b->y); DBG_COUNT(hits_fire); }
                else { hurt_ex(b->dmg, HURT_BLOW, b->x - b->vx, b->y - b->vy); DBG_COUNT(hits_bone); }
            }
        }
        if (b->life <= 0.0f) b->alive = 0;
    }
}

/* souls: one per kill, more with a Soul Jar, three times over while Soul Reaping runs */
static void gain_souls(float n) {
    float mult = gear_soul_mult(&RUN.gear) * (G.reap_t > 0.0f ? 3.0f : 1.0f);
    RUN.soul_frac += n * mult;
    if (rndf() < gear_cowl(&RUN.gear)) RUN.soul_frac += 1.0f;      /* Death's cowl */
    int whole = (int)RUN.soul_frac;
    if (whole > 0) { RUN.souls += whole; RUN.soul_frac -= (float)whole; G.level_souls += whole; snd_sfx_ex(SFX_GHOST_TICK, 0.5f, 0.5f); }
}

static void kill_mob(Mob *m) {
    m->state = MS_DIE; m->t = 0.0f;
    G.kills++;
    static const int SND[] = { SFX_BONES, SFX_GHOST_DIE, SFX_BAT, SFX_SHATTER, SFX_BONES, SFX_SKITTER };
    sfx_at(SND[m->type], m->x, m->y, 1.0f);
    if (m->type == MOB_SPIDER && m->z > 0.3f) m->sub = 9;           /* it falls from the web */
    gain_souls(1.0f);
}

/* A hit on a creature, from (fx, fy) (shields only stop what comes from in front). Returns 1 if it died. */
static int damage_mob(Mob *m, float dmg, int flags, float fx, float fy, float slow) {
    if (!m->alive || m->state != MS_WALK || m->type == MOB_GHOST || dmg <= 0.0f) return 0;
    if (m->type == MOB_SHIELD && m->shield > 0.0f && !(flags & (DM_PIERCE | DM_ARC))) {
        float a = atan2f(fy - m->y, fx - m->x);
        if (fabsf(wrap_pi(a - m->face)) < 1.25f) {
            float ab = fminf(m->shield, dmg);
            m->shield -= ab; dmg -= ab;
            if (m->shield <= 0.0f) sfx_at(SFX_SHATTER, m->x, m->y, 1.0f);   /* splinters */
            else sfx_at(SFX_STEP, m->x, m->y, 1.4f);                       /* a thud on the wood */
            m->misses = 0;                                         /* a blow on the shield still counts */
            if (dmg <= 0.0f) { m->stagger = fmaxf(m->stagger, 0.2f); return 0; }
        }
    }
    if ((flags & DM_SLOW) && slow > 0.0f) m->slowed = fmaxf(m->slowed, slow);
    m->misses = 0;
    m->hp -= dmg;
    if (m->hp <= 0.0f) { kill_mob(m); return 1; }
    if (m->type == MOB_SKEL || m->type == MOB_SHIELD) {            /* knocked back a step */
        m->stagger = 0.6f; m->throw_t = 0.0f; m->swing = 0.0f; m->cool = fmaxf(m->cool, 0.9f);
        sfx_at(SFX_BONES, m->x, m->y, 0.45f);
    }
    if (m->type == MOB_SPIDER && m->sub == 0) { m->sub = 1; m->sub_t = 0.0f; }   /* disturbed: it comes down */
    return 0;
}

static void thorns(Mob *m) {
    int t = gear_thorns(&RUN.gear);
    if (t > 0 && m->alive) damage_mob(m, (float)t, DM_ARC, G.px, G.py, 0.0f);
}

static void explode(float x, float y) {
    for (int i = 0; i < MAXBOOMS; i++)
        if (!G.booms[i].alive) { G.booms[i] = (Boom){ 1, x, y, 0.0f, 1.0f, 0 }; break; }
    sfx_at(SFX_EXPLODE, x, y, 1.0f);
}
static void explode_big(float x, float y, float r) {
    for (int i = 0; i < MAXBOOMS; i++)
        if (!G.booms[i].alive) { G.booms[i] = (Boom){ 1, x, y, 0.0f, r, 1 }; break; }
    sfx_at(SFX_EXPLODE, x, y, 1.4f); sfx_at(SFX_BOOM, x, y, 0.6f);
}

/* everything within r of (x, y) takes dmg, but not `skip` (it took the direct hit) */
static void splash(float x, float y, float r, float dmg, const Mob *skip) {
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *m = &G.mobs[i];
        if (m == skip || !m->alive || m->state != MS_WALK || m->type == MOB_GHOST) continue;
        if (hypotf(m->x - x, m->y - y) < r && line_clear(x, y, m->x, m->y)) damage_mob(m, dmg, DM_ARC, x, y, 0.0f);
    }
}

/* Wraiths drift through stone towards the caster, looming, until it comes close; then they recoil
 * with a shriek and flee back through the walls, fading as they go. Nothing can touch them, and
 * they touch nothing. */
static void update_ghost(Mob *m, float dt) {
    m->t += dt; m->st += dt; m->anim += dt * 1.6f;
    float dx = G.px - m->x, dy = G.py - m->y, d = sqrtf(dx * dx + dy * dy) + 1e-4f;
    m->z = 0.10f + 0.06f * sinf(m->t * 1.7f);
    if (m->state == MS_WALK) {
        float sp = (d > 6.0f ? 0.75f : 0.5f) * (G.dead || G.exiting ? 0.0f : 1.0f);
        float weave = sinf(m->t * 0.6f + m->orbit) * 0.4f;
        m->x += (dx / d * sp - dy / d * weave) * dt;
        m->y += (dy / d * sp + dx / d * weave) * dt;
        if ((d < 2.1f && line_clear(m->x, m->y, G.px, G.py)) || d < 1.2f) {
            m->state = MS_RECOIL; m->st = 0.0f;
            sfx_at(SFX_SHRIEK, m->x, m->y, 1.4f);
            G.chill = 1.0f;
            DBG_COUNT(shrieks);
        } else if (m->st > 30.0f) { m->state = MS_FLEE; m->st = 0.0f; }   /* tired of waiting: melts away */
        if (d < 6.0f && rnd(500) == 0) sfx_at(SFX_MOAN, m->x, m->y, 0.9f);
    } else if (m->state == MS_RECOIL) {
        m->x -= dx / d * 0.6f * dt; m->y -= dy / d * 0.6f * dt;     /* reeling back */
        if (m->st > 0.55f) { m->state = MS_FLEE; m->st = 0.0f; }
    } else {                                                       /* fleeing, through whatever is in the way */
        m->x -= dx / d * 4.0f * dt; m->y -= dy / d * 4.0f * dt;
        m->fade += dt / 0.9f;
        if (m->fade >= 1.0f) m->alive = 0;
    }
    m->x = clampf(m->x, 0.5f, MAPN - 0.5f); m->y = clampf(m->y, 0.5f, MAPN - 0.5f);
    m->cx = m->tx = (int)m->x; m->cy = m->ty = (int)m->y;
}

/* how fast a creature's clock runs: frozen stops it, frost slows it, Time Slip slows everything near */
static float mob_time(Mob *m, float dt) {
    if (m->frozen > 0.0f) { m->frozen -= dt; return 0.0f; }
    float k = 1.0f;
    if (m->slowed > 0.0f) { m->slowed -= dt; k *= 0.5f; }
    if (G.slip_t > 0.0f && hypotf(m->x - G.px, m->y - G.py) < 7.0f) k *= 0.3f;
    return dt * k;
}

/* The wall skull: asleep until the caster comes close, then a scream and three seconds of fire
 * gathering, then a fireball; again and again while the caster stays near. One hit ends it. */
static void update_wskull(Mob *m, float dt) {
    m->t += dt;
    if (m->state == MS_DIE) {                                      /* falls from the wall and breaks */
        m->z = fmaxf(0.0f, m->z - dt * 1.6f);
        if (m->t >= 0.55f) { add_deco(m->x, m->y, SP_BONES1); m->alive = 0; }
        return;
    }
    dt = mob_time(m, dt);
    m->sub_t += dt;
    if (m->feared > 0.0f) { m->feared -= dt; m->sub = 0; m->sub_t = 0.0f; return; }   /* Dread: it cowers silent */
    float d = hypotf(G.px - m->x, G.py - m->y);
    int seen = !G.dead && !G.exiting && line_clear(m->x, m->y, G.px, G.py);
    switch (m->sub) {
    case 0: {                                                      /* asleep, until the caster walks up to it, eyes on it */
        float vx = (G.px - m->x) / (d + 1e-4f), vy = (G.py - m->y) / (d + 1e-4f);
        int facing = vx * cosf(m->face) + vy * sinf(m->face) > 0.6f && -(vx * cosf(G.ang) + vy * sinf(G.ang)) > 0.8f;
        if (seen && facing && d < 3.0f) {
            m->sub = 1; m->sub_t = 0.0f; sfx_at(SFX_SCREAM, m->x, m->y, 1.5f); DBG_COUNT(wskull_wakes);
#ifdef CRAWL_DEBUG
            fprintf(stderr, "   skull at (%.2f,%.2f) caster (%.2f,%.2f) ang %.2f\n", m->x, m->y, G.px, G.py, G.ang);
#endif
            m->ignore = (float)G.t + 0.4f + 1.2f * rndf() + (rndf() < 0.2f ? 1.6f : 0.0f);   /* startled */
            G.kick = 0.05f; G.chill = fmaxf(G.chill, 0.35f);
        }
        break;
    }
    case 1:                                                        /* the fire gathers: three seconds */
        if (m->sub_t >= 3.0f) {
            if (seen && d < 9.0f) throw_missile(m, MIS_FIRE);
            m->sub = 2; m->sub_t = 0.0f;
        }
        break;
    default:                                                       /* spent; it gathers again if you linger */
        if (m->sub_t >= 3.0f) {
            m->sub = seen && d < 4.5f ? 1 : 0; m->sub_t = 0.0f;   /* awake now: it needs no second look */
            if (m->sub == 1) sfx_at(SFX_SCREAM, m->x, m->y, 0.9f);
        }
        break;
    }
}

/* The spider: hidden in its corner web with only its eyes showing; it drops on its thread when the
 * caster comes within two steps, bites (poison and a slow), and climbs back to wait. */
static void bite(Mob *m) {
    const Gear *g = &RUN.gear;
    sfx_at(SFX_SKITTER, m->x, m->y, 1.4f); snd_sfx_ex(SFX_HURT, 0.7f, 0.5f);
    G.face_t = FACE_T; G.face_side = 0; G.hurt = fmaxf(G.hurt, 0.6f);
    thorns(m);
    DBG_COUNT(bites);
    if (G.purify_t > 0.0f) return;                                  /* Purify's protection */
    float k = 1.0f - gear_poison_resist(g);
    G.poison_t = fmaxf(G.poison_t, 6.0f * k);
    G.poison_dps = 1.0f + (float)((G.level - 4) / 2 > 0 ? (G.level - 4) / 2 : 0);
    G.slow_t = fmaxf(G.slow_t, 4.0f * k);
}

static void update_spider(Mob *m, float dt) {
    m->t += dt;
    if (m->state == MS_DIE) {
        m->z = fmaxf(0.0f, m->z - dt * 1.4f);
        if (m->t >= 0.9f) { add_deco(m->x, m->y, SP_SPIDER_D); m->alive = 0; }
        return;
    }
    dt = mob_time(m, dt);
    m->sub_t += dt; m->cool -= dt;
    float dax = G.px - m->ax, day = G.py - m->ay, da = sqrtf(dax * dax + day * day) + 1e-4f;
    float reach = fminf(da - 0.45f, 0.75f);                         /* how far out from the corner it drops */
    switch (m->sub) {
    case 0:
        m->x = m->ax; m->y = m->ay; m->z = 0.62f;
        if (!G.dead && m->cool <= 0.0f && da < 2.0f && line_clear(m->ax, m->ay, G.px, G.py)) { m->sub = 1; m->sub_t = 0.0f; sfx_at(SFX_SKITTER, m->x, m->y, 1.0f); DBG_COUNT(spider_drops); }
        break;
    case 1: {                                                      /* down the thread */
        float k = fminf(1.0f, m->sub_t / 0.35f);
        m->z = lerpf(0.62f, 0.12f, k);
        m->x = m->ax + dax / da * reach * k; m->y = m->ay + day / da * reach * k;
        if (k >= 1.0f) { m->sub = 2; m->sub_t = 0.0f; }
        break;
    }
    case 2:                                                        /* the bite */
        if (m->sub_t >= 0.25f) {
            if (!G.dead && hypotf(G.px - m->x, G.py - m->y) < 1.05f) bite(m);
            m->sub = 3; m->sub_t = 0.0f;
            m->bx = m->x; m->by = m->y;                             /* where it climbs back from */
        }
        break;
    default: {                                                     /* back up to the web */
        float k = fminf(1.0f, m->sub_t / 1.2f);
        m->z = lerpf(0.12f, 0.62f, k);
        m->x = lerpf(m->bx, m->ax, k); m->y = lerpf(m->by, m->ay, k);
        if (k >= 1.0f) { m->sub = 0; m->cool = 3.0f; }
        break;
    }
    }
    m->cx = m->tx = (int)m->x; m->cy = m->ty = (int)m->y;
}

static void update_mob(Mob *m, float dt) {
    if (m->state == MS_DIE && m->sub == 77) { if ((m->t += dt) >= 0.9f) m->alive = 0; return; }   /* gone into the BFS9000 */
    if (m->type == MOB_GHOST) { update_ghost(m, dt); return; }
    if (m->type == MOB_WSKULL) { update_wskull(m, dt); return; }
    if (m->type == MOB_SPIDER) { update_spider(m, dt); return; }
    m->t += dt;
    if (m->state == MS_DIE) {
        float dur = m->type == MOB_BAT ? 0.9f : 0.75f;
        if (m->type == MOB_BAT) m->z = fmaxf(0.0f, m->z - dt * 0.6f);
        if (m->t >= dur) {
            if (m->type == MOB_SKEL || m->type == MOB_SHIELD) add_deco(m->x, m->y, SP_SKEL_D3);
            m->alive = 0;
        }
        return;
    }
    dt = mob_time(m, dt);
    if (dt <= 0.0f) return;
    m->anim += dt * m->speed * 3.0f;
    m->z = m->type == MOB_BAT ? 0.20f + 0.07f * sinf(m->t * 7.0f) : 0.0f;
    float pdx0 = G.px - m->x, pdy0 = G.py - m->y, pd0 = sqrtf(pdx0 * pdx0 + pdy0 * pdy0);
    int undead = m->type == MOB_SKEL || m->type == MOB_SHIELD;
    if (m->feared > 0.0f) m->feared -= dt;
    if (m->stagger > 0.0f) { m->stagger -= dt; return; }
    int afraid = undead && m->feared > 0.0f;
    if (m->type == MOB_SKEL && !G.dead && !afraid) {               /* once it has seen you, it keeps throwing */
        int los = pd0 < 9.0f && G.level_t > 1.5 && line_clear(m->x, m->y, G.px, G.py);
        if (los && !m->seen) { m->seen = 1; m->cool = (0.45f + 0.35f * rndf()) * throw_interval() / 1.5f; }
        if (m->throw_t > 0.0f) {
            m->throw_t -= dt;
            if (m->throw_t <= 0.0f) { throw_bone(m); m->cool = throw_interval() * (1.0f + (rndf() - 0.5f) * 0.27f); DBG_COUNT(throws); }
            return;                                                /* stands still to throw */
        }
        if (m->seen && los && (m->cool -= dt) <= 0.0f) { m->throw_t = 0.35f; return; }
    }
    if (m->type == MOB_SHIELD && !G.dead && !afraid) {             /* closes in behind its shield and strikes */
        int los = pd0 < 9.0f && G.level_t > 1.5 && line_clear(m->x, m->y, G.px, G.py);
        if (los) { m->seen = 1; m->face += wrap_pi(atan2f(pdy0, pdx0) - m->face) * fminf(1.0f, dt * 6.0f); }
        m->cool -= dt;
        if (m->swing > 0.0f) {
            m->swing -= dt;
            if (m->swing <= 0.0f) {
                m->swing = 0.0f; m->cool = 1.4f;
                sfx_at(SFX_SWORD, m->x, m->y, 1.2f);
                if (pd0 < 1.6f && hurt_ex((float)(enemy_damage() + 1), HURT_BLOW, m->x, m->y) > 0) { thorns(m); DBG_COUNT(hits_sword); }
            }
            return;
        }
        if (los && pd0 < 1.35f && m->cool <= 0.0f) { m->swing = 0.45f; return; }
    }
    if (m->type == MOB_BAT && !G.dead) {                           /* bats come for your face */
        int los = pd0 < 5.0f && line_clear(m->x, m->y, G.px, G.py);
        if (los) m->attacking = 1;
        if (m->attacking && pd0 > 7.0f) { m->attacking = 0; m->tx = m->cx; m->ty = m->cy; }
        if (m->attacking) {
            m->orbit += dt * (1.3f + 0.4f * sinf(m->t * 0.7f));
            float a = G.ang + sinf(m->orbit) * 1.5f, r = 0.95f + 0.18f * sinf(m->orbit * 2.3f);
            float gx = G.px + cosf(a) * r, gy = G.py + sinf(a) * r;
            float vx = gx - m->x, vy = gy - m->y, vd = sqrtf(vx * vx + vy * vy) + 1e-4f, step = fminf(vd, 2.6f * dt);
            float nx = m->x + vx / vd * step, ny = m->y + vy / vd * step;
            if (!blocks_move((int)nx, (int)m->y)) m->x = nx;
            if (!blocks_move((int)m->x, (int)ny)) m->y = ny;
            m->cx = m->tx = (int)m->x; m->cy = m->ty = (int)m->y;
            if (hypotf(G.px - m->x, G.py - m->y) < 1.25f) {           /* touching: after 3 s it bites */
                m->contact += dt;
                if (m->contact >= 3.0f) {
                    m->contact = 0.0f;
                    if (hurt_ex((float)enemy_damage(), HURT_BLOW, m->x, m->y) > 0) thorns(m);
                    sfx_at(SFX_BAT, m->x, m->y, 1.0f); DBG_COUNT(hits_bat);
                }
            } else m->contact = 0.0f;
            if (rnd(60) == 0) sfx_at(SFX_BAT, m->x, m->y, 0.6f);
            return;
        }
    }
    float gx = m->tx + 0.5f, gy = m->ty + 0.5f, dx = gx - m->x, dy = gy - m->y, d = sqrtf(dx * dx + dy * dy);
    float step = m->speed * dt * (m->type == MOB_BAT ? 0.8f + 0.4f * sinf(m->t * 3.0f) : 1.0f) * (afraid ? 1.4f : 1.0f);
    if (d > step) { m->x += dx / d * step; m->y += dy / d * step; if (m->type == MOB_SHIELD && !m->seen) m->face = atan2f(dy, dx); return; }
    m->x = gx; m->y = gy; m->cx = m->tx; m->cy = m->ty;
    /* next cell: close in on the caster when it is near and visible, otherwise wander; flee if afraid */
    float pdx = G.px - m->x, pdy = G.py - m->y, pd = sqrtf(pdx * pdx + pdy * pdy);
    float keep = m->type == MOB_SHIELD ? 1.05f : 2.2f;              /* shield-bearers come right up close */
    int chase = pd < (m->type == MOB_SHIELD ? 9.0f : 7.0f) && pd > keep && line_clear(m->x, m->y, G.px, G.py);
    int best = -1; float bs = -1e9f;
    for (int k = 0; k < 4; k++) {
        int nx = m->cx + DX[k], ny = m->cy + DY[k];
        if (blocks_move(nx, ny) || occupied(nx, ny, m)) continue;
        float s = rndf();
        if (afraid) s += hypotf(nx + 0.5f - G.px, ny + 0.5f - G.py) * 2.0f;
        else if (chase) s -= hypotf(nx + 0.5f - G.px, ny + 0.5f - G.py);
        if (s > bs) { bs = s; best = k; }
    }
    if (best >= 0 && (afraid || !(pd <= keep))) { m->tx = m->cx + DX[best]; m->ty = m->cy + DY[best]; }
    if (m->type == MOB_BAT && pd < 6.0f && rnd(40) == 0) sfx_at(SFX_BAT, m->x, m->y, 0.6f);
}

/* ---- the caster's spells ---- */

static int wisp_field[MAPN][MAPN];                                  /* steps to the wisps' target, for going round corners */
static void wisp_bfs(int tx, int ty) {
    static int qx[MAPN * MAPN], qy[MAPN * MAPN];
    for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) wisp_field[y][x] = -1;
    int qh = 0, qt = 0;
    wisp_field[ty][tx] = 0; qx[qt] = tx; qy[qt] = ty; qt++;
    while (qh < qt) {
        int x = qx[qh], y = qy[qh]; qh++;
        if (wisp_field[y][x] > 14) continue;
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (blocks_light(nx, ny) || cell(nx, ny)->kind == K_PILLAR || wisp_field[ny][nx] >= 0) continue;
            wisp_field[ny][nx] = wisp_field[y][x] + 1; qx[qt] = nx; qy[qt] = ny; qt++;
        }
    }
}

static Shot *new_shot(int kind, float x, float y, float a, float speed, float dmg) {
    for (int i = 0; i < MAXSHOTS; i++) {
        Shot *s = &G.shots[i];
        if (s->alive) continue;
        *s = (Shot){ 1, kind, -1, x, y, 0.0f, cosf(a) * speed, sinf(a) * speed, kind == AT_SPEAR ? 1.0f : 1.6f, dmg, 0 };
        return s;
    }
    return NULL;
}

static void add_beam(int kind, float x0, float y0, float z0, float x1, float y1, float z1) {
    for (int i = 0; i < MAXBEAMS; i++)
        if (!G.beams[i].alive) { G.beams[i] = (Beam){ 1, kind, x0, y0, z0, x1, y1, z1, 0.0f }; return; }
}

static int spell_tier(int sp) { return RUN.gear.atk[sp] < 0 ? 0 : RUN.gear.atk[sp]; }

/* lightning: to the target, then hopping on to others nearby */
static void chain_lightning(int t, float dmg) {
    uint32_t hit = 0;
    Mob *m = &G.mobs[t];
    float hx = G.px + cosf(G.ang) * 0.35f, hy = G.py + sinf(G.ang) * 0.35f;
    add_beam(AT_CHAIN, hx, hy, 0.42f, m->x, m->y, m->z + 0.5f);
    hit |= 1u << t;
    damage_mob(m, dmg, DM_ARC, G.px, G.py, 0.0f);
    int tier = spell_tier(AT_CHAIN), hops = 2 + (tier >= 4) + (tier >= 7);
    float cx = m->x, cy = m->y, cz = m->z;
    for (int h = 0; h < hops; h++) {
        int best = -1; float bd = 3.5f;
        for (int i = 0; i < MAXMOBS; i++) {
            Mob *o = &G.mobs[i];
            if ((hit >> i & 1) || !o->alive || o->state != MS_WALK || o->type == MOB_GHOST) continue;
            float d = hypotf(o->x - cx, o->y - cy);
            if (d < bd && line_clear(cx, cy, o->x, o->y)) { bd = d; best = i; }
        }
        if (best < 0) break;
        Mob *o = &G.mobs[best];
        add_beam(AT_CHAIN, cx, cy, cz + 0.5f, o->x, o->y, o->z + 0.5f);
        hit |= 1u << best;
        dmg *= 0.75f;
        damage_mob(o, dmg, DM_ARC, cx, cy, 0.0f);
        cx = o->x; cy = o->y; cz = o->z;
    }
    snd_sfx_ex(SFX_ZAP, 1.2f, 0.5f);
}

/* the BFS9000: everything the caster can see collapses into a point of light */
static void bfs9000(void) {
    int tier = RUN.gear.atk[AT_BFS];
    RUN.souls -= bfs_cost(tier);
    G.bfs_t = 0.001f;
    snd_sfx_ex(SFX_IMPLODE, 1.3f, 0.5f);
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *m = &G.mobs[i];
        if (!m->alive || m->state != MS_WALK || m->type == MOB_GHOST) continue;
        if (hypotf(m->x - G.px, m->y - G.py) > 14.0f || !line_clear(G.px, G.py, m->x, m->y)) continue;
        m->hp = 0.0f;
        kill_mob(m);
        m->sub = 77;                                               /* drawn imploding */
#ifdef CRAWL_DEBUG
        bfs_kills++;
#endif
    }
}

/* let the chosen spell go at target t (or straight ahead if it is gone) */
static void fizzle(int sp);
static void cast_spell(int sp, int t) {
    const Gear *g = &RUN.gear;
    Mob *m = t >= 0 ? &G.mobs[t] : &G.mobs[0];
    int have = t >= 0 && m->alive && m->state == MS_WALK;
    float tx = have ? m->x : G.px + cosf(G.ang) * 4, ty = have ? m->y : G.py + sinf(G.ang) * 4;
    float a = atan2f(ty - G.py, tx - G.px), dmg = spell_damage(g, sp);
    if (G.player && sp != AT_CHAIN && sp != AT_DRAIN) a = G.ang;   /* a person's shots go where they look */
    float hx = G.px + cosf(a) * 0.35f, hy = G.py + sinf(a) * 0.35f;
    int bat = have && m->type == MOB_BAT && !G.player;
#ifdef CRAWL_DEBUG
    casts[sp]++;
    fprintf(stderr, "EVENT cast_%s t=%.2f\n", ATK_NAME[sp], G.t);
#endif
    switch (sp) {
    case AT_CHAIN: if (have) chain_lightning(t, dmg); else fizzle(sp); return;
    case AT_DRAIN:
        if (have) {
            add_beam(AT_DRAIN, hx, hy, 0.42f, m->x, m->y, m->z + 0.5f);
            float before = m->hp;
            damage_mob(m, dmg, DM_ARC, G.px, G.py, 0.0f);
            float dealt = before - fmaxf(m->hp, 0.0f);
            G.hp = clampi(G.hp + (int)(dealt * fminf(0.8f, 0.5f + 0.05f * spell_tier(AT_DRAIN)) + 0.5f), 0, G.max_hp);
            pop_readout();
            snd_sfx_ex(SFX_WHISPER, 0.8f, 0.5f);
        } else fizzle(sp);
        return;
    case AT_BFS: bfs9000(); return;
    case AT_WISPS: {
        int n = 3 + (spell_tier(AT_WISPS) >= 3) + (spell_tier(AT_WISPS) >= 6);
        if (have) wisp_bfs(m->cx, m->cy);
        for (int i = 0; i < n; i++) {
            Shot *s = new_shot(AT_WISPS, hx, hy, a + (i - (n - 1) * 0.5f) * 0.45f, 4.5f, dmg);
            if (s) { s->target = have ? t : -1; s->life = 3.0f; }
        }
        snd_sfx_ex(SFX_SHIMMER, 0.6f, 0.5f);
        return;
    }
    default: {
        float speed = sp == AT_SPEAR ? 12.0f : (sp == AT_SPARKS ? 11.0f : (sp == AT_HELL ? 6.0f : (sp == AT_FROST ? 9.0f : 8.0f)));
        if (bat) a += (rndf() - 0.5f) * (sp == AT_SPARKS || sp == AT_FROST ? 0.3f : 0.6f);   /* bats are hard to hit */
        new_shot(sp, hx, hy, a, speed, dmg);
        snd_sfx_ex(sp == AT_FROST ? SFX_FROST : SFX_CAST, sp == AT_SPARKS ? 0.6f : (sp == AT_HELL ? 1.3f : 1.0f), 0.5f);
        return;
    }
    }
}

static void update_shots(float dt) {
    const Gear *g = &RUN.gear;
    for (int i = 0; i < MAXSHOTS; i++) {
        Shot *s = &G.shots[i];
        if (!s->alive) continue;
        s->life -= dt;
        if (s->kind == AT_WISPS) {                                 /* homing, round corners if need be */
            Mob *m = s->target >= 0 ? &G.mobs[s->target] : NULL;
            if (m && (!m->alive || m->state != MS_WALK)) { m = NULL; s->target = -1; }
            if (!m) {                                              /* find another */
                float bd = 6.0f;
                for (int k = 0; k < MAXMOBS; k++) {
                    Mob *o = &G.mobs[k];
                    if (!o->alive || o->state != MS_WALK || o->type == MOB_GHOST) continue;
                    float d = hypotf(o->x - s->x, o->y - s->y);
                    if (d < bd && line_clear(s->x, s->y, o->x, o->y)) { bd = d; m = o; s->target = k; }
                }
            }
            if (m) {
                float gx = m->x, gy = m->y;
                int cx = (int)s->x, cy = (int)s->y;
                if (!line_clear(s->x, s->y, m->x, m->y) && wisp_field[cy][cx] > 0) {
                    for (int d = 0; d < 4; d++) {
                        int nx = cx + DX[d], ny = cy + DY[d];
                        if (wisp_field[ny][nx] >= 0 && wisp_field[ny][nx] < wisp_field[cy][cx]) { gx = nx + 0.5f; gy = ny + 0.5f; break; }
                    }
                }
                float want = atan2f(gy - s->y, gx - s->x), have = atan2f(s->vy, s->vx);
                have += clampf(wrap_pi(want - have), -6.0f * dt, 6.0f * dt);
                s->vx = cosf(have) * 4.5f; s->vy = sinf(have) * 4.5f;
            }
        }
        int n = 1 + (int)(sqrtf(s->vx * s->vx + s->vy * s->vy) * dt / 0.08f);
        for (int k = 0; k < n && s->alive; k++) {
            float nx = s->x + s->vx * dt / n, ny = s->y + s->vy * dt / n;
            if (blocks_light((int)nx, (int)ny) || cell((int)nx, (int)ny)->kind == K_PILLAR) {
                if (s->kind == AT_WISPS) {                         /* wisps slide along the stone */
                    int bx = blocks_light((int)nx, (int)s->y) || cell((int)nx, (int)s->y)->kind == K_PILLAR;
                    int by = blocks_light((int)s->x, (int)ny) || cell((int)s->x, (int)ny)->kind == K_PILLAR;
                    if (bx) s->vx = -s->vx * 0.5f;
                    if (by || !bx) s->vy = -s->vy * 0.5f;
                    break;
                }
                s->alive = 0;
                if (s->kind == AT_HELL) { explode_big(s->x, s->y, 1.6f); splash(s->x, s->y, 1.5f, s->dmg, NULL); }
                else if (s->kind == AT_FROST) sfx_at(SFX_FROST, s->x, s->y, 0.6f);
                else if (s->kind == SHOT_REFLECT && s->z < 0.5f) sfx_at(SFX_BONES, s->x, s->y, 0.4f);
                else if (s->kind != AT_SPEAR && s->kind != AT_SPARKS) explode(s->x, s->y);
                break;
            }
            s->x = nx; s->y = ny;
            for (int mi = 0; mi < MAXMOBS; mi++) {
                Mob *mb = &G.mobs[mi];
                if (!mb->alive || mb->state != MS_WALK || mb->type == MOB_GHOST) continue;   /* fire passes through wraiths */
                if (s->kind == AT_SPEAR && (s->pierced >> mi & 1)) continue;
                float hr = mb->type == MOB_BAT ? 0.3f : (mb->type == MOB_WSKULL || mb->type == MOB_SPIDER ? 0.38f : 0.42f);
                if (hypotf(mb->x - s->x, mb->y - s->y) >= hr) continue;
                float fx = s->x - s->vx * 0.1f, fy = s->y - s->vy * 0.1f;
                switch (s->kind) {
                case AT_SPEAR:                                     /* through everything in the line, shields too */
                    s->pierced |= 1u << mi;
                    damage_mob(mb, s->dmg, DM_PIERCE, fx, fy, 0.0f);
                    sfx_at(SFX_BONES, mb->x, mb->y, 0.5f);
                    continue;
                case AT_HELL:
                    s->alive = 0; explode_big(s->x, s->y, 1.6f);
                    damage_mob(mb, s->dmg, 0, fx, fy, 0.0f); splash(s->x, s->y, 1.5f, s->dmg, mb);
                    for (int b = 0; b < MAXBURNS; b++)
                        if (!G.burns[b].alive) { G.burns[b] = (Burn){ 1, s->x, s->y, 0.0f, 4.0f + spell_tier(AT_HELL) }; break; }
                    break;
                case AT_FIRE:
                    s->alive = 0; explode(s->x, s->y);
                    damage_mob(mb, s->dmg, 0, fx, fy, 0.0f);
                    if (spell_tier(AT_FIRE) >= 2) splash(s->x, s->y, 0.75f, s->dmg * 0.5f, mb);
                    break;
                case AT_FROST:
                    s->alive = 0; sfx_at(SFX_FROST, s->x, s->y, 0.8f);
                    damage_mob(mb, s->dmg, DM_SLOW, fx, fy, 2.5f + 0.25f * spell_tier(AT_FROST));
                    break;
                default:                                           /* sparks, wisps, reflected missiles */
                    s->alive = 0;
                    if (s->kind == SHOT_REFLECT) explode(s->x, s->y); else sfx_at(SFX_ZAP, s->x, s->y, 0.35f);
                    damage_mob(mb, s->dmg, 0, fx, fy, 0.0f);
                    break;
                }
                break;
            }
        }
        if (s->alive && s->life <= 0) { s->alive = 0; if (s->kind == AT_FIRE || s->kind == AT_HELL) explode(s->x, s->y); }
    }
    for (int i = 0; i < MAXBOOMS; i++) if (G.booms[i].alive && (G.booms[i].t += dt) >= 0.6f) G.booms[i].alive = 0;
    for (int i = 0; i < MAXBEAMS; i++) if (G.beams[i].alive && (G.beams[i].t += dt) >= 0.3f) G.beams[i].alive = 0;
    for (int i = 0; i < MAXBURNS; i++) {                           /* the floor burns a while after hellfire */
        Burn *b = &G.burns[i];
        if (!b->alive) continue;
        float pt = b->t; b->t += dt;
        if (b->t >= 2.5f) { b->alive = 0; continue; }
        if ((int)(pt * 4.0f) != (int)(b->t * 4.0f))
            for (int k = 0; k < MAXMOBS; k++) {
                Mob *m = &G.mobs[k];
                if (m->alive && m->state == MS_WALK && m->type != MOB_GHOST && m->type != MOB_WSKULL && hypotf(m->x - b->x, m->y - b->y) < 0.7f)
                    damage_mob(m, b->dps * 0.25f, DM_ARC, b->x, b->y, 0.0f);
            }
    }
    (void)g;
}

/* ---- the caster's defences: tattooed runes, each waiting for its moment ---- */

static int incoming(float within) {                                 /* missiles that will reach the caster soon */
    int n = 0;
    for (int i = 0; i < MAXBONES; i++) {
        const Bone *b = &G.bones[i];
        if (!b->alive) continue;
        float dx = G.px - b->x, dy = G.py - b->y, sp = hypotf(b->vx, b->vy) + 1e-3f;
        float along = (dx * b->vx + dy * b->vy) / sp, off = fabsf(dx * b->vy - dy * b->vx) / sp;
        if (along > 0.0f && off < 0.6f && along / sp < within) n++;
    }
    return n;
}
static int enemies_within(float r, int undead_only) {
    int n = 0;
    for (int i = 0; i < MAXMOBS; i++) {
        const Mob *m = &G.mobs[i];
        if (!m->alive || m->state != MS_WALK || m->type == MOB_GHOST || m->type == MOB_WSKULL) continue;
        if (m->type == MOB_SPIDER && m->sub == 0) continue;
        if (undead_only && m->type != MOB_SKEL && m->type != MOB_SHIELD) continue;
        if (hypotf(m->x - G.px, m->y - G.py) < r && line_clear(m->x, m->y, G.px, G.py)) n++;
    }
    return n;
}
static int skull_charging(float r, float after) {
    for (int i = 0; i < MAXMOBS; i++) {
        const Mob *m = &G.mobs[i];
        if (m->alive && m->type == MOB_WSKULL && m->state == MS_WALK && m->sub == 1 && m->sub_t >= after
            && hypotf(m->x - G.px, m->y - G.py) < r && line_clear(m->x, m->y, G.px, G.py)) return 1;
    }
    return 0;
}
static int melee_close(void) {                                      /* a sword raised, or a spider coming down */
    for (int i = 0; i < MAXMOBS; i++) {
        const Mob *m = &G.mobs[i];
        if (!m->alive || m->state != MS_WALK) continue;
        float d = hypotf(m->x - G.px, m->y - G.py);
        if ((m->type == MOB_SHIELD && d < 1.5f) || (m->type == MOB_SPIDER && m->sub >= 1 && m->sub <= 2 && d < 1.4f)) return 1;
    }
    return 0;
}
static int in_combat(void) { return enemies_within(7.0f, 0) > 0 || incoming(2.0f) > 0 || skull_charging(9.0f, 0.0f); }

static int def_trigger(int k) {
    float hpf = (float)G.hp / (float)G.max_hp;
    switch (k) {
    case DF_WARD: return G.ward_hp <= 0.0f && (incoming(0.7f) || skull_charging(7.0f, 1.5f) || melee_close());
    case DF_BLINK: return (melee_close() && hpf < 0.6f) || (incoming(0.4f) && G.ward_hp <= 0.0f && G.mirror_t <= 0.0f && hpf < 0.5f);
    case DF_MIRROR: return G.mirror_t <= 0.0f && (incoming(1.5f) >= 2 || skull_charging(9.0f, 2.0f));
    case DF_MEND: return G.mend_t <= 0.0f && hpf < 0.45f;
    case DF_PURIFY: return G.poison_t > 2.0f || G.slow_t > 1.5f;
    case DF_NOVA: return enemies_within(1.8f, 0) >= 2 || melee_close();
    case DF_DREAD: return enemies_within(4.0f, 1) >= 2;
    case DF_SLIP: return G.slip_t <= 0.0f && (enemies_within(5.0f, 0) + incoming(2.0f) >= 3 || (hpf < 0.35f && in_combat()));
    default: return G.reap_t <= 0.0f && enemies_within(7.0f, 0) >= 2;
    }
}

static void begin_turn(float target);
static float heading_angle(int h);
static int decide(int x, int y, int h);

/* Blink: two or three steps to wherever is furthest from trouble, out of sight if it can */
static void blink(void) {
    static int dist[MAPN][MAPN], qx[MAPN * MAPN], qy[MAPN * MAPN];
    int range = 2 + RUN.gear.def[DF_BLINK] / 3;
    for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) dist[y][x] = -1;
    int qh = 0, qt = 0, bx = -1, by = -1; float bs = -1e9f;
    dist[G.cy][G.cx] = 0; qx[qt] = G.cx; qy[qt] = G.cy; qt++;
    while (qh < qt) {
        int x = qx[qh], y = qy[qh]; qh++;
        if (dist[y][x] >= 2 && G.map[y][x].kind == K_FLOOR && !occupied(x, y, NULL)) {
            float s = dist[y][x] * 0.3f;
            for (int i = 0; i < MAXMOBS; i++) {
                const Mob *m = &G.mobs[i];
                if (!m->alive || m->state != MS_WALK || m->type == MOB_GHOST) continue;
                float d = hypotf(m->x - x - 0.5f, m->y - y - 0.5f);
                s += fminf(d, 6.0f) * 0.5f - (line_clear(m->x, m->y, x + 0.5f, y + 0.5f) ? 1.5f : 0.0f);
            }
            if (s > bs) { bs = s; bx = x; by = y; }
        }
        if (dist[y][x] >= range) continue;
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (!passable(nx, ny) || dist[ny][nx] >= 0 || cell(nx, ny)->kind == K_DOOR) continue;
            dist[ny][nx] = dist[y][x] + 1; qx[qt] = nx; qy[qt] = ny; qt++;
        }
    }
    if (bx < 0) return;
    G.cx = G.tx = bx; G.cy = G.ty = by; G.px = bx + 0.5f; G.py = by + 0.5f; G.u = 1.0f;
    G.spd = 0.0f;
    if (!G.player) {
        G.hand = 0.0f;
        G.next_h = decide(G.cx, G.cy, G.h);
        begin_turn(heading_angle(G.next_h)); G.state = PS_TURN;
    }
    G.chill = fmaxf(G.chill, 0.5f);
    snd_sfx_ex(SFX_WHOOSH, 1.0f, 0.5f);
}

static void cast_defense(int k) {
    int t = RUN.gear.def[k];
    if (t <= 0) return;
    G.def_cd[k] = def_cooldown(t);
    G.rune_glow[k] = 1.4f; G.rune_lift = 1.4f;
    snd_sfx_ex(SFX_RUNE, 0.9f, 0.5f);
#ifdef CRAWL_DEBUG
    defs[k]++;
    fprintf(stderr, "EVENT defense %s t=%.2f hp=%d\n", DEF_NAME[k], G.t, G.hp);
#endif
    switch (k) {
    case DF_WARD: G.ward_hp = 10.0f + 5.0f * t; G.ward_t = 12.0f; break;
    case DF_BLINK: blink(); break;
    case DF_MIRROR: G.mirror_t = 3.0f + 0.5f * t; break;
    case DF_MEND: G.mend_t = 4.0f; G.mend_rate = (15.0f + 5.0f * t) / 4.0f; break;
    case DF_PURIFY: G.poison_t = G.slow_t = 0.0f; G.purify_t = 4.0f + t; break;
    case DF_NOVA:
        for (int i = 0; i < MAXMOBS; i++) {
            Mob *m = &G.mobs[i];
            if (m->alive && m->state == MS_WALK && m->type != MOB_GHOST && hypotf(m->x - G.px, m->y - G.py) < 1.8f + 0.1f * t)
                { m->frozen = 2.0f + 0.3f * t; m->swing = 0.0f; m->throw_t = 0.0f; }
        }
        snd_sfx_ex(SFX_FROST, 1.3f, 0.5f);
        G.nova_t = 0.001f; G.chill = fmaxf(G.chill, 0.55f);
        break;
    case DF_DREAD:
        for (int i = 0; i < MAXMOBS; i++) {
            Mob *m = &G.mobs[i];
            if (m->alive && m->state == MS_WALK && (m->type == MOB_SKEL || m->type == MOB_SHIELD || m->type == MOB_WSKULL)
                && hypotf(m->x - G.px, m->y - G.py) < 5.0f) m->feared = 3.0f + 0.5f * t;
        }
        G.dread_t = 0.001f;
        snd_sfx_ex(SFX_MOAN, 1.2f, 0.5f);
        break;
    case DF_SLIP: G.slip_t = 3.0f + 0.4f * t; break;
    default: G.reap_t = 15.0f + t; break;
    }
}

/* each rune watches for its moment; the caster is only human: sometimes slow, sometimes not at all */
static void update_defense(float dt) {
    const Gear *g = &RUN.gear;
    int fighting = in_combat();
    for (int k = 0; k < NDEF; k++) {
        if (g->def[k] <= 0) continue;
        G.def_cd[k] -= dt; G.def_hush[k] -= dt;
        if (G.def_wait[k] > 0.0f) {
            G.def_wait[k] -= dt;
            if (G.def_wait[k] <= 0.0f && G.def_cd[k] <= 0.0f && !G.dead) cast_defense(k);
            continue;
        }
        if (G.def_cd[k] > 0.0f || G.def_hush[k] > 0.0f || G.dead) continue;
        if (def_trigger(k)) {
            if (rndf() < 0.12f) { G.def_hush[k] = 1.5f; continue; }   /* missed the moment */
            G.def_wait[k] = 0.15f + 0.55f * rndf() + (rndf() < 0.1f ? 0.8f : 0.0f);
        } else if (fighting && rndf() < dt * 0.004f) cast_defense(k);   /* a nervous, wasted cast */
    }
}

/* poison, slow and the timed spells wearing off; mending and blessed regeneration */
static void update_status(float dt) {
    if (G.poison_t > 0.0f) {
        G.poison_t -= dt;
        G.poison_acc += G.poison_dps * dt;
        while (G.poison_acc >= 1.0f && !G.dead) { G.poison_acc -= 1.0f; hurt_ex(1.0f, HURT_POISON, G.px, G.py); }
    } else G.poison_acc = 0.0f;
    G.slow_t = fmaxf(0.0f, G.slow_t - dt);
    G.purify_t = fmaxf(0.0f, G.purify_t - dt);
    if (G.ward_t > 0.0f && (G.ward_t -= dt) <= 0.0f) G.ward_hp = 0.0f;
    G.mirror_t = fmaxf(0.0f, G.mirror_t - dt);
    G.slip_t = fmaxf(0.0f, G.slip_t - dt);
    G.reap_t = fmaxf(0.0f, G.reap_t - dt);
    for (int k = 0; k < NDEF; k++) G.rune_glow[k] = fmaxf(0.0f, G.rune_glow[k] - dt);
    G.rune_lift = fmaxf(0.0f, G.rune_lift - dt);
    if (G.nova_t > 0.0f && (G.nova_t += dt) > 0.6f) G.nova_t = 0.0f;
    if (G.dread_t > 0.0f && (G.dread_t += dt) > 0.8f) G.dread_t = 0.0f;
    float heal = 0.0f;
    if (G.mend_t > 0.0f) { G.mend_t -= dt; heal += G.mend_rate * dt; }
    heal += gear_regen(&RUN.gear) * dt;
    if (heal > 0.0f && G.hp < G.max_hp && !G.dead) {
        G.regen_acc += heal;
        if (G.regen_acc >= 1.0f) { int h = (int)G.regen_acc; G.regen_acc -= h; G.hp = clampi(G.hp + h, 0, G.max_hp); if (G.mend_t > 0.0f) pop_readout(); }
    }
}

static void update_doors(float dt) {
    for (int i = 0; i < G.ndoors; i++) {
        Door *d = &G.doors[i];
        float pdist = hypotf(d->x + 0.5f - G.px, d->y + 0.5f - G.py);
        if (d->opening) {
            d->open = fminf(1.0f, d->open + dt / 0.85f);
            if (d->open >= 1.0f) { d->opening = 0; d->idle = 0.0f; }
        } else if (d->open > 0.0f) {
            int busy = pdist < 1.6f || (d->locked == 2 && vault_of_room((int)G.px, (int)G.py) == d->color);
            for (int m = 0; m < MAXMOBS; m++)
                if (G.mobs[m].alive && ((G.mobs[m].cx == d->x && G.mobs[m].cy == d->y) || (G.mobs[m].tx == d->x && G.mobs[m].ty == d->y))) busy = 1;
            d->idle = busy ? 0.0f : d->idle + dt;
            if (d->idle > 2.5f) {
                if (d->open >= 1.0f) sfx_at(SFX_DOOR, d->x + 0.5f, d->y + 0.5f, 0.6f);
                d->open = fmaxf(0.0f, d->open - dt / 0.85f);
            }
        }
    }
}

/* ================================================================== */
/* The caster's walk                                                   */
/* ================================================================== */

static float heading_angle(int h) { return h * (PI_F * 0.5f); }
static void begin_turn(float target);

/* steps from (sx,sy) to the nearest unexplored cell, not passing back through (bx,by) */
static int dist_to_unexplored(int sx, int sy, int bx, int by) {
    static int stamp[MAPN][MAPN], dist[MAPN][MAPN], qx[MAPN * MAPN], qy[MAPN * MAPN], gen;
    gen++;
    int qh = 0, qt = 0;
    stamp[sy][sx] = gen; dist[sy][sx] = 0; qx[qt] = sx; qy[qt] = sy; qt++;
    stamp[by][bx] = gen;
    while (qh < qt) {
        int x = qx[qh], y = qy[qh]; qh++;
        if (G.map[y][x].visits == 0) return dist[y][x];
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (!passable(nx, ny) || stamp[ny][nx] == gen) continue;
            stamp[ny][nx] = gen; dist[ny][nx] = dist[y][x] + 1; qx[qt] = nx; qy[qt] = ny; qt++;
        }
    }
    return 999;
}

static int bfs_dist[MAPN][MAPN];
static void bfs_from(int sx, int sy) {
    static int qx[MAPN * MAPN], qy[MAPN * MAPN];
    for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) bfs_dist[y][x] = -1;
    int qh = 0, qt = 0;
    bfs_dist[sy][sx] = 0; qx[qt] = sx; qy[qt] = sy; qt++;
    while (qh < qt) {
        int x = qx[qh], y = qy[qh]; qh++;
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (!passable(nx, ny) || bfs_dist[ny][nx] >= 0) continue;
            bfs_dist[ny][nx] = bfs_dist[y][x] + 1; qx[qt] = nx; qy[qt] = ny; qt++;
        }
    }
}

/* the way to open vault k: its key (which may be locked up in another vault), then its door */
static int key_goal(int k, int depth, int *gx, int *gy) {
    if (depth > 3 || k < 0 || G.vault_door[k] < 0) return 0;
    const Door *d = &G.doors[G.vault_door[k]];
    if (d->locked != 1) return 0;
    if (G.has_key[k]) { *gx = d->x; *gy = d->y; return 1; }
    for (int i = 0; i < G.nitems; i++) {
        const Item *it = &G.items[i];
        if (!it->alive || it->type != IT_KEY || it->color != k) continue;
        int v = vault_of_room(it->cx, it->cy);
        if (v >= 0 && v != k && G.doors[G.vault_door[v]].locked == 1) return key_goal(v, depth + 1, gx, gy);
        *gx = it->cx; *gy = it->cy;
        return 1;
    }
    return 0;
}

/* a key whose vault is still shut: the caster can hear it chime from a little way off */
static int key_calls(const Item *it) {
    if (it->type != IT_KEY || it->color < 0 || it->color >= G.nvaults || G.vault_door[it->color] < 0) return 0;
    return G.doors[G.vault_door[it->color]].locked == 1;
}

static int worth_taking(const Item *it) {
    if (it->type == IT_POTION) return G.hp < G.max_hp * 4 / 5;      /* saved for when it is needed */
    return 1;
}

/* something worth walking to from (x,y): a vault door once its key is held, else treasure it has seen */
static int pick_goal(int x, int y, int *gx, int *gy) {
    int saving = RUN.goal > RUN.treasure;
    if (G.leaving && G.exit_fx >= 0) {                              /* time to go: the exit, by way of key and vault if need be */
        if (G.exit_in_vault >= 0 && key_goal(G.exit_in_vault, 0, gx, gy)) return 1;
        if (G.level_t < 320.0) {                                    /* but first, treasure it remembers seeing */
            for (int k = 0; k < G.nvaults; k++)                     /* a key in hand opens a hoard on the way out */
                if (G.has_key[k] && G.vault_door[k] >= 0 && G.doors[G.vault_door[k]].locked == 1) {
                    *gx = G.doors[G.vault_door[k]].x; *gy = G.doors[G.vault_door[k]].y;
                    return 1;
                }
            bfs_from(x, y);
            int best = -1, bd = 31;
            for (int i = 0; i < G.nitems; i++) {
                const Item *it = &G.items[i];
                if (!it->alive || !worth_taking(it) || (it->cx == x && it->cy == y)) continue;
                int d = bfs_dist[it->cy][it->cx];
                if (it->type == IT_KEY) { if (d < 0 || d > 22 || !key_calls(it)) continue; d -= 8; }   /* a vault's worth: worth the walk */
                else if (!it->seen) continue;
                if (d >= 0 && d < bd) { bd = d; best = i; }
            }
            if (best >= 0) { *gx = G.items[best].cx; *gy = G.items[best].cy; return 1; }
        }
        *gx = G.exit_fx; *gy = G.exit_fy;
        return 1;
    }
    for (int k = 0; k < G.nvaults; k++)                             /* a key in hand: straight to its door */
        if (G.has_key[k] && G.vault_door[k] >= 0 && G.doors[G.vault_door[k]].locked == 1) {
            *gx = G.doors[G.vault_door[k]].x; *gy = G.doors[G.vault_door[k]].y;
            return 1;
        }
    bfs_from(x, y);
    int best = -1; float bs = 1e9f;
    int reach = saving ? 30 : 24;                                   /* gold buys things now: it goes out of its way */
    for (int i = 0; i < G.nitems; i++) {
        const Item *it = &G.items[i];
        if (!it->alive || (it->cx == x && it->cy == y) || !worth_taking(it)) continue;
        int d = bfs_dist[it->cy][it->cx];
        if (!it->seen && !(it->type == IT_KEY && d >= 0 && d <= 14 && key_calls(it))) continue;   /* a key's jingle carries */
        if (d < 0 || d > reach) continue;
        float sc = (float)d - (it->type == IT_KEY ? 8.0f : 0.0f) - (it->type == IT_POTION ? 6.0f : 0.0f)
                 - (it->type == IT_CROWN || it->type == IT_CHEST || it->type == IT_RING ? 4.0f : 0.0f);
        if (sc < bs) { bs = sc; best = i; }
    }
    if (best < 0) return 0;
    *gx = G.items[best].cx; *gy = G.items[best].cy;
    return 1;
}

/* which way to leave (x,y): towards a goal if there is one, else new ground first,
 * otherwise towards the nearest unexplored cell */
static int decide(int x, int y, int h) {
    int gx, gy;
    if (pick_goal(x, y, &gx, &gy)) {
        bfs_from(gx, gy);
        int gbest = -1, gs = 1 << 30;
        for (int d = 0; d < 4; d++) {
            int nx = x + DX[d], ny = y + DY[d];
            if (!passable(nx, ny) || bfs_dist[ny][nx] < 0) continue;
            int sc = bfs_dist[ny][nx] * 4 + (d == h ? 0 : 1) + (d == ((h + 2) & 3) ? 2 : 0) + (G.bulky[ny][nx] ? 5 : 0);
            if (sc < gs) { gs = sc; gbest = d; }
        }
        if (gbest >= 0 && bfs_dist[y][x] > 0) return gbest;
    }
    int best = -1; float bs = -1e9f;
    for (int d = 0; d < 4; d++) {
        if (d == ((h + 2) & 3)) continue;
        int nx = x + DX[d], ny = y + DY[d];
        if (!passable(nx, ny)) continue;
        float s;
        if (G.map[ny][nx].visits == 0) s = 100.0f + (d == h ? 1.3f : 0.0f) + rndf() * 1.6f;
        else s = -(float)dist_to_unexplored(nx, ny, x, y) + (d == h ? 0.4f : 0.0f) + rndf() * 0.8f;
        if (G.bulky[ny][nx]) s -= 2.5f;                           /* walk past piles, not through them */
        if (s > bs) { bs = s; best = d; }
    }
    return best < 0 ? ((h + 2) & 3) : best;
}

static void start_move(void) {
    G.tx = G.cx + DX[G.h]; G.ty = G.cy + DY[G.h];
    if (!passable(G.tx, G.ty)) {                                   /* should not happen: turn instead */
        G.next_h = decide(G.cx, G.cy, G.h);
        G.tx = G.cx; G.ty = G.cy; G.u = 1.0f;
        G.state = PS_MOVE;
        return;
    }
    G.next_h = decide(G.tx, G.ty, G.h);
    G.u = 0.0f;
    Cell *c = &G.map[G.ty][G.tx];
    if (c->kind == K_DOOR && G.doors[c->door].open < 0.95f) {
        Door *d = &G.doors[c->door];
        if (d->locked == 1) { d->locked = 2; G.has_key[d->color] = 0; G.unlock_t = G.level_t; snd_sfx(SFX_UNLOCK); pop_readout(); }
        if (!d->opening) { d->opening = 1; sfx_at(SFX_DOOR, d->x + 0.5f, d->y + 0.5f, 1.0f); }
        G.state = PS_DOOR;
        G.spd = 0.0f;
        return;
    }
    G.state = PS_MOVE;
}

/* returns 1 when this step brought the caster to the exit, which takes over from walking */
static int arrive(void) {
    G.cx = G.tx; G.cy = G.ty;
    G.px = G.cx + 0.5f; G.py = G.cy + 0.5f;
    if (G.leaving && !G.exiting && G.cx == G.exit_fx && G.cy == G.exit_fy) {   /* the way out: face it, open it, go */
        G.exiting = 1; G.exit_t = 0.0f; G.u = 1.0f;
        begin_turn(heading_angle(G.exit_dir)); G.adur = 0.4f;
        G.state = PS_EXIT; G.spd = 0.0f; G.hand = 0.0f;
        return 1;
    }
    if (G.map[G.cy][G.cx].visits < 250) {
        if (G.map[G.cy][G.cx].visits == 0) G.explored++;
        G.map[G.cy][G.cx].visits++;
    }
    return 0;
}

static void advance(float du) {                     /* move along the current step, with head bob and footsteps */
    G.u += du;
    float k = fminf(G.u, 1.0f);
    G.px = lerpf(G.cx + 0.5f, G.tx + 0.5f, k);
    G.py = lerpf(G.cy + 0.5f, G.ty + 0.5f, k);
    float ph0 = G.bobph;
    G.bobph += du * TAU_F;
    if (floorf(ph0 / PI_F) != floorf(G.bobph / PI_F)) snd_sfx_ex(SFX_STEP, 1.0f, ((int)(G.bobph / PI_F) & 1) ? 0.44f : 0.56f);
}

static void begin_turn(float target) {
    G.ang0 = G.ang;
    G.ang1 = G.ang + wrap_pi(target - G.ang);
    float delta = fabsf(G.ang1 - G.ang0);
    G.adur = delta > 2.0f ? 0.55f : (delta > 0.2f ? 0.30f : 0.05f);
    G.st = 0.0f;
}

/* can the caster pick this one out? a spider in its web shows only its eyes, and only a hunter's
 * hood lets the caster pick those out in time */
static int targetable(const Mob *m) {
    if (!m->alive || m->state != MS_WALK || m->type == MOB_GHOST) return 0;
    if ((m->type == MOB_SPIDER || m->type == MOB_WSKULL) && m->sub == 0) {
        float h = gear_hunter(&RUN.gear), notice = h > 1.0f ? fminf(1.5f + (h - 1.0f) * 12.0f, 8.0f) : 0.0f;
        return hypotf(m->x - G.px, m->y - G.py) < notice;
    }
    return 1;
}

/* what to throw at: anything ahead, plus whatever is attacking from the side or behind
 * (a skeleton that has spotted the caster, a bat at its face, a sword, a spider, a skull's fire) */
static int find_target(void) {
    float dx = cosf(G.ang), dy = sinf(G.ang), rx = -dy, ry = dx;
    int best = -1; float bd = 1e9f;
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *m = &G.mobs[i];
        if (!targetable(m)) continue;
        float vx = m->x - G.px, vy = m->y - G.py, dist = sqrtf(vx * vx + vy * vy);
        float depth = vx * dx + vy * dy, lat = vx * rx + vy * ry, score;
        if (depth >= 0.35f && depth <= 7.5f && fabsf(lat) <= fmaxf(depth * 0.42f, 0.3f)) score = depth;
        else if ((m->type == MOB_SKEL && m->seen && dist < 8.0f) || (m->type == MOB_BAT && dist < 1.4f) || (m->type == MOB_SHIELD && m->seen && dist < 3.0f)
                 || (m->type == MOB_SPIDER && m->sub >= 1 && dist < 2.0f) || (m->type == MOB_WSKULL && m->sub >= 1 && dist < 9.0f)) score = dist + 1.5f;
        else continue;
        if (m->type == MOB_WSKULL) score -= m->sub == 1 ? 4.0f : 1.5f;   /* one hit ends it: first, before it fires */
        if (m->ignore > (float)G.t || !shot_clear(m->x, m->y)) continue;
        if (score < bd) { bd = score; best = i; }
    }
    return best;
}

/* which spell for this target: a crowd, a shield, a bat, a skull, or low on health */
static int choose_spell(int t) {
    const Gear *g = &RUN.gear;
    const Mob *m = &G.mobs[t];
    float hpf = (float)G.hp / (float)G.max_hp;
    int vis = 0, close = 0, near = 0, packed = 0;
    for (int i = 0; i < MAXMOBS; i++) {
        const Mob *o = &G.mobs[i];
        if (!targetable(o)) continue;
        float d = hypotf(o->x - G.px, o->y - G.py);
        if (d < 10.0f && line_clear(G.px, G.py, o->x, o->y)) { vis++; if (d < 4.0f) close++; }
        if (o != m && hypotf(o->x - m->x, o->y - m->y) < 3.5f) { near++; if (hypotf(o->x - m->x, o->y - m->y) < 1.5f) packed++; }
    }
    if (g->atk[AT_BFS] >= 0 && RUN.souls >= bfs_cost(g->atk[AT_BFS]) && (vis >= 4 || (hpf < 0.3f && close >= 2))) return AT_BFS;
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_BFSANY") && g->atk[AT_BFS] >= 0 && RUN.souls >= bfs_cost(g->atk[AT_BFS]) && vis >= 2) return AT_BFS;
#endif
    int owned[NATK], no = 0, pick = AT_FIRE;
    float best = -1.0f;
    int shielded = m->type == MOB_SHIELD && m->shield > 0.0f;
    for (int sp = 0; sp < AT_BFS; sp++) {
        if (g->atk[sp] < 0) continue;
        owned[no++] = sp;
        float v = spell_damage(g, sp) / spell_cycle(g, sp);
        if (sp == AT_WISPS) v *= (float)(3 + (g->atk[sp] >= 3) + (g->atk[sp] >= 6));
        if (m->type == MOB_WSKULL) v = 1.0f / spell_cycle(g, sp);   /* any hit will do: the quickest */
        switch (sp) {
        case AT_SPEAR: if (shielded) v *= 2.2f; if (near) v *= 1.3f; break;
        case AT_CHAIN: v *= 1.0f + 0.6f * (float)(near < 3 ? near : 3); if (shielded) v *= 1.8f; break;
        case AT_HELL: v *= 1.0f + 0.8f * packed; break;
        case AT_DRAIN: if (hpf < 0.45f) v *= 2.2f; break;
        case AT_FROST: if (m->type == MOB_BAT || m->type == MOB_SPIDER || m->type == MOB_SHIELD) v *= 1.6f; else v *= 0.8f; break;
        case AT_SPARKS: if (m->type == MOB_BAT) v *= 1.5f; break;
        case AT_WISPS: if (m->type == MOB_BAT || m->type == MOB_SPIDER) v *= 1.6f; break;
        default: break;
        }
        if (sp == AT_WISPS) v *= 0.75f;                            /* not every wisp gets there */
        if (sp == G.spell) v *= 0.8f;                              /* and a little variety */
        v *= 0.85f + 0.3f * rndf();
        if (v > best) { best = v; pick = sp; }
    }
    if (no > 1 && rnd(100) < 12) pick = owned[rnd(no)];             /* a whim */
    return pick;
}

static void aim_at(int t) {
    G.target = t; G.resume_state = PS_MOVE;
    G.spell = choose_spell(t);
    float cyc = spell_cycle(&RUN.gear, G.spell);
    G.cast_len = fminf(0.75f, fmaxf(0.3f, cyc * 0.42f));
    begin_turn(atan2f(G.mobs[t].y - G.py, G.mobs[t].x - G.px));
    G.adur = 0.18f + 0.13f * fabsf(G.ang1 - G.ang0);
    G.state = PS_AIM; G.spd = 0.0f;
}

static void update_player(float dt) {
    G.cooldown -= dt;
    G.calm += dt;
    switch (G.state) {
    case PS_MOVE: {
        int keyed = G.has_key[0] || G.has_key[1] || G.has_key[2];
        float target = (keyed ? CRUISE * 1.25f : CRUISE) * gear_haste(&RUN.gear) * (G.slow_t > 0.0f ? 0.6f : 1.0f);   /* a key in hand: hurry */
        if (G.next_h != G.h && G.u > 0.62f) target = CRUISE * 0.45f;
        int blocker = -1;                                          /* something in the way: stop and deal with it */
        for (int i = 0; i < MAXMOBS && G.blocked < 3.0f; i++) {
            Mob *m = &G.mobs[i];
            if (!solid_mob(m) || m->state != MS_WALK) continue;
            float vx = m->x - G.px, vy = m->y - G.py;
            float ahead = vx * cosf(G.ang) + vy * sinf(G.ang), side = -vx * sinf(G.ang) + vy * cosf(G.ang);
            if (ahead > 0.0f && ahead < 1.3f && fabsf(side) < 0.55f && line_clear(G.px, G.py, m->x, m->y)) blocker = i;
        }
        if (blocker >= 0) {
            target = 0.0f; G.spd = fminf(G.spd, 0.3f); G.blocked += dt;
            if (G.cooldown <= 0.0f) { aim_at(blocker); break; }
        } else G.blocked = 0.0f;
        G.spd += clampf(target - G.spd, -6.0f * dt, 6.0f * dt);
        advance(G.spd * dt);
        if (G.next_h != G.h && G.u >= 0.78f) {
            /* corner ahead: start the quick turn while finishing the step */
            begin_turn(heading_angle(G.next_h));
            G.state = PS_TURN;
            break;
        }
        if (G.u >= 1.0f) {
            if (arrive()) break;
            if (G.next_h != G.h) {
                begin_turn(heading_angle(G.next_h));
                G.state = PS_TURN;
            } else start_move();
        }
        if (G.cooldown <= 0.0f && G.state == PS_MOVE) {
            int t = find_target();
            if (t >= 0) aim_at(t);
        }
        break;
    }
    case PS_TURN: {
        G.st += dt;
        if (G.u < 1.0f) {                                         /* still finishing the step into the corner */
            advance(fmaxf(G.spd, CRUISE * 0.4f) * dt);
            if (G.u >= 1.0f && arrive()) break;
        } else G.spd *= 0.85f;
        float k = smoothstepf(0.0f, 1.0f, G.st / G.adur);
        G.ang = lerpf(G.ang0, G.ang1, k);
        if (G.st >= G.adur) {
            if (G.u < 1.0f) { advance(1.0f - G.u); if (arrive()) break; }
            G.ang = wrap_pi(G.ang1);
            G.h = G.next_h;
            G.spd = CRUISE * 0.6f;
            start_move();
        }
        break;
    }
    case PS_DOOR: {
        Cell *c = cell(G.tx, G.ty);
        if (c->kind != K_DOOR || G.doors[c->door].open >= 0.95f) G.state = PS_MOVE;
        break;
    }
    case PS_AIM: {
        G.st += dt;
        Mob *m = &G.mobs[G.target];
        if (m->alive) G.ang1 = G.ang0 + wrap_pi(atan2f(m->y - G.py, m->x - G.px) - G.ang0);
        G.ang = lerpf(G.ang0, G.ang1, smoothstepf(0.0f, 1.0f, G.st / G.adur));
        if (G.st >= G.adur) { G.state = PS_CAST; G.st = 0.0f; G.fired = 0; }
        break;
    }
    case PS_CAST: {
        G.st += dt;
        float L = G.cast_len, up = 0.24f * L, fire = 0.27f * L, down = 0.73f * L;
        G.hand = G.st < up ? G.st / up : (G.st > down ? fmaxf(0.0f, 1.0f - (G.st - down) / (0.24f * L)) : 1.0f);
        if (!G.fired && G.st >= fire) {
            G.fired = 1;
            if (G.spell == AT_BFS && RUN.souls < bfs_cost(RUN.gear.atk[AT_BFS])) G.spell = AT_FIRE;
            cast_spell(G.spell, G.target);
            G.calm = 0.0f;
        }
        if (G.st >= L) {
            Mob *m = &G.mobs[G.target];
            G.hand = 0.0f; G.cooldown = fmaxf(0.15f, spell_cycle(&RUN.gear, G.spell) - L);
            if (m->alive && ++m->misses >= 3) m->ignore = (float)G.t + 5.0f;   /* three throws and no hit: leave it */
            int stand = m->type == MOB_SKEL || m->type == MOB_SHIELD || m->type == MOB_WSKULL || (m->type == MOB_SPIDER && m->sub >= 1);
            if (targetable(m) && stand && hypotf(m->x - G.px, m->y - G.py) < 8.0f
                && m->ignore < (float)G.t && shot_clear(m->x, m->y)) { G.state = PS_HOLD; G.st = 0.0f; break; }   /* stand and finish it */
            begin_turn(heading_angle(G.h)); G.adur = 0.18f + 0.13f * fabsf(G.ang1 - G.ang0); G.state = PS_UNAIM;
        }
        break;
    }
    case PS_HOLD: {
        Mob *m = &G.mobs[G.target];
        G.st += dt;
        int ok = targetable(m) && shot_clear(m->x, m->y) && G.st < 6.0f;
        if (ok) G.ang += wrap_pi(atan2f(m->y - G.py, m->x - G.px) - G.ang) * fminf(1.0f, dt * 10.0f);
        if (!ok) { begin_turn(heading_angle(G.h)); G.adur = 0.18f + 0.13f * fabsf(G.ang1 - G.ang0); G.state = PS_UNAIM; }
        else if (G.cooldown <= 0.0f) {
            G.spell = choose_spell(G.target);
            G.cast_len = fminf(0.75f, fmaxf(0.3f, spell_cycle(&RUN.gear, G.spell) * 0.42f));
            G.state = PS_CAST; G.st = 0.0f; G.fired = 0;
        }
        break;
    }
    case PS_EXIT: {
        G.st += dt;
        float pt = G.exit_t;
        G.exit_t += dt;
        G.ang = G.st < G.adur ? lerpf(G.ang0, G.ang1, smoothstepf(0.0f, 1.0f, G.st / G.adur)) : G.ang1;
        if (pt < 0.4f && G.exit_t >= 0.4f) { snd_sfx(SFX_DOOR); snd_sfx_ex(SFX_SHIMMER, 0.8f, 0.5f); }
        G.exit_open = clampf((G.exit_t - 0.4f) / 0.7f, 0.0f, 1.0f);
        if (G.exit_t > 1.2f && G.exit_t < 2.1f) { G.px += cosf(G.ang) * dt * 0.5f; G.py += sinf(G.ang) * dt * 0.5f; G.bobph += dt * 5.0f; }
        if (G.exit_t >= 2.4f && !G.tally) {
            G.tally = 1; G.tally_t = 0.0; G.clear_time = G.level_t;
            snd_sfx_ex(SFX_FANFARE, 1.0f, 0.5f);
        }
        break;
    }
    case PS_UNAIM:
        G.st += dt;
        G.ang = lerpf(G.ang0, G.ang1, smoothstepf(0.0f, 1.0f, G.st / G.adur));
        if (G.st >= G.adur) { G.ang = wrap_pi(G.ang1); G.state = G.resume_state; }
        break;
    case PS_SETTLE: {                                              /* handed back: to the middle of the cell, square to the walls */
        G.st += dt;
        float k = smoothstepf(0.0f, 1.0f, fminf(1.0f, G.st / 0.45f));
        G.px = lerpf(G.sx0, G.cx + 0.5f, k); G.py = lerpf(G.sy0, G.cy + 0.5f, k);
        G.ang = lerpf(G.ang0, G.ang1, k);
        G.hand = fmaxf(0.0f, G.hand - dt * 2.0f);
        if (G.st >= 0.45f) { G.ang = wrap_pi(G.ang1); G.u = 1.0f; G.tx = G.cx; G.ty = G.cy; start_move(); }
        break;
    }
    }
    float amp = 0.014f * clampf(G.spd / CRUISE, 0.0f, 1.0f);
    G.camz = 0.5f + amp * sinf(G.bobph);
}

/* ================================================================== */
/* Level flow                                                          */
/* ================================================================== */

static void begin_shop(void);
static void human_begin(void);

static void reset_caster_state(void) {
    G.poison_t = G.poison_dps = G.poison_acc = G.slow_t = G.purify_t = G.regen_acc = 0.0f;
    G.ward_hp = G.ward_t = G.mirror_t = G.slip_t = G.reap_t = G.mend_t = G.mend_rate = 0.0f;
    memset(G.def_cd, 0, sizeof G.def_cd); memset(G.def_wait, 0, sizeof G.def_wait);
    memset(G.def_hush, 0, sizeof G.def_hush); memset(G.rune_glow, 0, sizeof G.rune_glow);
    G.bfs_t = 0.0f; G.face_t = G.grin_t = 0.0f;
    G.max_hp = gear_max_hp(&RUN.gear);
    if (G.hp > G.max_hp) G.hp = G.max_hp;
}

static void start_level(void) {
    memset(G.has_key, 0, sizeof G.has_key);
    G.shop = 0;
    crawl_art_level(G.level);
    gen_level();
    memset(G.mobs, 0, sizeof G.mobs);
    memset(G.shots, 0, sizeof G.shots);
    memset(G.booms, 0, sizeof G.booms);
    memset(G.bones, 0, sizeof G.bones);
    memset(G.beams, 0, sizeof G.beams);
    memset(G.burns, 0, sizeof G.burns);
    reset_caster_state();
    const Room *R = &G.rooms[0];
    G.cx = R->x + R->w / 2; G.cy = R->y + R->h / 2;
    if (G.map[G.cy][G.cx].kind != K_FLOOR) { G.cx = R->x; G.cy = R->y; }
    G.exit_seen = G.leaving = G.exiting = 0; G.exit_open = G.exit_t = 0.0f;
    G.kills = G.kills_total = 0; G.tally = 0; G.tally_t = 0.0; G.tally_done = 0;
    memset(G.tally_shown, 0, sizeof G.tally_shown);
    place_items(G.cx, G.cy);
    place_exit(G.cx, G.cy);
    G.level_treasure = 0; G.level_souls = 0; G.dead = 0; G.dead_t = 0.0;
    G.hurt = G.gold = G.kick = G.chill = 0.0f; G.pvx = G.pvy = 0.0f;
    G.px = G.cx + 0.5f; G.py = G.cy + 0.5f;
    G.h = rnd(4);
    for (int k = 0; k < 4 && !is_open(G.cx + DX[G.h], G.cy + DY[G.h]); k++) G.h = (G.h + 1) & 3;
    G.ang = heading_angle(G.h);
    G.map[G.cy][G.cx].visits = 1; G.explored = 1;
    G.spd = 0.0f; G.bobph = 0.0f; G.camz = 0.5f; G.hand = 0.0f; G.cooldown = 2.0f; G.calm = 0.0f;
    start_move();
    place_wall_skulls(G.cx, G.cy);
    place_spiders(G.cx, G.cy);
    place_guardians();
    int x, y;
    for (int i = 0; i < 4 + (G.level >= 4); i++) if (pick_cell_at_depth(G.cx, G.cy, 7, 40, 1, &x, &y)) spawn_mob(x, y, random_type());
#ifdef CRAWL_DEBUG
    { int nt[6] = { 0 }; for (int i = 0; i < MAXMOBS; i++) if (G.mobs[i].alive) nt[G.mobs[i].type]++;
      fprintf(stderr, "LEVEL %d mobs: skel %d ghost %d bat %d wskull %d shield %d spider %d\n", G.level, nt[0], nt[1], nt[2], nt[3], nt[4], nt[5]); }
#endif
    G.level_t = 0.0; G.card = 0.0f; G.ending = 0;
    G.readout = READOUT_T + 1.5f;                                  /* show where things stand as the level opens */
    if (G.player) human_begin();
}

#ifdef CRAWL_DEBUG
/* for testing: CRAWL_GEAR=n gives every spell, rune, robe and ring at tier n; CRAWL_GOLD, CRAWL_SOULS */
static void debug_gear(void) {
    if (getenv("CRAWL_GEAR")) {
        int t = atoi(getenv("CRAWL_GEAR"));
        for (int i = 0; i < NATK; i++) RUN.gear.atk[i] = (int8_t)t;
        for (int i = 0; i < NDEF; i++) RUN.gear.def[i] = (int8_t)t;
        for (int i = 1; i < NROBE; i++) RUN.gear.robe_own[i] = (int8_t)t;   /* the whole wardrobe */
        for (int i = 1; i < NHOOD; i++) RUN.gear.hood_own[i] = (int8_t)t;
        for (int i = 0; i < NRING; i++) RUN.gear.ring_own[i] = (int8_t)t;
        RUN.gear.robe = RB_SHADOW; RUN.gear.hood = HD_HUNTER;
        for (int f = 0; f < NFINGERS; f++) RUN.gear.ring[f] = (int8_t)f;
    }
    if (getenv("CRAWL_ROBE")) RUN.gear.robe = (int8_t)atoi(getenv("CRAWL_ROBE"));
    gear_sync(&RUN.gear);
    if (getenv("CRAWL_GOLD")) RUN.treasure = atoi(getenv("CRAWL_GOLD"));
    if (getenv("CRAWL_SOULS")) RUN.souls = atoi(getenv("CRAWL_SOULS"));
}
#endif

/* The run carries on across scene switches: the level, HP, gold, souls and gear are kept, and the
 * current level is laid out afresh (without a second visit to the merchant). */
static void crawl_enter(uint32_t seed) {
    crawl_art_init();
    int resume = RUN.active;
    if (resume && !RUN.loaded) {                                   /* back from the other scene: where was it? */
        int won = G.tally || G.exiting;
        RUN.level = G.level + won; RUN.hp = G.hp > 0 ? G.hp : gear_max_hp(&RUN.gear);
        RUN.shop_next = won || (G.shop && !G.shop_bought);
    }
    RUN.loaded = 0;
    memset(&G, 0, sizeof G);
    G.rs = seed | 1u;
    if (!resume) {
        memset(&RUN, 0, sizeof RUN);
        RUN.active = 1; RUN.level = 1; RUN.hp = 100; RUN.shop_next = 1;
        gear_reset(&RUN.gear);
#ifdef CRAWL_DEBUG
        if (getenv("CRAWL_LEVEL")) RUN.level = atoi(getenv("CRAWL_LEVEL"));
        debug_gear();
#endif
    }
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_SEED")) G.rs = (uint32_t)atoi(getenv("CRAWL_SEED")) | 1u;
#endif
    G.level = RUN.level; G.hp = RUN.hp;
    if (RUN.shop_next) begin_shop(); else start_level();
}

static void collect(Item *it) {
    it->alive = 0;
    switch (it->type) {
    case IT_KEY:
        G.has_key[it->color] = 1; snd_sfx(SFX_KEYGET); G.gold = 1.0f; G.grin_t = 1.6f;
#ifdef CRAWL_DEBUG
        bfs_from(G.doors[G.vault_door[it->color]].x, G.doors[G.vault_door[it->color]].y);
        fprintf(stderr, "EVENT key%d t=%.2f path=%d\n", it->color, G.t, bfs_dist[G.cy][G.cx]);
#endif
        break;
    case IT_POTION: G.hp = G.hp + 25 > G.max_hp ? G.max_hp : G.hp + 25; snd_sfx(SFX_POTION); break;
    case IT_URN: {                                                 /* it shatters and the souls inside are yours */
        int n = 5 + rnd(6);
        RUN.souls += n; G.level_souls += n;
        snd_sfx_ex(SFX_SHATTER, 0.8f, 0.5f); snd_sfx_ex(SFX_SHIMMER, 0.6f, 0.5f); G.chill = 0.5f;
        break;
    }
    case IT_RING: {                                                /* a ring of its own, for nothing */
        Offer o = { GK_RING, rnd(NRING), G.level, 0, 0 };
        gear_take(&RUN.gear, &o);
        G.max_hp = gear_max_hp(&RUN.gear);
        snd_sfx(SFX_KEYGET); G.gold = 1.0f; G.grin_t = 1.6f;
        break;
    }
    default: {
        int v = treasure_value(it->type);
        RUN.treasure += v; G.level_treasure += v; G.value_got += v; G.items_got++;
        snd_sfx_ex(SFX_COIN, it->type >= IT_CROWN ? 1.2f : 0.9f, 0.5f);
        G.gold = it->type >= IT_CROWN ? 1.0f : 0.6f;
        if (it->type >= IT_CROWN) G.grin_t = 1.6f;
        break;
    }
    }
    pop_readout();
}

static void update_items(void) {
    int seek = gear_seek(&RUN.gear);
    for (int i = 0; i < G.nitems; i++) {
        Item *it = &G.items[i];
        if (!it->alive) continue;
        float d = hypotf(it->x - G.px, it->y - G.py);
        if (!it->seen && ((d < (it->type == IT_KEY ? 13.0f : 8.5f) && line_clear(G.px, G.py, it->x, it->y)) || d < seek)) it->seen = 1;   /* the Seeker's Ring feels it through stone */
        if (d < 0.5f && G.level_t > 1.0 && !G.ending) collect(it);
    }
}

/* the tally counts up row by row: time, kills, treasure, souls */
#define TALLY_ROWS 4
static const float TALLY_ROW0 = 1.6f, TALLY_ROW_GAP = 1.0f, TALLY_COUNT = 0.75f, TALLY_END = 9.0f;
static int tally_value(int row, float t, int final) {
    float k = smoothstepf(0.0f, 1.0f, (t - TALLY_ROW0 - row * TALLY_ROW_GAP) / TALLY_COUNT);
    return (int)floorf(final * k);
}
static void update_tally(double dt) {
    float pt = (float)G.tally_t;
    G.tally_t += dt;
    float t = (float)G.tally_t;
    int finals[TALLY_ROWS] = { (int)G.clear_time, G.kills, G.items_got, G.level_souls };
    for (int r = 0; r < TALLY_ROWS; r++) {
        int v = tally_value(r, t, finals[r]);
        if (v != G.tally_shown[r]) { G.tally_shown[r] = v; snd_sfx_ex(SFX_TICK, 0.7f, 0.5f); }
        float done = TALLY_ROW0 + r * TALLY_ROW_GAP + TALLY_COUNT;
        if (pt < done && t >= done) snd_sfx_ex(r == 2 ? SFX_COIN : SFX_ENTER, 1.0f, 0.5f);
    }
    float perfect = TALLY_ROW0 + TALLY_ROWS * TALLY_ROW_GAP + 0.2f;
    if (pt < perfect && t >= perfect && G.items_total > 0 && G.items_got == G.items_total) snd_sfx_ex(SFX_KEYGET, 1.0f, 0.5f);
    if (t >= TALLY_END) { G.level++; RUN.level = G.level; RUN.hp = G.hp; begin_shop(); }
}

static void update_shop(double dt);

static void update_ui(void);
static void open_ui(int which);
static void update_human(float dt);
static void crawl_update(double dt) {
    float fdt = (float)dt;
    G.msg_t = fmaxf(0.0f, G.msg_t - fdt); G.hint_t = fmaxf(0.0f, G.hint_t - fdt);
    if (G.player && G.ui && !G.shop) { update_ui(); return; }     /* a menu is up: the crypt holds its breath */
    if (G.player && (G.tally || G.dead) && IN.hit[KEY_ESC]) { open_ui(UI_QUIT); return; }
    G.t += dt; G.level_t += dt; G.card += fdt;
    if (G.shop) { update_shop(dt); return; }
    if (G.tally) {                                                 /* the level-complete screen; the crypt waits */
        update_tally(dt);
        if (G.tally) G.fade = smoothstepf(0.0f, 0.4f, (float)G.tally_t) * (1.0f - smoothstepf(TALLY_END - 0.7f, TALLY_END, (float)G.tally_t));
        return;
    }
    float ox = G.px, oy = G.py;
    if (G.level_t > 1.0 && !G.dead) { if (G.player && !G.exiting) update_human(fdt); else update_player(fdt); }
    if (fdt > 0.0f) { G.pvx = (G.px - ox) / fdt; G.pvy = (G.py - oy) / fdt; }
    for (int i = 0; i < MAXMOBS; i++) if (G.mobs[i].alive) update_mob(&G.mobs[i], fdt);
    update_shots(fdt);
    update_bones(fdt);
    update_doors(fdt);
    if (!G.dead) { update_items(); update_status(fdt); if (G.level_t > 1.0) update_defense(fdt); }
    if (G.bfs_t > 0.0f && (G.bfs_t += fdt) > 1.6f) G.bfs_t = 0.0f;
    G.hurt = fmaxf(0.0f, G.hurt - fdt / 0.35f);
    G.gold = fmaxf(0.0f, G.gold - fdt / 0.45f);
    G.chill = fmaxf(0.0f, G.chill - fdt / 0.6f);
    G.face_t = fmaxf(0.0f, G.face_t - fdt);
    G.grin_t = fmaxf(0.0f, G.grin_t - fdt);
    G.kick *= expf(-fdt * 9.0f);
    if (G.readout > 0.0f) G.readout -= fdt;
    if (G.hp <= G.max_hp / 4 && !G.dead && (G.heartbeat -= fdt) <= 0.0f) { snd_sfx_ex(SFX_HEART, 0.9f, 0.5f); G.heartbeat = 1.1f; }

    /* keep the action coming: if nothing has happened for a while, wake something up ahead */
    if (G.calm > 11.0f) {
        int x, y, alive = 0;
        for (int i = 0; i < MAXMOBS; i++) alive += G.mobs[i].alive && !G.mobs[i].guard && G.mobs[i].type != MOB_WSKULL && G.mobs[i].type != MOB_SPIDER;
        if (alive < 8 && pick_cell_at_depth(G.cx, G.cy, 4, 8, 1, &x, &y)) spawn_mob(x, y, random_type());
        G.calm = 4.0f;
    }

    G.mood_t -= fdt;
    if (G.mood_t <= 0.0f) {                                        /* torch crackle follows nearby torches */
        float m = 0.0f;
        for (int l = 0; l < G.nlights; l++) {
            float d = hypotf(G.lights[l].x - G.px, G.lights[l].y - G.py);
            if (d < 5.0f) m += 1.2f / (1.0f + d * d);
        }
        snd_set_mood(clampf(m, 0.0f, 1.0f));
        G.mood_t = 0.15f;
    }

#ifdef CRAWL_DEBUG
    {
        static double next;
        if (G.t >= next) {
            next = G.t + 1.0;
            int alive = 0;
            for (int i = 0; i < MAXMOBS; i++) alive += G.mobs[i].alive;
            fprintf(stderr, "lvl=%d t=%5.1f state=%d cell=(%d,%d)->(%d,%d) h=%d next=%d spd=%.2f mobs=%d explored=%d/%d hp=%d gold=%d items=%d/%d key=%d vault=%d dead=%d souls=%d val=%d/%d\n",
                    G.level, G.t, G.state, G.cx, G.cy, G.tx, G.ty, G.h, G.next_h, G.spd, alive, G.explored, G.ncells,
                    G.hp, RUN.treasure, G.items_got, G.items_total, G.has_key[0] | G.has_key[1] << 1 | G.has_key[2] << 2,
                    G.nvaults ? G.doors[G.vault_door[0]].locked : -1, G.dead, RUN.souls, G.value_got, G.value_total);
            fprintf(stderr, "   throws=%d bonehits=%d bathits=%d firehits=%d swordhits=%d bites=%d dodges=%d exit=(%d,%d) seen=%d leaving=%d kills=%d/%d\n",
                    throws, hits_bone, hits_bat, hits_fire, hits_sword, bites, dodges, G.exit_fx, G.exit_fy, G.exit_seen, G.leaving, G.kills, G.kills_total);
        }
    }
#endif
    /* Explore first and gather treasure: head for the exit (via its key and vault if need be) once
     * most of the treasure is in hand and most of the level seen, or after four and a half minutes,
     * or at five and a half whatever happens. Not while a key is in hand for a vault that doesn't
     * hold the exit, or while a freshly opened vault is still full. */
    if (!G.exit_seen && G.exit_fx >= 0 && hypotf(G.exit_fx + 0.5f - G.px, G.exit_fy + 0.5f - G.py) < 7.0f
        && line_clear(G.px, G.py, G.exit_fx + 0.5f, G.exit_fy + 0.5f)) {
        G.exit_seen = 1;
        snd_sfx_ex(SFX_SHIMMER, 0.35f, 0.5f);                       /* a faint call from the runes */
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT exit_seen t=%.2f\n", G.t);
#endif
    }
    int pending = 0;
    for (int k = 0; k < G.nvaults; k++) {
        const Door *d = &G.doors[G.vault_door[k]];
        if (k == G.exit_in_vault) continue;
        if (G.has_key[k] && d->locked == 1) pending = 1;
        if (d->locked == 2 && G.level_t - G.unlock_t < 30.0) {
            for (int i = 0; i < G.nitems; i++)
                if (G.items[i].alive && G.items[i].type != IT_KEY && vault_of_room(G.items[i].cx, G.items[i].cy) == k) pending = 1;
        }
    }
    float got = G.value_total ? (float)G.value_got / (float)G.value_total : 1.0f;
    float want = RUN.goal > RUN.treasure ? 0.9f : 0.75f;            /* saving up: it wants nearly all of it */
    if (!G.leaving && !G.dead && G.exit_fx >= 0 && !G.player
        && ((G.level_t > 270.0 && !pending) || G.level_t > 330.0 || (got >= want && G.explored > G.ncells * 0.7f && G.level_t > 150.0 && !pending))) {
        G.leaving = 1;
#ifdef CRAWL_DEBUG
        fprintf(stderr, "EVENT leaving t=%.2f in_vault=%d got=%.2f\n", G.t, G.exit_in_vault, got);
#endif
    }
    if (!G.tally && !G.dead && !G.player && (G.exit_fx < 0 || G.level_t > 600.0)) {   /* no exit, or hopelessly lost: call it done */
        G.tally = 1; G.tally_t = 0.0; G.clear_time = G.level_t; snd_sfx_ex(SFX_FANFARE, 1.0f, 0.5f);
    }
    float fout = 1.0f;
    if (G.dead) {                                                  /* down to the floor, a red haze, then the level again */
        G.dead_t += dt;
        G.camz = lerpf(0.5f, 0.1f, smoothstepf(0.0f, 0.9f, (float)G.dead_t));
        fout = 1.0f - smoothstepf(3.8f, 4.8f, (float)G.dead_t);
        if (G.dead_t >= 5.0) { RUN.treasure -= G.level_treasure; G.hp = gear_max_hp(&RUN.gear); start_level(); }
    }
    float fin = smoothstepf(0.0f, 1.2f, (float)G.level_t);
    if (G.exiting) fout = 1.0f - smoothstepf(1.6f, 2.4f, G.exit_t);  /* into the light */
    G.fade = fin * fout;
    build_lightmap();
}

/* ================================================================== */
/* The merchant's antechamber, before each level: the owner's cat on a chest, three wares on
 * pedestals (an attack spell, a defensive rune, something to wear), one to be had for the gold in
 * hand. The first is free. Then out through the east door and into the level. */
/* ================================================================== */

#define CAT_X 12.62f
#define CAT_Y 15.45f
#define CAT_S 1.05f
static const float PED_X[NPOOL] = { 13.75f, 15.5f, 17.25f }, PED_Y[NPOOL] = { 13.75f, 13.55f, 13.75f };
enum { SH_GREET, SH_LOOK, SH_THINK, SH_GO, SH_BUY, SH_THANK, SH_LEAVE };
static const int ITEM_SPR[] = { SP_ITEM_TOME, SP_ITEM_RUNE, SP_ITEM_ROBE, SP_ITEM_HOOD, SP_ITEM_RING };

static void gen_shop(void) {
    memset(G.map, 0, sizeof G.map);
    for (int y = 0; y < MAPN; y++)
        for (int x = 0; x < MAPN; x++) {
            Cell *c = &G.map[y][x];
            c->kind = K_WALL; c->region = -1; c->door = -1;
            c->tex = (uint8_t)(hash2(x, y) % 10 < 8 ? T_BRICK : T_BANNER);
        }
    G.nrooms = 0; G.ndoors = 0; G.nlights = 0; G.nlc = 0; G.ndecos = 0; G.nitems = 0;
    G.items_total = G.items_got = G.value_total = G.value_got = 0; G.nvaults = 0;
    for (int k = 0; k < NKEYS; k++) G.vault_room[k] = G.vault_door[k] = -1;
    memset(G.bulky, 0, sizeof G.bulky); memset(G.fog, 0, sizeof G.fog);
    for (int y = 13; y <= 17; y++) for (int x = 12; x <= 18; x++) carve(x, y, 0, TH_BRICK);
    G.rooms[G.nrooms++] = (Room){ 12, 13, 7, 5, 0, TH_BRICK };
    for (int x = 19; x <= 24; x++) carve(x, 15, 1, TH_CORRIDOR);
    G.map[15][19].kind = K_DOOR; G.map[15][19].door = 0;
    G.doors[0] = (Door){ 19, 15, 1, 0.0f, 0.0f, 0, 0, 0 }; G.ndoors = 1;
    G.map[12][15].tex = T_SIGN;                                    /* the merchant's sign behind the wares */
    G.map[12][13].tex = G.map[12][17].tex = T_TORCH0;
    light_static(13.5f, 12.92f, 1.35f, 4.6f); light_static(17.5f, 12.92f, 1.35f, 4.6f);
    G.map[17][11].tex = T_TORCH0; light_static(12.08f, 17.5f, 1.2f, 4.2f);   /* by the cat */
    G.map[12][14].tex = G.map[12][16].tex = T_BRICK;
    add_deco(12.4f, 13.4f, SP_CANDLES); light_static(12.45f, 13.45f, 0.9f, 3.0f);
    add_deco(18.55f, 13.45f, SP_URN); add_deco(18.55f, 17.55f, SP_URN);
    for (int i = 0; i < NPOOL; i++) add_deco(PED_X[i], PED_Y[i], SP_PEDESTAL);
    G.ncells = 0;
    for (int y = 0; y < MAPN; y++) for (int x = 0; x < MAPN; x++) if (G.map[y][x].kind == K_FLOOR) G.ncells++;
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
    float v[NPOOL];
    int best = -1, dear = -1;
    float bs = 0.0f;
    for (int i = 0; i < NPOOL; i++) {
        v[i] = gear_value(g, &G.offers[i], G.level);
        if (v[i] <= 0.0f || G.offers[i].price > RUN.treasure) continue;
        float s = v[i] / sqrtf(fmaxf(1.0f, (float)G.offers[i].price) / 100.0f + 1.0f) * (0.8f + 0.4f * rndf());
        if (s > bs) { bs = s; best = i; }
        if (dear < 0 || G.offers[i].price > G.offers[dear].price) dear = i;
    }
    G.shop_saving = 0;
    if (G.level == 1) return best;                                 /* the first is free: take the best */
    #define SPELL(o) ((o).kind == GK_ATK || (o).kind == GK_DEF)
    if (RUN.goal > 0) {                                            /* saving: a spell worth the wait now? */
        int sp = -1;
        for (int i = 0; i < NPOOL; i++)
            if (SPELL(G.offers[i]) && G.offers[i].price <= RUN.treasure && v[i] > 0.5f && (sp < 0 || v[i] > v[sp])) sp = i;
        if (sp >= 0) { RUN.goal = 0; return sp; }
        if (RUN.treasure < RUN.goal && rndf() < 0.7f) { G.shop_saving = 1; return -1; }
        RUN.goal = 0;
    }
    float next = gear_haul(G.level) * 0.6f;                        /* what the coming level should bring, cautiously */
    for (int i = 0; i < NPOOL; i++) {                              /* a spell it can't quite afford: put the gold by */
        if (!SPELL(G.offers[i]) || G.offers[i].price <= RUN.treasure || v[i] <= (best >= 0 ? v[best] : 0.3f)) continue;
        float p = G.offers[i].price <= RUN.treasure + next ? 0.4f : 0.15f;
        if (best >= 0 && SPELL(G.offers[best])) p *= 0.3f;         /* a spell in hand already */
        if (rndf() < p) { RUN.goal = G.offers[i].price; G.shop_saving = 1; return -1; }
    }
    #undef SPELL
    if (dear >= 0 && rndf() < 0.12f) return dear;                  /* splurge */
    return best;
}

static void begin_shop(void) {
    G.shop = 1; G.state = PS_SHOP;
    crawl_art_level(1);                                            /* the antechamber looks the same every time */
    gen_shop();
    memset(G.mobs, 0, sizeof G.mobs); memset(G.shots, 0, sizeof G.shots); memset(G.booms, 0, sizeof G.booms);
    memset(G.bones, 0, sizeof G.bones); memset(G.beams, 0, sizeof G.beams); memset(G.burns, 0, sizeof G.burns);
    reset_caster_state();
    if (G.hp <= 0) G.hp = G.max_hp;
    G.tally = 0; G.dead = 0; G.exiting = G.leaving = 0; G.readout = 0.0f; G.card = 99.0f;
    G.hurt = G.gold = G.kick = G.chill = 0.0f;
    G.px = 14.35f; G.py = 15.95f; G.cx = 14; G.cy = 15;            /* close by the merchant */
    G.ang = face_of(CAT_X, CAT_Y);
    G.camz = 0.5f; G.hand = 0.0f; G.spd = 0.0f; G.bob = 0.0f;
    for (int p = 0; p < NPOOL; p++) {
        G.offers[p] = gear_offer(p, G.level, &G.rs);
        if (G.level == 1) G.offers[p].price = 0;
        G.offer_gone[p] = 0;
    }
    G.shop_step = SH_GREET; G.shop_t = 0.0; G.shop_pick = -1; G.shop_bought = 0; G.shop_look = -1;
    G.shop_gold = RUN.treasure; G.shop_saving = 0; G.sparkle = 0.0f;
    G.shop_heal = G.hp + (G.max_hp - G.hp) * 4 / 10;               /* a rest by the purring cat */
    G.cat_blink = G.cat_slow = G.cat_ear = G.cat_trill = 0.0f; G.purr_t = 0.4f;
    G.cat_next_blink = 2.5f + 2.0f * rndf(); G.cat_next_ear = 3.0f + 3.0f * rndf(); G.cat_tail_k = 0.0f;
    G.sdur = 0.0f; G.smove = 0.0f;
    G.level_t = 0.0;
#ifdef CRAWL_DEBUG
    fprintf(stderr, "SHOP level %d gold %d souls %d:", G.level, RUN.treasure, RUN.souls);
    for (int p = 0; p < NPOOL; p++) { char nm[40]; gear_name(&G.offers[p], nm, sizeof nm); fprintf(stderr, " [%s $%d%s]", nm, G.offers[p].price, G.offers[p].rare ? " RARE" : ""); }
    fprintf(stderr, "\n");
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
    G.shop_pick = i; G.shop_bought = 1; G.offer_gone[i] = 1; G.sparkle = 0.8f;
    RUN.treasure -= o->price;
    gear_take(&RUN.gear, o);
    int old = G.max_hp;
    G.max_hp = gear_max_hp(&RUN.gear);
    if (G.max_hp > old) G.hp += G.max_hp - old;                    /* a new charm's strength comes with it */
    if (o->price > 0) snd_sfx_ex(SFX_COIN, 1.1f, 0.5f);
    snd_sfx_ex(SFX_SHIMMER, 0.8f, 0.5f);
    if (o->kind == GK_DEF) snd_sfx_ex(SFX_RUNE, 0.8f, 0.5f);
    G.grin_t = 2.5f;
    sfx_at(SFX_TRILL, CAT_X, CAT_Y, 1.3f); G.cat_trill = 0.5f; G.cat_slow = 1.6f; G.cat_tail_k = 1.0f;
#ifdef CRAWL_DEBUG
    char nm[40]; gear_name(o, nm, sizeof nm);
    fprintf(stderr, "EVENT bought %s for %d (left %d) t=%.2f\n", nm, o->price, RUN.treasure, G.t);
#endif
}

static void shop_human(float dt);
static void update_ui(void);

static void update_shop(double dt) {
    float fdt = (float)dt;
    G.shop_t += dt;
    float st = (float)G.shop_t;
    shop_cat(fdt);
    if (G.player) {
        if (G.ui) { update_ui(); return; }
        shop_human(fdt);
        G.sdur = 0.0f;
    }
    if (G.sdur > 0.0f) {                                           /* the current turn or walk */
        G.smove += 0.0f;
        float k = smoothstepf(0.0f, 1.0f, fminf(1.0f, st / G.sdur));
        G.ang = lerpf(G.sa0, G.sa1, k);
        float w = fminf(1.0f, st / G.sdur);
        if (G.sx0 != G.sx1 || G.sy0 != G.sy1) {
            G.px = lerpf(G.sx0, G.sx1, w); G.py = lerpf(G.sy0, G.sy1, w);
            G.bobph += fdt * 9.0f; G.bob = sinf(G.bobph) * 0.012f;
            if (G.sx1 != G.sx0) { G.ang = G.sa0 + wrap_pi(atan2f(G.sy1 - G.sy0, G.sx1 - G.sx0) - G.sa0) * fminf(1.0f, st / 0.3f); }
        }
        G.cx = (int)G.px; G.cy = (int)G.py;
    }
    G.camz = 0.5f + G.bob;
    if (G.shop_gold != RUN.treasure) {                             /* the purse counting down */
        int d = RUN.treasure - G.shop_gold, step = abs(d) / 12 + 1;
        G.shop_gold += d > 0 ? (step < d ? step : d) : -(step < -d ? step : -d);
    }
    G.sparkle = fmaxf(0.0f, G.sparkle - fdt);
    G.grin_t = fmaxf(0.0f, G.grin_t - fdt);
    if (G.hp < G.shop_heal && st > 0.5f && (G.regen_acc += fdt * 12.0f) >= 1.0f) { int h = (int)G.regen_acc; G.regen_acc -= h; G.hp = clampi(G.hp + h, 0, G.shop_heal); }
    switch (G.player ? -1 : G.shop_step) {
    case -1: break;                                                /* a person is choosing */
    case SH_GREET:                                                 /* the merchant first: a slow blink */
        if (st > 0.8f && st - fdt <= 0.8f) G.cat_slow = 1.4f;
        if (st >= 2.4f) { G.shop_step = SH_LOOK; G.shop_look = 0; G.shop_t = 0.0; shop_turn(face_of(PED_X[0], PED_Y[0]), 0.7f); }
        break;
    case SH_LOOK: {                                                /* each ware in turn */
        float hold = 0.7f + 1.0f + 0.5f * (float)(hash2(G.level, G.shop_look) % 100) / 100.0f;
        if (st >= hold) {
            if (++G.shop_look >= NPOOL) { G.shop_look = -1; G.shop_step = SH_THINK; G.shop_t = 0.0; shop_turn(face_of(15.5f, 13.0f), 0.5f); }
            else { G.shop_t = 0.0; shop_turn(face_of(PED_X[G.shop_look], PED_Y[G.shop_look]), 0.45f); }
        }
        break;
    }
    case SH_THINK:
        if (st >= 1.1f) {
            G.shop_pick = shop_choose();
            G.shop_t = 0.0;
            if (G.shop_pick >= 0) {
                int i = G.shop_pick;
                G.shop_look = i;
                shop_walk(PED_X[i] + (15.5f - PED_X[i]) * 0.25f, PED_Y[i] + 1.0f, 1.6f);
                G.shop_step = SH_GO;
            } else { G.shop_step = SH_THANK; shop_turn(face_of(CAT_X, CAT_Y), 0.7f); }
#ifdef CRAWL_DEBUG
            fprintf(stderr, "EVENT shop_pick %d saving=%d goal=%d t=%.2f\n", G.shop_pick, G.shop_saving, RUN.goal, G.t);
#endif
        }
        break;
    case SH_GO:
        if (st >= G.sdur + 0.15f) { G.shop_step = SH_BUY; G.shop_t = 0.0; shop_turn(face_of(PED_X[G.shop_pick], PED_Y[G.shop_pick]), 0.3f); }
        break;
    case SH_BUY: {                                                 /* reach out and take it */
        G.hand = st < 0.35f ? st / 0.35f : (st > 1.05f ? fmaxf(0.0f, 1.0f - (st - 1.05f) / 0.3f) : 1.0f);
        if (!G.shop_bought && st >= 0.5f) shop_purchase(G.shop_pick);
        if (st >= 1.5f) { G.hand = 0.0f; G.shop_step = SH_THANK; G.shop_t = 0.0; shop_turn(face_of(CAT_X, CAT_Y), 0.7f); }
        break;
    }
    case SH_THANK:                                                 /* a look back at the merchant */
        if (!G.shop_bought && st > 0.9f && st - fdt <= 0.9f) G.cat_slow = 1.4f;
        if (st >= 2.1f) { G.shop_step = SH_LEAVE; G.shop_t = 0.0; shop_walk(18.2f, 15.5f, 1.8f); }
        break;
    default: {                                                     /* out through the door */
        Door *d = &G.doors[0];
        if (hypotf(d->x + 0.5f - G.px, d->y + 0.5f - G.py) < 2.2f && d->open < 1.0f) d->opening = 1;
        if (st >= G.sdur && G.sx1 < 20.0f) { G.shop_t = 0.0; shop_walk(22.5f, 15.5f, 1.8f); }
        break;
    }
    }
    if (G.px > 20.4f) {                                            /* through the door and into the level */
        G.fade = 1.0f - smoothstepf(20.4f, 21.6f, G.px);
        if (G.px >= 21.6f) { start_level(); return; }
    }
    update_doors(fdt);
    if (G.px <= 20.4f) G.fade = smoothstepf(0.0f, 0.7f, (float)G.level_t);
    G.mood_t -= fdt;
    if (G.mood_t <= 0.0f) { snd_set_mood(0.6f); G.mood_t = 0.5f; }
    build_lightmap();
}

/* ================================================================== */
/* A person at the controls (hold H for three seconds)                 */
/* ================================================================== */

#define MOUSE_TURN 0.0015f                  /* radians per mouse count */
#define KEY_TURN 2.6f                       /* radians a second with the arrow keys */
#define WALK 3.0f                           /* cells a second */
#define BODY 0.24f                          /* the caster's girth, against walls */
static const char *const ATK_SHORT[NATK] = { "FIRE", "FROST", "CHAIN", "SPEAR", "DRAIN", "HELL", "WISPS", "SPARKS", "BFS" };
static const char *const KEY_COLOR[NKEYS] = { "GOLD", "SILVER", "CRIMSON" };

static void say(const char *fmt, ...) {
    char b[64];
    va_list ap;
    va_start(ap, fmt); vsnprintf(b, sizeof b, fmt, ap); va_end(ap);
    if (strcmp(b, G.msg) || G.msg_t < 1.2f) { snprintf(G.msg, sizeof G.msg, "%s", b); G.msg_t = 2.2f; }
}

static void fizzle(int sp) {                                      /* chain or drain with nothing to bite on */
    float ex = G.px, ey = G.py;
    for (float d = 0.3f; d < 4.0f; d += 0.1f) {
        float x = G.px + cosf(G.ang) * d, y = G.py + sinf(G.ang) * d;
        if (blocks_light((int)x, (int)y)) break;
        ex = x; ey = y;
    }
    add_beam(sp, G.px + cosf(G.ang) * 0.35f, G.py + sinf(G.ang) * 0.35f, 0.42f, ex, ey, 0.45f);
    snd_sfx_ex(SFX_ZAP, 0.5f, 0.5f);
}

static int open_door(Door *d) {
    if (d->locked == 1) {
        if (!G.has_key[d->color]) { say("THE %s DOOR IS LOCKED", KEY_COLOR[d->color]); return 0; }
        d->locked = 2; G.has_key[d->color] = 0; G.unlock_t = G.level_t; snd_sfx(SFX_UNLOCK); pop_readout();
    }
    if (!d->opening && d->open < 0.95f) { d->opening = 1; sfx_at(SFX_DOOR, d->x + 0.5f, d->y + 0.5f, 1.0f); }
    return 1;
}

static void human_exit(void) {                                    /* into the runes */
    G.exiting = 1; G.leaving = 1; G.exit_t = 0.0f; G.u = 1.0f; G.pc_t = -1.0f; G.hand = 0.0f;
    G.cx = G.tx = G.exit_fx; G.cy = G.ty = G.exit_fy;
    begin_turn(heading_angle(G.exit_dir)); G.adur = 0.4f;
    G.state = PS_EXIT; G.spd = 0.0f;
}

/* walking into something: a door opens (if it can), the exit takes you */
static void human_bump(int cx, int cy) {
    Cell *c = cell(cx, cy);
    if (G.shop) { if (c->kind == K_DOOR) open_door(&G.doors[c->door]); return; }
    if (c->kind == K_DOOR) open_door(&G.doors[c->door]);
    else if (G.exit_fx >= 0 && cx == G.exit_wx && cy == G.exit_wy && (int)G.px == G.exit_fx && (int)G.py == G.exit_fy) human_exit();
}

/* E or Space: the door or exit just ahead */
static int human_use(void) {
    int cx = (int)floorf(G.px + cosf(G.ang) * 0.8f), cy = (int)floorf(G.py + sinf(G.ang) * 0.8f);
    Cell *c = cell(cx, cy);
    if (c->kind == K_DOOR) { open_door(&G.doors[c->door]); return 1; }
    if (!G.shop && G.exit_fx >= 0 && cx == G.exit_wx && cy == G.exit_wy && hypotf(G.exit_fx + 0.5f - G.px, G.exit_fy + 0.5f - G.py) < 1.0f) { human_exit(); return 1; }
    return 0;
}

static int body_blocked(float x, float y, int *bx, int *by) {
    float xs[2] = { x - BODY, x + BODY }, ys[2] = { y - BODY, y + BODY };
    for (int i = 0; i < 2; i++)
        for (int j = 0; j < 2; j++) {
            int cx = (int)floorf(xs[i]), cy = (int)floorf(ys[j]);
            if (blocks_move(cx, cy)) { *bx = cx; *by = cy; return 1; }
        }
    return 0;
}

static void push_from(float ox, float oy, float r) {              /* out of something round in the way */
    float vx = G.px - ox, vy = G.py - oy, d = hypotf(vx, vy);
    int bx, by;
    if (d >= r || d < 1e-4f) return;
    float nx = G.px + vx / d * (r - d), ny = G.py + vy / d * (r - d);
    if (!body_blocked(nx, ny, &bx, &by)) { G.px = nx; G.py = ny; }
}

/* one step of walking, sliding along walls; returns how far it got */
static float human_move(float dx, float dy) {
    float ox = G.px, oy = G.py;
    int bx, by;
    if (dx != 0.0f) { if (!body_blocked(G.px + dx, G.py, &bx, &by)) G.px += dx; else human_bump(bx, by); }
    if (dy != 0.0f) { if (!body_blocked(G.px, G.py + dy, &bx, &by)) G.py += dy; else human_bump(bx, by); }
    for (int i = 0; i < MAXMOBS; i++) {
        const Mob *m = &G.mobs[i];
        if (solid_mob(m) && m->state == MS_WALK && m->type != MOB_BAT) push_from(m->x, m->y, 0.5f);
    }
    if (G.shop) {                                                  /* the pedestals and the merchant's chest */
        for (int i = 0; i < NPOOL; i++) push_from(PED_X[i], PED_Y[i], 0.5f);
        push_from(CAT_X, CAT_Y, 0.75f);
    }
    return hypotf(G.px - ox, G.py - oy);
}

/* what a cast would bite on: the nearest creature close to the middle of the view */
static int aim_target(void) {
    float dx = cosf(G.ang), dy = sinf(G.ang);
    int best = -1; float bs = 1e9f;
    for (int i = 0; i < MAXMOBS; i++) {
        const Mob *m = &G.mobs[i];
        if (!targetable(m)) continue;
        float vx = m->x - G.px, vy = m->y - G.py, depth = vx * dx + vy * dy, lat = fabsf(-vx * dy + vy * dx);
        if (depth < 0.3f || depth > 10.0f || lat > depth * 0.12f + 0.3f || !line_clear(G.px, G.py, m->x, m->y)) continue;
        float sc = depth + lat * 4.0f;
        if (sc < bs) { bs = sc; best = i; }
    }
    return best;
}

static void pick_spell(int sp) {
    if (sp < 0 || sp >= NATK) return;
    if (RUN.gear.atk[sp] < 0) { say("NOT IN YOUR SPELLBOOK"); return; }
    if (sp != G.spell) snd_sfx_ex(SFX_TICK, 0.8f, 0.5f);
    G.spell = sp;
    char nm[40]; gear_item_name(GK_ATK, sp, RUN.gear.atk[sp], nm, sizeof nm);
    say("%s", nm);
}

static void spell_keys(void) {
    for (int k = 0; k < 9; k++) if (IN.hit[KEY_1 + k]) pick_spell(k);
    if (IN.wheel) {                                                /* the wheel turns the pages */
        int dir = IN.wheel > 0 ? -1 : 1, sp = G.spell;
        for (int n = 0; n < NATK; n++) { sp = (sp + dir + NATK) % NATK; if (RUN.gear.atk[sp] >= 0) break; }
        pick_spell(sp);
    }
}

/* turning and walking, shared by the crypt and the merchant's room */
static void human_walk(float dt) {
    float run = IN.down[KEY_SHIFT] ? 1.5f : 1.0f;
    G.ang = wrap_pi(G.ang + IN.mdx * MOUSE_TURN + ((float)IN.down[KEY_RIGHT] - (float)IN.down[KEY_LEFT]) * KEY_TURN * run * dt);
    float fwd = (float)(IN.down[KEY_W] || IN.down[KEY_UP]) - (float)(IN.down[KEY_S] || IN.down[KEY_DOWN]);
    float side = (float)IN.down[KEY_D] - (float)IN.down[KEY_A];
    float len = sqrtf(fwd * fwd + side * side), moved = 0.0f;
    if (len > 0.0f) {
        float sp = WALK * run * gear_haste(&RUN.gear) * (G.slow_t > 0.0f ? 0.6f : 1.0f) * dt / len;
        float c = cosf(G.ang), s = sinf(G.ang);
        moved = human_move((c * fwd - s * side) * sp, (s * fwd + c * side) * sp);
    }
    G.spd = dt > 0.0f ? moved / dt : 0.0f;
    float ph0 = G.bobph;
    G.bobph += moved * TAU_F;
    if (floorf(ph0 / PI_F) != floorf(G.bobph / PI_F)) snd_sfx_ex(SFX_STEP, 1.0f, ((int)(G.bobph / PI_F) & 1) ? 0.44f : 0.56f);
    G.camz = 0.5f + 0.014f * clampf(G.spd / CRUISE, 0.0f, 1.0f) * sinf(G.bobph);
    int ncx = (int)G.px, ncy = (int)G.py;
    if (ncx != G.cx || ncy != G.cy) {
        G.cx = G.tx = ncx; G.cy = G.ty = ncy; G.u = 1.0f;
        if (!G.shop && G.map[ncy][ncx].visits < 250) { if (G.map[ncy][ncx].visits == 0) G.explored++; G.map[ncy][ncx].visits++; }
    }
}

static void open_ui(int which) {
    G.ui = which;
    if (which == UI_INVENTORY) { G.ui_sel = 0; G.inv0 = RUN.gear; snd_sfx_ex(SFX_KEY, 0.8f, 0.5f); }
    else { G.ui_sel = 1; G.quit_line++; snd_sfx_ex(SFX_KNOCK, 0.6f, 0.5f); }
}

/* the hand and the spell in it, the way a person casts: click (or Ctrl) and it goes where you look */
static void human_cast(float dt) {
    G.cooldown -= dt;
    int trig = IN.down[KEY_MOUSE1] || IN.down[KEY_CTRL];
    if (trig && G.pc_t < 0.0f && G.cooldown <= 0.0f && !G.dead) {
        if (G.spell == AT_BFS && RUN.souls < bfs_cost(RUN.gear.atk[AT_BFS])) {
            if (IN.hit[KEY_MOUSE1] || IN.hit[KEY_CTRL]) { say("THE BFS9000 NEEDS %d SOULS", bfs_cost(RUN.gear.atk[AT_BFS])); snd_sfx_ex(SFX_ZAP, 0.4f, 0.5f); }
        } else {
            float cyc = spell_cycle(&RUN.gear, G.spell);
            G.pc_t = 0.0f; G.pc_fired = 0; G.pc_len = clampf(cyc * 0.35f, 0.22f, 0.5f); G.cooldown = cyc;
        }
    }
    float rest = G.shop ? 0.45f : 0.62f, h = rest;                 /* lower in the merchant's room, to see the wares */
    if (G.pc_t >= 0.0f) {
        G.pc_t += dt;
        float L = G.pc_len;
        if (!G.pc_fired && G.pc_t >= 0.35f * L) { G.pc_fired = 1; cast_spell(G.spell, aim_target()); G.calm = 0.0f; }
        h = G.pc_t < 0.3f * L ? lerpf(rest, 1.0f, G.pc_t / (0.3f * L)) : (G.pc_t < 0.7f * L ? 1.0f : lerpf(1.0f, rest, (G.pc_t - 0.7f * L) / (0.3f * L)));
        if (G.pc_t >= L) G.pc_t = -1.0f;
    }
    if (G.grab_t > 0.0f) { G.grab_t -= dt; h = fmaxf(h, rest + (1.0f - rest) * sinf(PI_F * clampf(1.0f - G.grab_t, 0.0f, 1.0f))); }
    G.hand = h;
}

static void update_human(float dt) {
    G.calm += dt;
    spell_keys();
    if (IN.hit[KEY_TAB]) { open_ui(UI_INVENTORY); return; }
    if (IN.hit[KEY_ESC]) { open_ui(UI_QUIT); return; }
    human_walk(dt);
    if (IN.hit[KEY_E] || IN.hit[KEY_SPACE]) human_use();
    if (G.exiting) return;
    human_cast(dt);
}

/* in the merchant's room: walk up to a ware to see it, E, Space or a click to buy it */
static void shop_human(float dt) {
    spell_keys();
    if (IN.hit[KEY_TAB]) { open_ui(UI_INVENTORY); return; }
    if (IN.hit[KEY_ESC]) { open_ui(UI_QUIT); return; }
    human_walk(dt);
    G.shop_look = -1;
    float bd = 3.2f;
    for (int i = 0; i < NPOOL; i++) {
        if (G.offer_gone[i]) continue;
        float d = hypotf(PED_X[i] - G.px, PED_Y[i] - G.py);
        if (d < bd && fabsf(wrap_pi(atan2f(PED_Y[i] - G.py, PED_X[i] - G.px) - G.ang)) < 0.3f + 0.25f / d) { bd = d; G.shop_look = i; }
    }
    if (IN.hit[KEY_E] || IN.hit[KEY_SPACE] || IN.hit[KEY_MOUSE1]) {
        int i = G.shop_look;
        if (i >= 0) {
            if (G.shop_bought) say("ONE WARE A VISIT");
            else if (G.offers[i].price > RUN.treasure) { say("YOU CAN'T AFFORD THAT"); G.cat_ear = 0.32f; G.cat_ear_side = rnd(2); }
            else { shop_purchase(i); G.grab_t = 1.0f; }
        } else if (!human_use() && hypotf(G.px - CAT_X, G.py - CAT_Y) < 1.8f) { G.cat_slow = 1.4f; sfx_at(SFX_TRILL, CAT_X, CAT_Y, 0.9f); }   /* a pat */
    }
    human_cast(dt);
}

/* the person takes over, from wherever the autopilot was */
static void human_begin(void) {
    G.state = PS_PLAYER; G.pc_t = -1.0f; G.u = 1.0f; G.spd = 0.0f;
    G.cx = G.tx = (int)G.px; G.cy = G.ty = (int)G.py;
    if (RUN.gear.atk[G.spell] < 0) G.spell = AT_FIRE;
}

/* back to the autopilot: the merchant's routine picks up where it fits; in the crypt, settle into the grid */
static void human_end(void) {
    G.player = 0; G.ui = UI_NONE; G.status_ret = PLAY_AUTO; G.pc_t = -1.0f;
    if (G.shop) {
        if (G.shop_bought) { G.shop_step = SH_LEAVE; G.shop_t = 0.0; shop_walk(18.2f, 15.5f, 1.8f); }
        else { G.shop_step = SH_THINK; G.shop_t = 0.0; shop_turn(face_of(15.5f, 13.0f), 0.5f); }
        return;
    }
    if (G.exiting || G.tally || G.dead) return;
    G.cx = G.tx = (int)G.px; G.cy = G.ty = (int)G.py;
    G.sx0 = G.px; G.sy0 = G.py;
    int h = ((int)lroundf(wrap_pi(G.ang) / (PI_F * 0.5f)) + 4) & 3;
    G.h = G.next_h = h;
    G.ang0 = G.ang; G.ang1 = G.ang + wrap_pi(heading_angle(h) - G.ang);
    G.state = PS_SETTLE; G.st = 0.0f;
}

/* ---- the inventory (Tab): the wardrobe and the spellbook ---- */

enum { INV_ROBE, INV_HOOD, INV_F0, INV_SLOTS = INV_F0 + NFINGERS };
static const int FINGER_OF_ROW[NFINGERS] = { 2, 1, 0, 3 };         /* shown index, middle, ring, little */
static const char *const FINGER_NAME[NFINGERS] = { "INDEX", "MIDDLE", "RING", "LITTLE" };

static int inv_options(int slot, int *opt) {
    const Gear *g = &RUN.gear;
    int n = 0;
    if (slot == INV_ROBE) { opt[n++] = RB_CLOAK; for (int i = 1; i < NROBE; i++) if (g->robe_own[i] > 0) opt[n++] = i; }
    else if (slot == INV_HOOD) { opt[n++] = HD_CLOAK; for (int i = 1; i < NHOOD; i++) if (g->hood_own[i] > 0) opt[n++] = i; }
    else { opt[n++] = -1; for (int i = 0; i < NRING; i++) if (g->ring_own[i] > 0) opt[n++] = i; }
    return n;
}
static int inv_worn(int slot) {
    const Gear *g = &RUN.gear;
    return slot == INV_ROBE ? g->robe : (slot == INV_HOOD ? g->hood : g->ring[FINGER_OF_ROW[slot - INV_F0]]);
}

static void update_inventory(void) {
    spell_keys();
    if (IN.hit[KEY_TAB] || IN.hit[KEY_ESC] || IN.hit[KEY_ENTER]) { G.ui = UI_NONE; snd_sfx_ex(SFX_KEY, 0.6f, 0.5f); return; }
    if (IN.hit[KEY_W] || IN.hit[KEY_UP]) { G.ui_sel = (G.ui_sel + INV_SLOTS - 1) % INV_SLOTS; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
    if (IN.hit[KEY_S] || IN.hit[KEY_DOWN]) { G.ui_sel = (G.ui_sel + 1) % INV_SLOTS; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
    int dir = (IN.hit[KEY_D] || IN.hit[KEY_RIGHT]) - (IN.hit[KEY_A] || IN.hit[KEY_LEFT]);
    if (!dir) return;
    int opt[8], n = inv_options(G.ui_sel, opt), cur = inv_worn(G.ui_sel), at = 0;
    if (n < 2) { say("NOTHING ELSE TO WEAR THERE"); return; }
    for (int i = 0; i < n; i++) if (opt[i] == cur) at = i;
    int pick = opt[(at + dir + n) % n];
    if (G.ui_sel == INV_ROBE) gear_wear_robe(&RUN.gear, pick);
    else if (G.ui_sel == INV_HOOD) gear_wear_hood(&RUN.gear, pick);
    else gear_wear_ring(&RUN.gear, FINGER_OF_ROW[G.ui_sel - INV_F0], pick);
    G.max_hp = gear_max_hp(&RUN.gear);
    if (G.hp > G.max_hp) G.hp = G.max_hp;
    snd_sfx_ex(SFX_STEP, 1.2f, 0.5f);
}

/* ---- Esc: leave the crypt? ---- */

enum { Q_FLEE, Q_STAY, Q_SLEEP, Q_CARVE, Q_RISE, Q_OPTIONS };
static void open_graves(int mode);
static void quit_choose(int c) {
    if (c == Q_CARVE || c == Q_RISE) { open_graves(c == Q_RISE); return; }
    if (c == Q_FLEE) { G.status_ret = PLAY_QUIT; snd_sfx_ex(SFX_GHOST_DIE, 0.8f, 0.5f); }
    else if (c == Q_STAY) { G.ui = UI_NONE; snd_sfx_ex(SFX_KEY, 0.6f, 0.5f); }
    else { G.ui = UI_NONE; human_end(); snd_sfx_ex(SFX_WHISPER, 0.7f, 0.5f); }
}
static void update_quit(void) {
    if (G.status_ret == PLAY_QUIT) return;
    if (IN.hit[KEY_W] || IN.hit[KEY_UP]) { G.ui_sel = (G.ui_sel + Q_OPTIONS - 1) % Q_OPTIONS; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
    if (IN.hit[KEY_S] || IN.hit[KEY_DOWN]) { G.ui_sel = (G.ui_sel + 1) % Q_OPTIONS; snd_sfx_ex(SFX_TICK, 0.6f, 0.5f); }
    if (IN.hit[KEY_ENTER] || IN.hit[KEY_SPACE] || IN.hit[KEY_E] || IN.hit[KEY_MOUSE1]) { quit_choose(G.ui_sel); return; }
    if (IN.hit[KEY_Y]) { quit_choose(Q_FLEE); return; }
    if (IN.hit[KEY_N] || IN.hit[KEY_ESC]) { quit_choose(Q_STAY); return; }
}

/* ---- the scene's side of the bargain with the app ---- */

static void crawl_input(const Input *in, double dt) { (void)dt; IN = *in; }

static void crawl_control(int what) {
    if (what == CTRL_PAUSE) { if (G.player && !G.ui && G.status_ret != PLAY_QUIT) open_ui(UI_QUIT); return; }
    G.player = 1; G.status_ret = PLAY_PLAYER; G.ui = UI_NONE;
    if (!G.quit_line) G.quit_line = 1 + rnd(97);                    /* the crypt's arguments, from somewhere new each time */
    if (G.shop) { G.sdur = 0.0f; G.shop_look = -1; }
    else if (!G.exiting && !G.tally && !G.dead) human_begin();
    G.hint_t = 6.0f;
    snd_sfx_ex(SFX_RUNE, 1.0f, 0.5f); snd_sfx_ex(SFX_WHOOSH, 0.6f, 0.5f);
    say("THE CONTROLS ARE YOURS");
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_TPEXIT") && !G.shop && G.exit_fx >= 0) {     /* testing: straight to the exit, facing it */
        G.px = G.exit_fx + 0.5f; G.py = G.exit_fy + 0.5f; G.ang = heading_angle(G.exit_dir); human_begin();
    }
    fprintf(stderr, "EVENT control t=%.2f at (%.2f,%.2f) ang %.3f shop %d\n", G.t, G.px, G.py, G.ang, G.shop);
#endif
}

static int crawl_status(void) { return G.player ? G.status_ret : PLAY_AUTO; }

/* What a save keeps: the level to start from, health, gold, souls and the gear. Not where you
 * stood: you come back to the start of the level, so its gold and souls are taken back out. */
#define SAVE_MAGIC 0x31535648u              /* "HVS1" */
static void put32(uint8_t *p, int32_t v) { for (int i = 0; i < 4; i++) p[i] = (uint8_t)((uint32_t)v >> (8 * i)); }
static int32_t get32(const uint8_t *p) { uint32_t v = 0; for (int i = 0; i < 4; i++) v |= (uint32_t)p[i] << (8 * i); return (int32_t)v; }

static int crawl_save(uint8_t *buf, int cap) {
    int n = 4 + 6 * 4 + (int)sizeof(Gear);
    if (cap < n) return 0;
    int level = G.level, hp = G.dead || G.hp <= 0 ? gear_max_hp(&RUN.gear) : G.hp, gold = RUN.treasure, souls = RUN.souls, shop = 0;
    if (G.tally || G.exiting) { level = G.level + 1; shop = 1; }   /* the level is won */
    else if (G.shop) shop = !G.shop_bought;
    else { gold -= G.level_treasure; souls -= G.level_souls; }
    put32(buf, (int32_t)SAVE_MAGIC);
    put32(buf + 4, 1);                                             /* version */
    put32(buf + 8, level); put32(buf + 12, hp); put32(buf + 16, gold); put32(buf + 20, souls); put32(buf + 24, shop);
    memcpy(buf + 28, &RUN.gear, sizeof(Gear));
    return n;
}

static void crawl_load(const uint8_t *buf, int len) {
    if (len < 28 + (int)sizeof(Gear) || (uint32_t)get32(buf) != SAVE_MAGIC || get32(buf + 4) != 1) return;
    memset(&RUN, 0, sizeof RUN);
    RUN.active = 1; RUN.loaded = 1;
    RUN.level = clampi(get32(buf + 8), 1, 999); RUN.treasure = get32(buf + 16); RUN.souls = get32(buf + 20);
    RUN.shop_next = get32(buf + 24) != 0;
    memcpy(&RUN.gear, buf + 28, sizeof(Gear));
    gear_sync(&RUN.gear);
    RUN.hp = clampi(get32(buf + 12), 1, gear_max_hp(&RUN.gear));
    if (RUN.treasure < 0) RUN.treasure = 0;
    if (RUN.souls < 0) RUN.souls = 0;
}

/* ---- the graves: three save slots, carved and risen from the Esc menu ---- */

/* A grave holds what fleeing to the surface keeps (the level to start from, health, gold, souls and
 * gear), then when it was carved. */
typedef struct { int used, level, hp, gold, souls; int64_t when; } Grave;
static const char *const ROMAN[SLOT_COUNT] = { "I", "II", "III" };

static Grave grave_info(int i) {
    Grave g;
    memset(&g, 0, sizeof g);
    uint8_t buf[SLOT_MAX];
    int n = slot_read(i, buf, sizeof buf), base = 28 + (int)sizeof(Gear);
    if (n < base || (uint32_t)get32(buf) != SAVE_MAGIC || get32(buf + 4) != 1) return g;
    g.used = 1; g.level = get32(buf + 8); g.hp = get32(buf + 12); g.gold = get32(buf + 16); g.souls = get32(buf + 20);
    if (n >= base + 8) g.when = (int64_t)((uint64_t)(uint32_t)get32(buf + base) | (uint64_t)(uint32_t)get32(buf + base + 4) << 32);
    return g;
}

static void open_graves(int mode) {
    G.ui = UI_GRAVES; G.graves_mode = mode; G.graves_arm = -1; G.graves_msg = 0; G.ui_sel = 0;
    int64_t newest = -1;
    for (int i = SLOT_COUNT - 1; i >= 0; i--) {
        Grave g = grave_info(i);
        if (!mode && !g.used) G.ui_sel = i;                        /* carving: the first empty grave */
        if (mode && g.used && g.when >= newest) { newest = g.when; G.ui_sel = i; }   /* rising: the newest */
    }
    snd_sfx_ex(SFX_KEY, 0.7f, 0.5f);
}

static void carve_grave(int i) {
    uint8_t buf[SLOT_MAX];
    int n = crawl_save(buf, SLOT_MAX - 8);
    if (n <= 0) return;
    uint64_t now = (uint64_t)(int64_t)time(NULL);
    put32(buf + n, (int32_t)(uint32_t)(now & 0xffffffffu)); put32(buf + n + 4, (int32_t)(uint32_t)(now >> 32));
    slot_write(i, buf, n + 8);
    G.ui = UI_NONE;
    say("CARVED INTO GRAVE %s", ROMAN[i]);
    snd_sfx_ex(SFX_KNOCK, 1.0f, 0.5f); snd_sfx_ex(SFX_RUNE, 0.6f, 0.5f);
}

/* the grave's run comes back at the start of its level, still in your hands */
static void rise_from_grave(int i) {
    uint8_t buf[SLOT_MAX];
    int n = slot_read(i, buf, sizeof buf), ql = G.quit_line;
    uint32_t seed = G.rs * 1664525u + 1013904223u;
    crawl_load(buf, n);
    crawl_enter(seed);
    G.player = 1; G.status_ret = PLAY_PLAYER; G.ui = UI_NONE; G.quit_line = ql;
    if (G.shop) { G.sdur = 0.0f; G.shop_look = -1; }
    else human_begin();
    say("RISEN FROM GRAVE %s: LEVEL %d", ROMAN[i], G.level);
    snd_sfx_ex(SFX_WHOOSH, 0.8f, 0.5f); snd_sfx_ex(SFX_WHISPER, 0.8f, 0.5f);
}

enum { GM_NONE, GM_ARMED, GM_EMPTY };
static void update_graves(void) {
    if (IN.hit[KEY_ESC] || IN.hit[KEY_TAB] || IN.hit[KEY_N]) {        /* back to the question */
        G.ui = UI_QUIT; G.ui_sel = G.graves_mode ? Q_RISE : Q_CARVE; snd_sfx_ex(SFX_KEY, 0.6f, 0.5f);
        return;
    }
    int mv = (IN.hit[KEY_S] || IN.hit[KEY_DOWN]) - (IN.hit[KEY_W] || IN.hit[KEY_UP]), pick = -1;
    for (int k = 0; k < SLOT_COUNT; k++) if (IN.hit[KEY_1 + k]) pick = k;
    if (mv || pick >= 0) {
        G.ui_sel = pick >= 0 ? pick : (G.ui_sel + mv + SLOT_COUNT) % SLOT_COUNT;
        G.graves_arm = -1; G.graves_msg = GM_NONE;
        snd_sfx_ex(SFX_TICK, 0.6f, 0.5f);
    }
    if (!(IN.hit[KEY_ENTER] || IN.hit[KEY_SPACE] || IN.hit[KEY_E] || IN.hit[KEY_MOUSE1])) return;
    int i = G.ui_sel, used = grave_info(i).used;
    if (G.graves_mode && !used) { G.graves_msg = GM_EMPTY; snd_sfx_ex(SFX_ZAP, 0.4f, 0.5f); return; }
    if ((used || G.graves_mode) && G.graves_arm != i) {               /* over a carved name, or losing this run: ask twice */
        G.graves_arm = i; G.graves_msg = GM_ARMED; snd_sfx_ex(SFX_KNOCK, 0.5f, 0.5f);
        return;
    }
    if (G.graves_mode) rise_from_grave(i); else carve_grave(i);
}

static void update_ui(void) {
    if (G.ui == UI_INVENTORY) update_inventory();
    else if (G.ui == UI_GRAVES) update_graves();
    else update_quit();
}

/* ================================================================== */
/* ================================================================== */
/* Rendering                                                           */
/* ================================================================== */

static inline uint8_t shade(uint8_t c, float light, int x, int y) {
    if (IS_FULLBRIGHT(c)) return c;
    int lv = (int)(light * LIGHT_UNIT + BAYER[y & 3][x & 3] * (1.0f / 16.0f));
    return crawl_cmap[clampi(lv, 0, NLIGHT - 1)][c];
}

/* Fog, done the VGA way: dithered pixels of grey haze, mixed from the bank at the spot being drawn
 * and the one the caster stands in, growing with distance, lit by whatever lights the spot. */
static float g_camfog;
static inline float fog_at(float x, float y) {
    float fx = x - 0.5f, fy = y - 0.5f;
    int ix = (int)floorf(fx), iy = (int)floorf(fy);
    if (ix < 0 || iy < 0 || ix >= MAPN - 1 || iy >= MAPN - 1) return 0.0f;
    float ax = fx - ix, ay = fy - iy;
    return lerpf(lerpf(G.fog[iy][ix], G.fog[iy][ix + 1], ax), lerpf(G.fog[iy + 1][ix], G.fog[iy + 1][ix + 1], ax), ay);
}
static inline float fog_amount(float x, float y, float depth_k) {   /* depth_k: 1 - exp(-distance * 0.28) */
    return fminf((fog_at(x, y) * 0.6f + g_camfog * 0.4f) * depth_k, 0.7f);
}
static inline uint8_t fogged(uint8_t c, float fog, float light, int x, int y) {
    if (fog * 16.0f > BAYER[y & 3][x & 3] + 0.5f) return (uint8_t)(R_CEIL + clampi((int)(3.0f + light * 8.0f), 3, 12));
    return c;
}

static void render_floor(Frame *f, float dx, float dy, float plx, float ply, float horizon) {
    float lx = dx - plx, ly = dy - ply, rx = dx + plx, ry = dy + ply;
    for (int y = 0; y < H; y++) {
        uint8_t *row = &f->pix[y * W];
        float p = y - horizon + 0.5f;
        int floor_row = p > 0.0f;
        float dist = floor_row ? G.camz * FOCAL / p : (1.0f - G.camz) * FOCAL / -p;
        if (dist > 30.0f) { memset(row, 0, W); continue; }
        float sx = dist * (rx - lx) / W, sy = dist * (ry - ly) / W;
        float wx = G.px + dist * lx, wy = G.py + dist * ly;
        float a = atten(dist), dk = 1.0f - expf(-dist * 0.28f);
        int grout = floor_row && dist < 6.0f;
        for (int x = 0; x < W; x++, wx += sx, wy += sy) {
            uint8_t base;
            if (floor_row) {
                int cx = (int)floorf(wx), cy = (int)floorf(wy);
                float fx = wx - cx, fy = wy - cy;
                base = (uint8_t)(R_FLOOR + 8 + ((cx * 7 + cy * 13) & 1));
                if (grout && (fx < 0.035f || fy < 0.035f)) base = R_FLOOR + 5;
                if (G.nova_t > 0.0f) {                             /* the frost nova racing out over the floor */
                    float r = 2.2f * sqrtf(G.nova_t / 0.6f), d = hypotf(wx - G.px, wy - G.py);
                    if (d < r && (fabsf(d - r) < 0.22f || BAYER[y & 3][x & 3] < (int)(10.0f * (1.0f - G.nova_t / 0.6f)))) {
                        row[x] = (uint8_t)(R_GLOW + clampi((int)(15.0f - fabsf(d - r) * 30.0f), 5, 15));
                        continue;
                    }
                }
            } else base = R_CEIL + 8;
            float L = lm_at(wx, wy) * a;
            row[x] = fogged(shade(base, L, x, y), fog_amount(wx, wy, dk * (floor_row ? 1.15f : 0.8f)), L, x, y);
        }
    }
}

static void render_walls(Frame *f, float dx, float dy, float plx, float ply, float horizon) {
    int tframe = (int)(G.t * 8.0);
    for (int x = 0; x < W; x++) {
        float camx = 2.0f * (x + 0.5f) / W - 1.0f;
        float rdx = dx + plx * camx, rdy = dy + ply * camx;
        int mx = (int)G.px, my = (int)G.py;
        float ddx = rdx == 0.0f ? 1e30f : fabsf(1.0f / rdx), ddy = rdy == 0.0f ? 1e30f : fabsf(1.0f / rdy);
        int sx = rdx < 0 ? -1 : 1, sy = rdy < 0 ? -1 : 1;
        float sdx = rdx < 0 ? (G.px - mx) * ddx : (mx + 1.0f - G.px) * ddx;
        float sdy = rdy < 0 ? (G.py - my) * ddy : (my + 1.0f - G.py) * ddy;
        int side = 0, tex = T_STONE, texx = 0, hit = 0;
        float perp = 30.0f;
        for (int i = 0; i < 64 && !hit; i++) {
            if (sdx < sdy) { sdx += ddx; mx += sx; side = 0; } else { sdy += ddy; my += sy; side = 1; }
            Cell *c = cell(mx, my);
            if (c->kind == K_WALL) {
                perp = side == 0 ? sdx - ddx : sdy - ddy;
                tex = c->tex;
                int pxc = side == 0 ? mx - sx : mx, pyc = side == 1 ? my - sy : my;
                if (cell(pxc, pyc)->kind == K_DOOR) tex = T_JAMB;
                if (tex == T_TORCH0) tex = T_TORCH0 + (int)((tframe + hash2(mx, my)) % 3);
                if (tex == T_EXIT0) tex = G.exit_open > 0.3f ? T_EXIT_OPEN : T_EXIT0 + ((int)(G.t * 2.5) & 1);
                float wallx = side == 0 ? G.py + perp * rdy : G.px + perp * rdx;
                wallx -= floorf(wallx);
                texx = (int)(wallx * TS);
                if ((side == 0 && rdx > 0) || (side == 1 && rdy < 0)) texx = TS - 1 - texx;
                hit = 1;
            } else if (c->kind == K_DOOR) {
                Door *d = &G.doors[c->door];
                float t, u;
                if (d->along_x) {
                    if (rdx == 0.0f) continue;
                    t = (mx + 0.5f - G.px) / rdx;
                    u = G.py + t * rdy - my;
                } else {
                    if (rdy == 0.0f) continue;
                    t = (my + 0.5f - G.py) / rdy;
                    u = G.px + t * rdx - mx;
                }
                if (t <= 0.0f || u < 0.0f || u >= 1.0f || u < d->open) continue;
                static const int LOCKTEX[NKEYS] = { T_LOCKED, T_LOCKED_S, T_LOCKED_C };
                perp = t; tex = d->locked ? LOCKTEX[d->color] : T_DOOR; side = d->along_x ? 0 : 1;
                texx = clampi((int)((u - d->open) * TS), 0, TS - 1);
                hit = 1;
            }
        }
        if (perp < 0.05f) perp = 0.05f;
        zbuf[x] = perp;
        if (!hit) continue;
        float lineh = FOCAL / perp;
        float top = horizon - (1.0f - G.camz) * lineh, bot = horizon + G.camz * lineh;
        float hx = G.px + perp * rdx - (side == 0 ? sx * 0.13f : 0.0f);
        float hy = G.py + perp * rdy - (side == 1 ? sy * 0.13f : 0.0f);
        float light = lm_at(hx, hy) * atten(perp) * (side ? 0.82f : 1.0f);
        float fog = fog_amount(hx, hy, 1.0f - expf(-perp * 0.28f));
        const uint8_t *col = &crawl_tex[tex][texx * TS];
        int y0 = clampi((int)ceilf(top - 0.5f), 0, H), y1 = clampi((int)ceilf(bot - 0.5f), 0, H);
        float tstep = TS / lineh, ty = (y0 + 0.5f - top) * tstep;
        for (int y = y0; y < y1; y++, ty += tstep)
            f->pix[y * W + x] = fogged(shade(col[clampi((int)ty, 0, TS - 1)], light, x, y), fog, light, x, y);
    }
}

/* alpha < 1: dithered see-through. remap: the ramp R_PURPLE turns into. flags: how it is drawn */
typedef struct { float x, y, z, depth; int spr; float alpha, twinkle, scale; int remap, flags; } Draw;
enum { DR_EYES = 1, DR_IMPLODE = 2, DR_BRIGHT = 4, DR_ICE = 8, DR_CAT = 16 };

static int cmp_draw(const void *a, const void *b) {
    float da = ((const Draw *)a)->depth, db = ((const Draw *)b)->depth;
    return da < db ? 1 : (da > db ? -1 : 0);
}

/* world to screen; 0 if behind the eye */
static int project(float x, float y, float z, float dx, float dy, float horizon, float *sx, float *sy, float *depth) {
    float vx = x - G.px, vy = y - G.py, dp = vx * dx + vy * dy;
    if (dp < 0.15f) return 0;
    *sx = W / 2 + FOCAL * (vx * -dy + vy * dx) / dp;
    *sy = horizon - (z - G.camz) * FOCAL / dp;
    *depth = dp;
    return 1;
}

/* a line through the crypt, hidden by walls; colour from the ramp, lit by nothing (it glows) */
static void line3d(Frame *f, float dx, float dy, float horizon, float x0, float y0, float z0, float x1, float y1, float z1,
                   uint8_t core, uint8_t halo, int dither) {
    float ax, ay, ad, bx, by, bd;
    int a_ok = project(x0, y0, z0, dx, dy, horizon, &ax, &ay, &ad), b_ok = project(x1, y1, z1, dx, dy, horizon, &bx, &by, &bd);
    float len = a_ok && b_ok ? hypotf(bx - ax, by - ay) : 60.0f;
    int n = clampi((int)len + 2, 2, 400);
    for (int i = 0; i <= n; i++) {
        float t = (float)i / n, sx, sy, dep;
        if (!project(lerpf(x0, x1, t), lerpf(y0, y1, t), lerpf(z0, z1, t), dx, dy, horizon, &sx, &sy, &dep)) continue;
        int x = (int)sx, y = (int)sy;
        if (x < 0 || x >= W || y < 0 || y >= H || dep >= zbuf[x]) continue;
        if (dither && BAYER[y & 3][x & 3] < 8) continue;
        f->pix[y * W + x] = core;
        if (halo) {
            if (y > 0 && f->pix[(y - 1) * W + x] != core) f->pix[(y - 1) * W + x] = halo;
            if (y < H - 1 && f->pix[(y + 1) * W + x] != core) f->pix[(y + 1) * W + x] = halo;
        }
    }
}

/* lightning and the soul drain, a moment each */
static void render_beams(Frame *f, float dx, float dy, float horizon) {
    for (int i = 0; i < MAXBEAMS; i++) {
        const Beam *b = &G.beams[i];
        if (!b->alive) continue;
        float k = 1.0f - b->t / 0.3f;
        if (b->kind == AT_CHAIN) {                                 /* a jagged bolt, re-forked every frame */
            uint32_t r = (uint32_t)(G.t * 30.0) * 2654435761u + (uint32_t)i * 40503u;
            float px = b->x0, py = b->y0, pz = b->z0, ox = -(b->y1 - b->y0), oy = b->x1 - b->x0, ol = hypotf(ox, oy) + 1e-4f;
            ox /= ol; oy /= ol;
            int segs = 7;
            for (int s = 1; s <= segs; s++) {
                float t = (float)s / segs, j = s == segs ? 0.0f : (rng_f(&r) - 0.5f) * 0.45f, jz = s == segs ? 0.0f : (rng_f(&r) - 0.5f) * 0.3f;
                float nx = lerpf(b->x0, b->x1, t) + ox * j, ny = lerpf(b->y0, b->y1, t) + oy * j, nz = lerpf(b->z0, b->z1, t) + jz;
                line3d(f, dx, dy, horizon, px, py, pz, nx, ny, nz, k > 0.4f ? C_WHITE : (uint8_t)(R_GLOW + 12), (uint8_t)(R_GLOW + (k > 0.5f ? 11 : 7)), k < 0.25f);
                px = nx; py = ny; pz = nz;
            }
        } else {                                                   /* the drain: a red thread, wavering, flowing home */
            float ox = -(b->y1 - b->y0), oy = b->x1 - b->x0, ol = hypotf(ox, oy) + 1e-4f;
            ox /= ol; oy /= ol;
            float px = b->x1, py = b->y1, pz = b->z1;
            for (int s = 1; s <= 12; s++) {
                float t = (float)s / 12, w = sinf(t * 9.0f + b->t * 30.0f) * 0.12f * sinf(t * PI_F);
                float nx = lerpf(b->x1, b->x0, t) + ox * w, ny = lerpf(b->y1, b->y0, t) + oy * w, nz = lerpf(b->z1, b->z0, t) + w * 0.6f;
                line3d(f, dx, dy, horizon, px, py, pz, nx, ny, nz, (uint8_t)(R_BLOOD + 14), (uint8_t)(R_BLOOD + 8), k < 0.3f);
                px = nx; py = ny; pz = nz;
            }
        }
    }
}

/* The merchant, put together each frame: blinking, ears twitching, the tail swishing over the
 * side of the chest, and the eyes following the caster. */
static uint8_t cat_buf[CAT_H * CAT_W];
static void cat_disc(float cx, float cy, float r, int fill, float t) {
    for (int y = (int)(cy - r - 1); y <= (int)(cy + r + 1); y++)
        for (int x = (int)(cx - r - 1); x <= (int)(cx + r + 1); x++) {
            if (x < 0 || y < 0 || x >= CAT_W || y >= CAT_H) continue;
            float dx = x + 0.5f - cx, dy = y + 0.5f - cy;
            if (dx * dx + dy * dy > r * r) continue;
            uint8_t c;
            if (!fill) c = (uint8_t)(R_WARM + 1);
            else {
                int stripe = ((int)(t * 11.0f)) & 1, tip = t > 0.86f;
                float lit = 0.5f - dx / r * 0.35f;
                c = (uint8_t)(R_WARM + clampi((int)((tip ? 3 : (stripe ? 5 : 9)) + lit * 4.0f - 2.0f), 2, 13));
            }
            cat_buf[y * CAT_W + x] = c;
        }
}
static void build_cat(void) {
    memcpy(cat_buf, crawl_cat_img, sizeof cat_buf);
    /* the tail: out of the rump and down over the end of the chest, swishing (more after a sale) */
    float sw = (float)G.t * (1.3f + 2.2f * G.cat_tail_k), amp = 4.0f + 5.0f * G.cat_tail_k;
    float p0x = crawl_cat_tail[0], p0y = crawl_cat_tail[1], p1x = p0x + 12.0f, p1y = p0y + 1.0f;
    float p2x = p0x + 10.0f + amp * sinf(sw), p2y = p0y + 31.0f - fabsf(amp * sinf(sw)) * 0.4f;
    for (int pass = 0; pass < 2; pass++)
        for (int i = 0; i <= 48; i++) {
            float t = i / 48.0f, u = 1.0f - t;
            float bx = u * u * p0x + 2 * u * t * p1x + t * t * p2x, by = u * u * p0y + 2 * u * t * p1y + t * t * p2y;
            float r = 3.4f - 1.5f * t;
            cat_disc(bx, by, pass ? r : r + 1.0f, pass, t);
        }
    /* an ear flicks */
    if (G.cat_ear > 0.0f) {
        const uint8_t *e = crawl_cat_ear[G.cat_ear_side];
        float k = sinf(PI_F * (0.32f - G.cat_ear) / 0.32f);
        int dir = G.cat_ear_side ? 1 : -1;
        for (int y = e[1]; y <= e[3]; y++) {
            int sh = (int)lroundf(k * 2.4f * (float)(e[3] - y) / (float)(e[3] - e[1])) * dir;
            for (int x = e[0]; x <= e[2]; x++) {
                int sx = x - sh;
                cat_buf[y * CAT_W + x] = sx >= e[0] && sx <= e[2] ? crawl_cat_img[y * CAT_W + sx] : C_CLEAR;
            }
        }
    }
    /* the eyes: green, slit pupils on the caster, lids for a blink or a slow, fond one */
    float lid = 0.0f;
    if (G.cat_blink > 0.0f) lid = sinf(PI_F * (0.16f - G.cat_blink) / 0.16f);
    if (G.cat_slow > 0.0f && G.cat_slow <= 1.4f) { float u = 1.4f - G.cat_slow; lid = fmaxf(lid, u < 0.45f ? u / 0.45f : (u < 0.85f ? 1.0f : 1.0f - (u - 0.85f) / 0.55f)); }
    int look = (int)lroundf(G.cat_look);
    for (int e = 0; e < 2; e++) {
        int cx = crawl_cat_eye[e][0], cy = crawl_cat_eye[e][1], rx = crawl_cat_eye[e][2], ry = crawl_cat_eye[e][3];
        int shut = (int)(lid * (2 * ry + 1) + 0.5f);
        for (int y = cy - ry; y <= cy + ry; y++)
            for (int x = cx - rx; x <= cx + rx; x++) {
                float ux = (x - cx) / (rx + 0.5f), uy = (y - cy) / (ry + 0.5f);
                if (ux * ux + uy * uy > 1.0f) continue;
                int row = y - (cy - ry);
                uint8_t c;
                if (row < shut) c = row == shut - 1 ? crawl_cat_lid[1] : crawl_cat_lid[0];
                else {
                    c = row == 0 ? crawl_cat_eyecol[0] : (row <= ry ? crawl_cat_eyecol[1] : crawl_cat_eyecol[2]);
                    if (x == cx + look) c = crawl_cat_eyecol[3];
                    if (x == cx - 1 + look && y == cy - 1) c = crawl_cat_eyecol[4];
                }
                cat_buf[y * CAT_W + x] = c;
            }
    }
}

static int spr_bottom(int spr) {                                    /* lowest drawn row of a sprite */
    static int8_t cache[NSPR];
    static int init;
    if (!init) { memset(cache, -1, sizeof cache); init = 1; }
    if (cache[spr] < 0) {
        int b = 0;
        for (int x = 0; x < TS; x++) for (int y = 0; y < TS; y++) if (crawl_spr[spr][x * TS + y] != C_CLEAR && y > b) b = y;
        cache[spr] = (int8_t)b;
    }
    return cache[spr];
}

static int key_sprite(int color, float ph) {
    static const int BASE[NKEYS] = { SP_KEY0, SP_KEYS0, SP_KEYC0 };
    return BASE[color] + (((int)(ph * 5.0f)) & 3);
}

static int mob_sprite(const Mob *m, float *scale) {
    int fr = ((int)m->anim) & 1;
    *scale = 1.0f;
    switch (m->type) {
    case MOB_SKEL:
        return m->state == MS_DIE ? (m->t < 0.25f ? SP_SKEL_D1 : (m->t < 0.5f ? SP_SKEL_D2 : SP_SKEL_D3))
                                  : (m->throw_t > 0.0f ? SP_SKEL_T : (fr ? SP_SKEL_B : SP_SKEL_A));
    case MOB_SHIELD: {
        if (m->state == MS_DIE) return m->t < 0.25f ? SP_SKEL_D1 : (m->t < 0.5f ? SP_SKEL_D2 : SP_SKEL_D3);
        int o = m->shield > 0.0f ? 0 : SP_SHIELD_XA - SP_SHIELD_A;
        return o + (m->swing > 0.0f ? SP_SHIELD_S : (fr ? SP_SHIELD_B : SP_SHIELD_A));
    }
    case MOB_GHOST:
        *scale = 1.35f;                                            /* taller than a man */
        return m->state == MS_WALK ? (fr ? SP_GHOST_B : SP_GHOST_A) : (((int)(m->t * 14.0f)) & 1 ? SP_GHOST_D1 : SP_GHOST_D2);
    case MOB_WSKULL:
        *scale = 0.62f;
        if (m->state == MS_DIE) return SP_WSKULL_D;
        if (m->sub == 1) return m->sub_t > 2.2f && ((int)(m->sub_t * 16.0f) & 1) ? SP_WSKULL2 : (m->sub_t > 0.5f ? SP_WSKULL1 : SP_WSKULL0);
        return SP_WSKULL0;
    case MOB_SPIDER:
        *scale = 0.42f;
        if (m->state == MS_DIE) return SP_SPIDER_D;
        if (m->sub == 2) return SP_SPIDER_BITE;
        return ((int)(m->t * 10.0f)) & 1 ? SP_SPIDER_B : SP_SPIDER_A;
    default:
        return m->state == MS_DIE ? SP_BAT_D : ((int)(m->t * 9.0f) & 1 ? SP_BAT_B : SP_BAT_A);
    }
}

static int shot_sprite(const Shot *s, float *scale) {
    int fl = ((int)(G.t * 14.0)) & 1;
    *scale = 1.0f;
    switch (s->kind) {
    case AT_FROST: *scale = 0.7f; return SP_FROST;
    case AT_SPEAR: *scale = 0.8f; return SP_SPEAR;
    case AT_HELL: *scale = 1.3f; return fl ? SP_HELL_B : SP_HELL_A;
    case AT_WISPS: *scale = 0.6f; return SP_WISP;
    case AT_SPARKS: *scale = 0.6f; return SP_SPARK;
    case SHOT_REFLECT: return s->z > 0.5f ? (fl ? SP_SKULLFIRE_B : SP_SKULLFIRE_A) : SP_BONEFLY0 + (((int)(G.t * 14.0)) & 3);
    default: return fl ? SP_FIRE_B : SP_FIRE_A;
    }
}

static void render_sprites(Frame *f, float dx, float dy, float horizon) {
    static Draw list[MAXDECO + MAXMOBS + MAXSHOTS + MAXBOOMS + MAXBONES + MAXITEMS + MAXBURNS];
    int n = 0;
    float rx = -dy, ry = dx;
    #define ADD(X, Y, Z, S, GH) do { float vx_ = (X) - G.px, vy_ = (Y) - G.py, dp_ = vx_ * dx + vy_ * dy; \
        if (dp_ > 0.12f && dp_ < 24.0f) list[n++] = (Draw){ X, Y, Z, dp_, S, (GH) > 0.0f ? (GH) : 1.0f, 0.0f, 1.0f, 0, 0 }; } while (0)
    for (int i = 0; i < G.ndecos; i++) ADD(G.decos[i].x, G.decos[i].y, 0.0f, G.decos[i].spr, 0);
    for (int i = 0; i < MAXMOBS; i++) {
        Mob *m = &G.mobs[i];
        if (!m->alive) continue;
        float sc;
        int s = mob_sprite(m, &sc), n0 = n;
        ADD(m->x, m->y, m->z, s, m->type == MOB_GHOST ? (m->state == MS_RECOIL ? 0.9f : 0.72f * (1.0f - m->fade) + 0.01f) : 0.0f);
        if (n == n0) continue;
        Draw *d = &list[n - 1];
        d->scale = sc;
        if (m->type == MOB_SPIDER && m->state == MS_WALK && m->sub == 0) d->flags = DR_EYES;
        if (m->frozen > 0.0f && m->state == MS_WALK) d->flags |= DR_ICE;   /* only its eyes, in the dark of the web */
        if (m->state == MS_DIE && m->sub == 77) {                  /* drawn in to a point */
            float k = smoothstepf(0.1f, 0.75f, m->t);
            d->z += sc * 0.5f * k; d->scale = sc * (1.0f - k) + 0.02f; d->flags = DR_IMPLODE;
        }
    }
    for (int i = 0; i < MAXSHOTS; i++) {
        const Shot *s = &G.shots[i];
        if (!s->alive || (s->x - G.px) * dx + (s->y - G.py) * dy < 0.6f) continue;   /* the hand covers it leaving */
        float sc;
        int sp = shot_sprite(s, &sc), n0 = n;
        ADD(s->x, s->y, -0.02f + (1.0f - sc) * 0.5f, sp, 0);
        if (n > n0) list[n - 1].scale = sc;
    }
    for (int i = 0; i < MAXBOOMS; i++) {
        const Boom *b = &G.booms[i];
        if (!b->alive) continue;
        float sc = b->big ? b->r * 0.8f : 1.0f;
        int n0 = n;
        ADD(b->x, b->y, b->big ? -0.1f : 0.0f, SP_BOOM1 + clampi((int)(b->t / 0.2f), 0, 2), 0);
        if (n > n0) list[n - 1].scale = sc;
    }
    for (int i = 0; i < MAXBURNS; i++) {
        const Burn *b = &G.burns[i];
        if (!b->alive) continue;
        int n0 = n;
        ADD(b->x, b->y, 0.0f, ((int)(G.t * 9.0) + i) & 1 ? SP_BURN_B : SP_BURN_A, 1.0f - smoothstepf(1.6f, 2.5f, b->t));
        if (n > n0) list[n - 1].scale = 0.9f;
    }
    for (int i = 0; i < MAXBONES; i++) {
        const Bone *b = &G.bones[i];
        if (!b->alive) continue;
        if (b->kind == MIS_FIRE) {
            int n0 = n;
            ADD(b->x, b->y, 0.12f, ((int)(G.t * 14.0)) & 1 ? SP_SKULLFIRE_B : SP_SKULLFIRE_A, 0);
            if (n > n0) list[n - 1].scale = 0.8f;
        } else ADD(b->x, b->y, 0.04f, SP_BONEFLY0 + (((int)b->spin) & 3), 0);
    }
    for (int i = 0; i < G.nitems; i++) {
        const Item *it = &G.items[i];
        if (!it->alive) continue;
        static const int SPR[] = { SP_COINS_A, SP_CHALICE, SP_GEMS, SP_CROWN, SP_CHEST, SP_KEY0, SP_POTION, SP_SOULURN, SP_RINGITEM };
        float ph = (float)G.t + it->phase, z = 0.0f;
        int sp = SPR[it->type];
        if (it->type == IT_COINS && ((int)(ph * 2.5f) & 1)) sp = SP_COINS_B;
        if (it->type == IT_KEY) { sp = key_sprite(it->color, ph); z = 0.14f + 0.04f * sinf(ph * 2.5f); }
        int n0 = n;
        ADD(it->x, it->y, z, sp, 0);
        if (n > n0 && it->type != IT_KEY) { float u = ph * 0.6f - floorf(ph * 0.6f); list[n - 1].twinkle = u < 0.1f ? sinf(u / 0.1f * PI_F) : 0.0f; }
    }
    if (G.shop) {
        build_cat();
        int n0 = n;
        ADD(CAT_X, CAT_Y, 0.0f, SP_PEDESTAL, 0);
        if (n > n0) { list[n - 1].scale = CAT_S; list[n - 1].flags = DR_CAT; }
        for (int i = 0; i < NPOOL; i++) {
            if (G.offer_gone[i]) continue;
            const Offer *o = &G.offers[i];
            int sp = ITEM_SPR[o->kind];
            float sc = 0.42f, z = 0.55f + 0.03f * sinf((float)G.t * 2.0f + i * 2.1f) - (TS - 1 - spr_bottom(sp)) / (float)TS * sc;
            n0 = n;
            ADD(PED_X[i], PED_Y[i], z, sp, 0);
            if (n > n0) {
                Draw *d = &list[n - 1];
                d->scale = sc; d->remap = gear_color(o->kind, o->id);
                float u = (float)G.t * 0.5f + i * 0.33f; u -= floorf(u);
                d->twinkle = u < 0.12f ? sinf(u / 0.12f * PI_F) : 0.0f;
            }
        }
    }
    #undef ADD
    qsort(list, (size_t)n, sizeof list[0], cmp_draw);
    float bfs_dark = G.bfs_t > 0.0f ? 1.0f - smoothstepf(0.3f, 1.4f, G.bfs_t) : 0.0f;
    for (int i = 0; i < n; i++) {
        Draw *d = &list[i];
        float vx = d->x - G.px, vy = d->y - G.py;
        float lat = vx * rx + vy * ry;
        float size = FOCAL / d->depth * d->scale;
        float scx = W / 2 + FOCAL * lat / d->depth;
        float top = horizon - (d->scale - G.camz + d->z) * FOCAL / d->depth;
        float left = scx - size / 2;
        if (d->flags & DR_EYES) {                                  /* two red points, blinking now and then */
            if (fmodf((float)G.t * 0.7f + d->x * 3.1f, 4.0f) < 0.12f) continue;
            int big = size > 40.0f, gap = (int)fmaxf(1.0f, 3.0f * size / TS);
            for (int e = 0; e < 2; e++) {
                int sx = (int)(left + 32.0f * size / TS) + (e ? gap : -gap - big), sy = (int)(top + 30.5f * size / TS);
                for (int yy = 0; yy <= big; yy++)
                    for (int xx = 0; xx <= big; xx++) {
                        int px = sx + xx, py = sy + yy;
                        if (px >= 0 && px < W && py >= 0 && py < H && d->depth < zbuf[px]) f->pix[py * W + px] = (uint8_t)(R_BLOOD + 13 + (int)(2.5f * (0.5f + 0.5f * sinf((float)G.t * 2.0f + d->x))));
                    }
            }
            continue;
        }
        if (d->flags & DR_IMPLODE && d->scale < 0.06f) {           /* the last point of light */
            int sx = (int)scx, sy = (int)(top + size * 0.5f), arm = (int)clampf(6.0f / d->depth, 1.0f, 6.0f);
            for (int k = -arm; k <= arm; k++) {
                int xs[2] = { sx + k, sx }, ys[2] = { sy, sy + k };
                for (int j = 0; j < 2; j++)
                    if (xs[j] >= 0 && xs[j] < W && ys[j] >= 0 && ys[j] < H && d->depth < zbuf[xs[j]]) f->pix[ys[j] * W + xs[j]] = abs(k) > arm / 2 ? (uint8_t)(R_GLOW + 10) : (uint8_t)C_WHITE;
            }
            continue;
        }
        int x0 = clampi((int)ceilf(left - 0.5f), 0, W), x1 = clampi((int)ceilf(left + size - 0.5f), 0, W);
        int y0 = clampi((int)ceilf(top - 0.5f), 0, H), y1 = clampi((int)ceilf(top + size - 0.5f), 0, H);
        if (x0 >= x1 || y0 >= y1) continue;
        if (d->flags & DR_CAT) {                                   /* the merchant: a bigger, row-major picture */
            float light = fminf(1.3f, lm_at(d->x + 0.4f, d->y) * atten(d->depth) + 0.25f), cstep = CAT_W / size;
            for (int x = x0; x < x1; x++) {
                if (d->depth >= zbuf[x]) continue;
                int tx = clampi((int)((x + 0.5f - left) * cstep), 0, CAT_W - 1);
                float ty = (y0 + 0.5f - top) * cstep;
                for (int y = y0; y < y1; y++, ty += cstep) {
                    uint8_t c = cat_buf[clampi((int)ty, 0, CAT_H - 1) * CAT_W + tx];
                    if (c != C_CLEAR) f->pix[y * W + x] = shade(c, light, x, y);
                }
            }
            continue;
        }
        float light = lm_at(d->x, d->y) * atten(d->depth) * (1.0f - 0.85f * bfs_dark);
        float fog = fog_amount(d->x, d->y, 1.0f - expf(-d->depth * 0.28f));
        const uint8_t *img = crawl_spr[d->spr];
        float step = TS / size;
        for (int x = x0; x < x1; x++) {
            if (d->depth >= zbuf[x]) continue;
            int tx = clampi((int)((x + 0.5f - left) * step), 0, TS - 1);
            const uint8_t *col = &img[tx * TS];
            float ty = (y0 + 0.5f - top) * step;
            for (int y = y0; y < y1; y++, ty += step) {
                uint8_t c = col[clampi((int)ty, 0, TS - 1)];
                if (c == C_CLEAR) continue;
                if (d->alpha < 1.0f && d->alpha * 16.0f <= BAYER[(y + (int)(G.t * 20.0)) & 3][x & 3] + 0.5f) continue;
                if (d->remap && c >= R_PURPLE && c < R_PURPLE + 16) c = ramp_at(d->remap, c - R_PURPLE);
                if (d->flags & DR_IMPLODE) { f->pix[y * W + x] = c == 0 ? 0 : (uint8_t)(R_GLOW + clampi(6 + (int)((c & 15) * 0.6f), 6, 15)); continue; }
                if ((d->flags & DR_ICE) && c != 0 && ((x + y) & 1)) {    /* rimed with frost */
                    f->pix[y * W + x] = fogged((uint8_t)(R_GLOW + clampi(2 + (int)(((c - 1) & 15) * 0.5f + light * 4.0f), 2, 12)), fog, light, x, y);
                    continue;
                }
                f->pix[y * W + x] = (d->flags & DR_BRIGHT) ? c : fogged(shade(c, light, x, y), fog, light, x, y);
            }
        }
        if (d->twinkle > 0.0f && crawl_glint[d->spr][0] >= 0) {     /* a glint off the gold */
            int gx = (int)(left + (crawl_glint[d->spr][0] + 0.5f) * size / TS), gy = (int)(top + (crawl_glint[d->spr][1] + 0.5f) * size / TS);
            int arm = (int)(d->twinkle * fmaxf(1.0f, size / 28.0f));
            for (int k = -arm; k <= arm; k++) {
                int xs[2] = { gx + k, gx }, ys[2] = { gy, gy + k };
                for (int j = 0; j < 2; j++)
                    if (xs[j] >= 0 && xs[j] < W && ys[j] >= 0 && ys[j] < H && d->depth < zbuf[xs[j]]) f->pix[ys[j] * W + xs[j]] = abs(k) == arm && arm > 1 ? (uint8_t)(R_GOLD + 11) : (uint8_t)C_WHITE;
            }
        }
    }
    for (int i = 0; i < MAXMOBS; i++) {                            /* spiders hang by a thread */
        const Mob *m = &G.mobs[i];
        if (!m->alive || m->type != MOB_SPIDER || (m->state == MS_WALK && m->sub == 0) || (m->state == MS_DIE && m->sub != 9)) continue;
        if (m->state == MS_DIE && m->t > 0.25f) continue;          /* snapped */
        float L = lm_at(m->x, m->y);
        uint8_t c = (uint8_t)(R_BONE + clampi((int)(4.0f + L * 6.0f), 3, 11));
        line3d(f, dx, dy, horizon, m->ax, m->ay, 0.99f, m->x, m->y, m->z + 0.42f * 0.7f, c, 0, 1);
    }
}

/* ---- the merchant's wares: prices over the pedestals, a sparkle when one is taken ---- */
static void render_shop_tags(Frame *f, float dx, float dy, float horizon) {
    for (int i = 0; i < NPOOL; i++) {
        float sx, sy, dep;
        if (!project(PED_X[i], PED_Y[i], i == G.shop_pick && G.sparkle > 0.0f ? 0.78f : 1.04f, dx, dy, horizon, &sx, &sy, &dep)) continue;
        int x = (int)sx;
        if (x < 0 || x >= W || dep >= zbuf[x] + 0.3f) continue;
        if (G.offer_gone[i]) {
            if (i != G.shop_pick || G.sparkle <= 0.0f) continue;
            float k = 0.8f - G.sparkle;
            for (int p = 0; p < 18; p++) {                         /* a burst of light where it was */
                float a = p * TAU_F / 18 + k * 2.0f, r = k * (40.0f + 12.0f * (p & 1)) / fmaxf(dep, 0.8f);
                int px = (int)(sx + cosf(a) * r), py = (int)(sy + sinf(a) * r * 0.8f);
                if (px >= 0 && px < W && py >= 0 && py < H) f->pix[py * W + px] = p % 3 == 0 ? (uint8_t)C_WHITE : (uint8_t)(R_GLOW + 12);
            }
            continue;
        }
        const Offer *o = &G.offers[i];
        char tag[16];
        if (o->price == 0) snprintf(tag, sizeof tag, "FREE");
        else snprintf(tag, sizeof tag, "$%d", o->price);
        int w = gfx_text_width(tag, 1), tx = x - w / 2, ty = (int)sy - 10;
        if (ty < 0 || ty > H - 60) continue;
        uint8_t col = o->price <= RUN.treasure ? (uint8_t)(R_GOLD + 12) : (uint8_t)(R_BLOOD + 11);
        gfx_text8(f, tx + 1, ty + 1, tag, 0, 1);
        gfx_text8(f, tx, ty, tag, col, 1);
        if (o->rare && ((int)(G.t * 3.0) & 1)) { int rw = gfx_text_width("RARE", 1); gfx_text8(f, x - rw / 2 + 1, ty - 9, "RARE", 0, 1); gfx_text8(f, x - rw / 2, ty - 10, "RARE", C_WHITE, 1); }
    }
}

/* The caster's face, Wolfenstein style: bloodier as health runs out, glancing about, flinching
 * towards a blow, grinning at loot. The hood is the one it wears. */
static void draw_face(Frame *f, int ox, int oy, int sub) {
    const Gear *g = &RUN.gear;
    float hpf = G.max_hp > 0 ? (float)G.hp / (float)G.max_hp : 0.0f;
    int stage = clampi((int)((1.0f - hpf) * 7.0f), 0, 6);
    int fr = G.dead || G.hp <= 0 ? 29 : (G.grin_t > 0.0f ? 28 : stage * 4 + sub);
    int hood = gear_hood_ramp(g->hood, g->hood_tier);
    static const int TRIM[NHOOD] = { R_GOLD, R_WOOD, R_GLOW, R_WOOD, R_BLOOD };
    int trim = TRIM[g->hood];
    const uint8_t *img = crawl_face_img[fr];
    for (int y = 0; y < FACE_H; y++)
        for (int x = 0; x < FACE_W; x++) {
            uint8_t c = img[y * FACE_W + x];
            if (c == C_CLEAR) continue;
            int sx = ox + x, sy = oy + y;
            if (sx < 0 || sx >= W || sy < 0 || sy >= H) continue;
            if (c >= R_PURPLE && c < R_PURPLE + 16) c = (uint8_t)(hood + (c - R_PURPLE));
            else if (c >= R_GOLD && c < R_GOLD + 13 && trim != R_GOLD) c = (uint8_t)(trim + clampi(c - R_GOLD + 3, 0, 15));
            else if (c >= R_SKIN && c < R_SKIN + 16 && ((x + y) & 1)) {
                if (G.poison_t > 0.0f) c = (uint8_t)(R_MOSS + clampi(c - R_SKIN - 1, 0, 15));          /* green about the gills */
                else if (G.slow_t > 0.0f) c = (uint8_t)(R_GHOST + clampi(c - R_SKIN - 2, 0, 15));
            }
            f->pix[sy * W + sx] = c;
        }
}

static int face_sub(void) {
    if (G.face_t > FACE_T - 0.7f) return G.face_side == 0 ? 3 : (G.face_side < 0 ? 1 : 2);   /* the flinch */
    uint32_t h = hash2((int)(G.t / 1.3), 977);                     /* looking about, now and then */
    return h % 10 < 6 ? 0 : (h % 10 < 8 ? 1 : 2);
}

static void bevel(Frame *f, int x, int y, int w, int h, int fill) {
    gfx_rect(f, x, y, w, h, fill);
    for (int i = 0; i < w; i++) { gfx_px(f, x + i, y, R_IRON + 1); gfx_px(f, x + i, y + h - 1, R_IRON + 9); }
    for (int i = 0; i < h; i++) { gfx_px(f, x, y + i, R_IRON + 1); gfx_px(f, x + w - 1, y + i, R_IRON + 9); }
}

/* the status bar, in the antechamber only (nothing stays put on an OLED for long) */
static void key_icon(Frame *f, int kx, int ky, int color);
static void render_statusbar(Frame *f) {
    int y0 = H - 44;
    gfx_rect(f, 0, y0, W, 44, R_IRON + 4);
    for (int x = 0; x < W; x++) { gfx_px(f, x, y0, R_IRON + 9); gfx_px(f, x, y0 + 1, R_IRON + 6); }
    char num[24];
    struct { int x, w; const char *label; } P[5] = { { 4, 40, "LEVEL" }, { 48, 56, "GOLD" }, { 108, 50, "HEALTH" }, { 206, 42, "SOULS" }, { 252, 48, "SPELL" } };
    for (int i = 0; i < 5; i++) {
        bevel(f, P[i].x, y0 + 4, P[i].w, 37, R_IRON + 2);
        gfx_text8(f, P[i].x + (P[i].w - gfx_text_width(P[i].label, 1)) / 2, y0 + 7, P[i].label, R_IRON + 12, 1);
        uint8_t col = C_WHITE;
        switch (i) {
        case 0: snprintf(num, sizeof num, "%d", G.level); break;
        case 1: snprintf(num, sizeof num, "$%d", G.shop ? G.shop_gold : RUN.treasure); col = R_GOLD + 12; break;
        case 2: snprintf(num, sizeof num, "%d%%", G.max_hp ? G.hp * 100 / G.max_hp : 0); col = G.hp * 4 <= G.max_hp ? R_BLOOD + 13 : (G.poison_t > 0.0f ? R_MOSS + 13 : C_WHITE); break;
        case 3: snprintf(num, sizeof num, "%d", RUN.souls); col = R_GLOW + 12; break;
        default: {                                                 /* the spell in hand, by name and tier */
            int sp = clampi(G.spell, 0, NATK - 1), t = RUN.gear.atk[sp];
            uint8_t sc = ramp_at(gear_color(GK_ATK, sp), 13);
            gfx_text8(f, P[i].x + (P[i].w - gfx_text_width(ATK_SHORT[sp], 1)) / 2, y0 + 19, ATK_SHORT[sp], sc, 1);
            if (sp == AT_BFS) snprintf(num, sizeof num, "-%d", bfs_cost(t));
            else { char nm[40]; gear_item_name(GK_ATK, sp, t, nm, sizeof nm); const char *r = strrchr(nm, ' '); snprintf(num, sizeof num, "%s", t > 0 && r ? r + 1 : ""); }
            gfx_text8(f, P[i].x + (P[i].w - gfx_text_width(num, 1)) / 2, y0 + 30, num, R_IRON + 13, 1);
            continue;
        }
        }
        gfx_text16(f, P[i].x + (P[i].w - gfx_text_width(num, 1)) / 2, y0 + 19, num, col, 1);
    }
    bevel(f, 304, y0 + 4, 12, 37, R_IRON + 2);                     /* the keys in hand */
    for (int c = 0; c < NKEYS; c++) if (G.has_key[c]) key_icon(f, 306, y0 + 9 + c * 10, c);
    bevel(f, 162, y0 + 1, 40, 43, 0);
    draw_face(f, 166, y0 + 3, face_sub());
}

/* what the caster is looking at, and what it costs */
static void render_shop_card(Frame *f) {
    int i = G.shop_step == SH_LOOK || G.shop_step == SH_GO || (G.shop_step == SH_BUY && !G.shop_bought) ? G.shop_look : -1;
    if (G.shop_step == SH_LOOK && G.shop_t < 0.35) i = -1;
    if (G.player) i = G.shop_look;
    int y0 = H - 44 - 46;
    if (i < 0 || G.offer_gone[i]) {
        if (!G.player && G.shop_step == SH_THANK && !G.shop_bought) {
            const char *msg = G.shop_saving ? "SAVING UP FOR SOMETHING BETTER" : "NOTHING TAKES ITS FANCY";
            int w = gfx_text_width(msg, 1);
            gfx_rect(f, (W - w) / 2 - 6, y0 + 22, w + 12, 16, 0);
            gfx_text8(f, (W - w) / 2, y0 + 26, msg, R_GOLD + 10, 1);
        }
        return;
    }
    static const char *const POOL[] = { "ATTACK SPELL", "DEFENSIVE RUNE", "ROBE", "HOOD", "RING" };
    const Offer *o = &G.offers[i];
    char name[40], price[16];
    gear_name(o, name, sizeof name);
    if (o->price == 0) snprintf(price, sizeof price, "FREE");
    else snprintf(price, sizeof price, "$%d", o->price);
    int bw = 264, bx = (W - bw) / 2;
    gfx_rect(f, bx, y0, bw, 42, 0);
    for (int x = 0; x < bw; x++) { gfx_px(f, bx + x, y0, R_STONE + 10); gfx_px(f, bx + x, y0 + 41, R_STONE + 6); }
    for (int y = 0; y < 42; y++) { gfx_px(f, bx, y0 + y, R_STONE + 10); gfx_px(f, bx + bw - 1, y0 + y, R_STONE + 6); }
    gfx_text8(f, bx + 8, y0 + 5, POOL[o->kind], R_STONE + 11, 1);
    if (o->rare) gfx_text8(f, bx + bw - 8 - gfx_text_width("RARE", 1), y0 + 5, "RARE", ((int)(G.t * 3.0) & 1) ? C_WHITE : R_GOLD + 12, 1);
    int col = gear_color(o->kind, o->id);
    gfx_text16(f, bx + 8, y0 + 14, name, ramp_at(col, 13), 1);
    gfx_text8(f, bx + 8, y0 + 31, gear_blurb(o), R_STONE + 13, 1);
    gfx_text16(f, bx + bw - 8 - gfx_text_width(price, 1), y0 + 14, price, o->price <= RUN.treasure ? R_GOLD + 12 : R_BLOOD + 12, 1);
}

/* in the crypt: the face pops up from the bottom-left corner when hurt or pleased, then goes */
static void render_face_popup(Frame *f) {
    float t = fmaxf(G.face_t, G.grin_t > 0.0f ? fminf(G.grin_t, FACE_T) : 0.0f);
    if (G.dead) t = (float)fminf(G.dead_t, 4.0);
    if (t <= 0.0f) return;
    float k = G.dead ? smoothstepf(0.0f, 0.3f, t) : fminf(smoothstepf(FACE_T, FACE_T - 0.2f, t), smoothstepf(0.0f, 0.35f, t));
    int x = -44 + (int)(k * 50.0f), y = H - 4 - 18 - 4 - 46;
    if (G.readout <= 0.0f) y = H - 4 - 46;
    bevel(f, x, y, 40, 46, 0);
    if (G.ward_hp > 0.0f) for (int i = 1; i < 39; i++) { gfx_px(f, x + i, y + 1, R_BONE + 13); gfx_px(f, x + i, y + 44, R_BONE + 13); }
    draw_face(f, x + 4, y + 3, face_sub());
}

/* what the defensive spells look like while they last */
static void render_wards(Frame *f) {
    if (G.ward_hp > 0.0f) {                                        /* bone shards wheeling round the caster */
        int n = 2 + (int)(G.ward_hp / 12.0f);
        for (int i = 0; i < n && i < 5; i++) {
            float a = (float)G.t * 2.2f + i * TAU_F / n, sa = sinf(a);
            if (sa < -0.2f) continue;                              /* behind */
            int size = (int)(18 + 10 * sa), cx = 160 + (int)(cosf(a) * 150), cy = 128 + (int)(sa * 42);
            const uint8_t *img = crawl_spr[SP_BONEFLY0 + (((int)(G.t * 10.0) + i) & 3)];
            for (int x = 0; x < size; x++)
                for (int y = 0; y < size; y++) {
                    int sx = cx - size / 2 + x, sy = cy - size / 2 + y;
                    if (sx < 0 || sx >= W || sy < 0 || sy >= H) continue;
                    uint8_t c = img[(x * TS / size) * TS + y * TS / size];
                    if (c != C_CLEAR) f->pix[sy * W + sx] = shade(c, 0.9f + 0.3f * sa, sx, sy);
                }
        }
    }
    if (G.mirror_t > 0.0f) {                                       /* a silvered ripple at the edges of sight */
        float k = fminf(1.0f, G.mirror_t * 2.0f);
        for (int y = 0; y < H; y++) {
            int w = (int)((5.0f + 3.0f * sinf(y * 0.13f + (float)G.t * 6.0f)) * k);
            for (int x = 0; x < w; x++) {
                if (BAYER[y & 3][x & 3] > (int)(12 * (1.0f - (float)x / (w + 1)))) continue;
                uint8_t c = (uint8_t)(R_GLOW + 8 + ((x + y) & 3));
                f->pix[y * W + x] = c; f->pix[y * W + W - 1 - x] = c;
            }
        }
    }
}

/* The hand as the caster is dressed: the sleeve in the robe's cloth, rings on the fingers, and a rune
 * tattooed on the back of the hand for each defensive spell it knows. Rebuilt when the gear changes. */
static uint8_t hand_buf[HAND_H * HAND_W], hand_rune[HAND_H * HAND_W];   /* hand_rune: 1 + spell for tattoo ink */
static Gear hand_gear; static int hand_built;

/* finger slots: ring, middle, index, little; where a ring sits on each (back of the hand, fingers up-left) */
static const int RING_AT[NFINGERS][2] = { { 55, 32 }, { 40, 36 }, { 26, 34 }, { 71, 38 } };
static const int RUNE_AT[NDEF][2] = { { 40, 50 }, { 56, 49 }, { 72, 50 }, { 45, 61 }, { 61, 61 }, { 77, 62 }, { 51, 72 }, { 67, 73 }, { 83, 74 } };
static const char *const RUNE_GLYPH[NDEF][7] = {
    { "#.#.#", "#.#.#", ".###.", "..#..", "..#..", "..#..", "..#.." },   /* algiz: the ward */
    { "##...", "#.#..", "#..#.", "#.#..", "##...", "#.#..", "#..#." },   /* raido: the journey */
    { "#...#", "##.##", "#.#.#", "#...#", "#...#", "#...#", "#...#" },   /* mannaz: the mirror */
    { "##...", "#.#..", "#.#..", "##...", "#.#..", "#.#..", "##..." },   /* berkano: healing */
    { "#....", "##...", "#.#..", "#....", "#....", "#....", "#...." },   /* laguz: water, cleansing */
    { "#.#.#", ".###.", "#####", ".###.", "#.#.#", "..#..", "..#.." },   /* frost, a star */
    { "#....", "##...", "#.#..", "#..#.", "#.#..", "##...", "#...." },   /* thurisaz: the thorn, dread */
    { "#...#", "##.##", "#.#.#", "#...#", "#.#.#", "##.##", "#...#" },   /* dagaz: the turning of time */
    { "###..", "#.#..", "#..#.", "#.#..", "###..", "#.#..", "#..#." },   /* perthro: lots, fate, souls */
};

static int is_skin(int x, int y) {
    if (x < 0 || y < 0 || x >= HAND_W || y >= HAND_H) return 0;
    uint8_t c = crawl_hand_img[y * HAND_W + x];
    return c >= R_SKIN && c < R_SKIN + 16;
}

static void build_hand(void) {
    const Gear *g = &RUN.gear;
    memcpy(hand_buf, crawl_hand_img, sizeof hand_buf);
    memset(hand_rune, 0, sizeof hand_rune);
    int ramp = gear_robe_ramp(g->robe, g->robe_tier);
    for (int y = 0; y < HAND_H; y++)                               /* the sleeve, in the robe's cloth */
        for (int x = 0; x < HAND_W; x++) {
            uint8_t *p = &hand_buf[y * HAND_W + x];
            if (*p < R_PURPLE || *p >= R_PURPLE + 16) continue;
            int v = *p - R_PURPLE;
            switch (g->robe) {
            case RB_SHADOW:                                        /* soot-dark, shifting */
                v = v - 3 - (BAYER[y & 3][x & 3] < 6 ? 2 : 0); break;
            case RB_SILK: {                                        /* a web woven into it */
                float a = atan2f(y - 150.0f, x - 150.0f), r = hypotf(x - 150.0f, y - 150.0f);
                if (fabsf(fmodf(a * 7.0f / PI_F + 20.0f, 1.0f) - 0.5f) > 0.46f || fabsf(fmodf(r, 11.0f)) < 0.8f) v += 4;
                break;
            }
            case RB_THORN: {                                       /* briars wound round the sleeve, under the cuff */
                int hit = 0;
                for (int b = 0; b < 2 && !hit; b++) {
                    float vy = 117.0f - (x - 54) * 0.59f + 13.0f + b * 20.0f + 3.5f * sinf(x * 0.21f + b * 2.0f);
                    if (fabsf(y - vy) < 0.9f) { *p = (uint8_t)(R_BLOOD + 3); hit = 1; }
                    else if (((x + b * 4) % 8) == 0 && y < vy && y > vy - 4) { *p = (uint8_t)(R_BLOOD + 5); hit = 1; }
                    else if (((x + b * 4) % 8) == 4 && y > vy && y < vy + 4) { *p = (uint8_t)(R_BLOOD + 5); hit = 1; }
                }
                if (hit) continue;
                break;
            }
            case RB_BLESSED: {                                     /* a gold sun on the cloth */
                float d = hypotf(x - 118.0f, y - 118.0f);
                if (d < 4.5f || (d < 10.0f && fabsf(fmodf(atan2f(y - 118.0f, x - 118.0f) * 8.0f / PI_F + 16.0f, 2.0f) - 1.0f) < 0.3f)) { *p = (uint8_t)(R_GOLD + 9 - (int)(d * 0.4f)); continue; }
                break;
            }
            default: break;
            }
            *p = (uint8_t)(ramp + clampi(v, 0, 15));
        }
    if (g->robe_tier >= 4)                                         /* finer robes: a second band of trim */
        for (int y = 1; y < HAND_H; y++)
            for (int x = 0; x < HAND_W - 6; x++)
                if (crawl_hand_img[y * HAND_W + x] >= R_GOLD && crawl_hand_img[y * HAND_W + x] < R_GOLD + 13) {
                    int tx = x + 4, ty = y + 5;
                    if (tx < HAND_W && ty < HAND_H && crawl_hand_img[ty * HAND_W + tx] >= R_PURPLE && crawl_hand_img[ty * HAND_W + tx] < R_PURPLE + 16)
                        hand_buf[ty * HAND_W + tx] = (uint8_t)(R_GOLD + 6 + (x & 1) * 3);
                }
    /* rings: a band across the finger, a stone set in it */
    static const int METAL[NRING] = { R_BONE, R_IRON, R_GOLD, R_GOLD, R_IRON };
    static const int GEM[NRING] = { -1, R_GLOW, R_MOSS, R_FLAME, R_BLOOD };
    float ux = 0.29f, uy = 0.96f, vx = 0.96f, vy = -0.29f;          /* along the finger, and across it */
    for (int f = 0; f < NFINGERS; f++) {
        if (g->ring[f] < 0) continue;
        int id = g->ring[f], wide = g->ring_tier[f] >= 4 ? 2 : 1;
        float cx = (float)RING_AT[f][0], cy = (float)RING_AT[f][1];
        for (int a = -wide; a <= 1; a++)
            for (int side = -1; side <= 1; side += 2)
                for (int sd = side < 0 ? 0 : 1; sd <= 7; sd++) {
                    float s = (float)(side * sd);
                    int x = (int)lroundf(cx + vx * s + ux * a), y = (int)lroundf(cy + vy * s + uy * a);
                    if (!is_skin(x, y) || crawl_hand_img[y * HAND_W + x] <= R_SKIN + 1) break;   /* the edge of the finger */
                    float lit = 0.5f + 0.5f * cosf(s * 0.25f + 0.6f) - (a == 1 ? 0.25f : 0.0f) - (a == -wide ? 0.1f : 0.0f);
                    hand_buf[y * HAND_W + x] = (uint8_t)(METAL[id] + clampi((int)(4 + lit * 10), 2, 14));
                }
        if (GEM[id] >= 0) {
            int gx = (int)lroundf(cx - ux * 0.5f), gy = (int)lroundf(cy - uy * 0.5f), r = g->ring_tier[f] >= 4 ? 2 : 1;
            for (int y = -r; y <= r; y++)
                for (int x = -r; x <= r; x++)
                    if (abs(x) + abs(y) <= r + (r > 1)) hand_buf[(gy + y) * HAND_W + gx + x] = (uint8_t)(GEM[id] + 9 + (x < 0) * 3 + (y < 0) * 2);
            hand_buf[(gy - r) * HAND_W + gx - (r > 1)] = C_WHITE;
        }
    }
    /* rune tattoos */
    for (int k = 0; k < NDEF; k++) {
        if (g->def[k] <= 0) continue;
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++) {
                if (RUNE_GLYPH[k][y][x] != '#') continue;
                int hx = RUNE_AT[k][0] - 2 + x + (6 - y) / 3, hy = RUNE_AT[k][1] - 3 + y;   /* slanted with the hand */
                if (!is_skin(hx, hy)) continue;
                hand_buf[hy * HAND_W + hx] = (uint8_t)(R_IRON + 4);
                hand_rune[hy * HAND_W + hx] = (uint8_t)(1 + k);
            }
    }
    hand_gear = *g; hand_built = 1;
}

static void render_hand(Frame *f) {
    float lift = G.rune_lift > 0.0f ? 0.8f * smoothstepf(1.4f, 1.25f, G.rune_lift) * smoothstepf(0.0f, 0.3f, G.rune_lift) : 0.0f;
    float hand = fmaxf(G.hand, lift);
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_SHOWHAND")) { hand = 1.0f; G.rune_glow[atoi(getenv("CRAWL_SHOWHAND")) % NDEF] = 1.0f; }
#endif
    if (hand <= 0.0f) return;
    if (!hand_built || memcmp(&hand_gear, &RUN.gear, sizeof hand_gear)) build_hand();
    /* the hand rises from the bottom right, reaching up and over, Catacomb 3-D / Doom style */
    float walk = G.player ? clampf(G.spd / WALK, 0.0f, 1.0f) : 0.0f;
    int ox = 170 + (int)(2.0f * sinf((float)G.t * 3.0f) + 5.0f * walk * sinf(G.bobph * 0.5f)),
        oy = H - (int)(hand * 130.0f) + (int)(2.0f * sinf((float)G.t * 6.0f) + 5.0f * walk * fabsf(sinf(G.bobph * 0.5f))) - (G.shop || G.player ? 40 : 0);
    float light = fminf(lm_at(G.px, G.py) + 0.6f * hand, 1.25f);
    for (int y = 0; y < HAND_H; y++) {
        int sy = oy + y;
        if (sy < 0 || sy >= H) continue;
        for (int x = 0; x < HAND_W; x++) {
            uint8_t c = hand_buf[y * HAND_W + x];
            if (c == C_CLEAR) continue;
            int sx = ox + x;
            if (sx < 0 || sx >= W) continue;
            int rk = hand_rune[y * HAND_W + x];
            if (rk && G.rune_glow[rk - 1] > 0.0f) {                    /* the rune burning as its spell goes off */
                static const int GLOW[NDEF] = { R_GLOW, R_GLOW, R_GLOW, R_GOLD, R_MOSS, R_GLOW, R_FLAME, R_GLOW, R_GLOW };
                float k = fminf(1.0f, G.rune_glow[rk - 1] / 0.8f);
                f->pix[sy * W + sx] = ramp_at(GLOW[rk - 1], (int)(6 + 9 * k));
                continue;
            }
            f->pix[sy * W + sx] = shade(c, light, sx, sy);
        }
    }
    float ct = -1.0f, fire = 0.0f;                                 /* how far into a cast, and when it lets go */
    if (G.state == PS_CAST) { ct = G.st; fire = 0.27f * G.cast_len; }
    else if (G.player && G.pc_t >= 0.0f) { ct = G.pc_t; fire = 0.35f * G.pc_len; }
    if (ct >= 0.0f && ct < fire + 0.03f) {                         /* the spell forming over the fingertips */
        static const int RAMP[NATK] = { R_FLAME, R_GLOW, R_GLOW, R_BONE, R_BLOOD, R_FLAME, R_GLOW, R_FLAME, R_PURPLE };
        static const float SIZE[NATK] = { 1.0f, 0.8f, 0.8f, 0.7f, 0.9f, 1.35f, 0.75f, 0.55f, 1.2f };
        int sp = clampi(G.spell, 0, NATK - 1), ramp = RAMP[sp];
        float r = (3.0f + 7.0f * clampf(ct / (fire * 0.8f), 0.0f, 1.0f)) * SIZE[sp];
        int cx = ox + 32, cy = oy + 1;                             /* between the first two fingertips */
        uint32_t jr = (uint32_t)(G.t * 40.0) * 2246822519u;
        for (int y = (int)(cy - r - 2); y <= (int)(cy + r + 2); y++)
            for (int x = (int)(cx - r - 2); x <= (int)(cx + r + 2); x++) {
                if (x < 0 || x >= W || y < 0 || y >= H) continue;
                float d = hypotf(x + 0.5f - cx, y + 0.5f - cy) / r;
                if (sp == AT_BFS) {                                /* a hole in the air with a violet rim */
                    if (d < 0.8f) f->pix[y * W + x] = 0;
                    else if (d < 1.15f) f->pix[y * W + x] = (uint8_t)(R_PURPLE + 15 - (int)((d - 0.8f) * 20));
                    continue;
                }
                if (sp == AT_CHAIN && d > 0.6f && d < 1.3f && (rng_next(&jr) & 7) == 0) { f->pix[y * W + x] = C_WHITE; continue; }   /* crackle */
                if (d > 1.0f) continue;
                f->pix[y * W + x] = (uint8_t)(ramp + clampi((int)((1.05f - d * 0.8f) * 15), 2, 15));
            }
    }
}

static void card_box(Frame *f, int bw, int bh, int by) {
    int bx = (W - bw) / 2;
    gfx_rect(f, bx, by, bw, bh, 0);
    for (int i = 0; i < bw; i++) { gfx_px(f, bx + i, by, R_STONE + 11); gfx_px(f, bx + i, by + bh - 1, R_STONE + 11);
                                   gfx_px(f, bx + i, by + 2, R_STONE + 6); gfx_px(f, bx + i, by + bh - 3, R_STONE + 6); }
    for (int i = 0; i < bh; i++) { gfx_px(f, bx, by + i, R_STONE + 11); gfx_px(f, bx + bw - 1, by + i, R_STONE + 11);
                                   if (i >= 2 && i < bh - 2) { gfx_px(f, bx + 2, by + i, R_STONE + 6); gfx_px(f, bx + bw - 3, by + i, R_STONE + 6); } }
}

static void card_text(Frame *f, int y, const char *s, int c) { gfx_text16(f, (W - gfx_text_width(s, 1)) / 2, y, s, c, 1); }

static void render_card(Frame *f) {
    char line[48];
    if (G.dead && G.dead_t > 1.1 && G.dead_t < 4.6) {
        card_box(f, 232, 64, 68);
        card_text(f, 80, "YOU HAVE PERISHED", R_BLOOD + 13);
        snprintf(line, sizeof line, "LEVEL %d BEGINS AGAIN", G.level);
        card_text(f, 104, line, C_WHITE);
        return;
    }
    if (G.card < 0.3f || G.card > 3.6f) return;
    snprintf(line, sizeof line, "LEVEL %d", G.level);
    const char *name = LEVEL_NAMES[(G.level - 1) % (int)(sizeof LEVEL_NAMES / sizeof LEVEL_NAMES[0])];
    card_box(f, 210, 64, 68);
    card_text(f, 80, line, C_WHITE);
    card_text(f, 104, name, R_FLAME + 11);
}

/* Wolfenstein-style intermission: the numbers run up one row at a time */
static void render_tally(Frame *f) {
    memset(f->pix, 0, (size_t)W * H);
    float t = (float)G.tally_t;
    card_box(f, 304, 164, 18);
    char line[48];
    snprintf(line, sizeof line, "LEVEL %d COMPLETE!", G.level);
    int flash = t < 0.6f && ((int)(t * 12.0f) & 1);
    gfx_text16(f, (W - gfx_text_width(line, 2)) / 2 + 1, 29, line, R_GOLD + 2, 2);          /* drop shadow */
    gfx_text16(f, (W - gfx_text_width(line, 2)) / 2, 28, line, flash ? C_WHITE : R_GOLD + 12, 2);
    const char *name = LEVEL_NAMES[(G.level - 1) % (int)(sizeof LEVEL_NAMES / sizeof LEVEL_NAMES[0])];
    card_text(f, 64, name, R_FLAME + 9);
    static const char *LABEL[TALLY_ROWS] = { "TIME", "KILLS", "TREASURE", "SOULS" };
    int totals[TALLY_ROWS] = { 0, G.kills_total, G.items_total, 0 };
    for (int r = 0; r < TALLY_ROWS; r++) {
        if (t < TALLY_ROW0 + r * TALLY_ROW_GAP - 0.3f) continue;
        int y = 86 + r * 19, v = G.tally_shown[r];
        gfx_text16(f, 52, y, LABEL[r], C_WHITE, 1);
        if (r == 0) snprintf(line, sizeof line, "%d:%02d", v / 60, v % 60);
        else if (r == 3) snprintf(line, sizeof line, "+%d", v);
        else snprintf(line, sizeof line, "%d / %d", v, totals[r]);
        int done = t >= TALLY_ROW0 + r * TALLY_ROW_GAP + TALLY_COUNT;
        gfx_text16(f, 268 - gfx_text_width(line, 1), y, line, done ? R_GOLD + 12 : R_GOLD + 8, 1);
        for (int x = 52 + gfx_text_width(LABEL[r], 1) + 6; x < 262 - gfx_text_width(line, 1); x += 4) gfx_px(f, x, y + 12, R_STONE + 7);   /* leader dots */
    }
    if (t > TALLY_ROW0 + TALLY_ROWS * TALLY_ROW_GAP + 0.2f) {
        int perfect = G.items_total > 0 && G.items_got == G.items_total;
        int all_dead = G.kills_total > 0 && G.kills == G.kills_total;
        const char *msg = perfect && all_dead ? "PERFECT!" : (perfect ? "ALL TREASURE!" : (all_dead ? "NONE LEFT STANDING" : ""));
        if (*msg && ((int)(t * 3.0f) & 1)) card_text(f, 166, msg, perfect ? R_GOLD + 12 : R_BLOOD + 13);
    }
}

/* HP, gold, souls and keys: slides up from the bottom edge when something changes, then away */
static const char *const KEY_ART[5] = { ".##.....", "#..#####", "#..#..#.", ".##...#.", "........" };
static void key_icon(Frame *f, int kx, int ky, int color) {
    static const int RAMP[NKEYS] = { R_GOLD, R_IRON, R_BLOOD };
    for (int y = 0; y < 5; y++)
        for (int x = 0; x < 8; x++)
            if (KEY_ART[y][x] == '#') gfx_px(f, kx + x, ky + y, (uint8_t)(RAMP[color] + (color == 0 ? (y < 2 ? 12 : 9) : (y < 2 ? 14 : 11))));
}
static void render_readout(Frame *f) {
    if (G.readout <= 0.0f || G.dead) return;
    float shown = READOUT_T - G.readout, k = fminf(smoothstepf(0.0f, 0.25f, shown), 1.0f - smoothstepf(0.0f, 0.35f, 0.35f - G.readout));
    if (G.readout > READOUT_T) k = smoothstepf(READOUT_T + 1.5f, READOUT_T + 1.25f, G.readout);
    if (k <= 0.0f) return;
    char num[16], gold[16], souls[16];
    snprintf(gold, sizeof gold, "%d", RUN.treasure);
    snprintf(souls, sizeof souls, "%d", RUN.souls);
    int nk = 0;
    for (int c = 0; c < NKEYS; c++) nk += G.has_key[c] > 0;
    int show_souls = RUN.souls > 0 || RUN.gear.atk[AT_BFS] >= 0;
    int gx = 84, sx = gx + 10 + gfx_text_width(gold, 1) + 8;
    int kx = show_souls ? sx + 12 + gfx_text_width(souls, 1) + 8 : sx;
    int pw = kx + nk * 11 + 4, ph = 18, px = 6, py = H - 4 - (int)(k * (ph + 4)) + 4;
    gfx_rect(f, px, py, pw, ph, 0);
    for (int i = 0; i < pw; i++) { gfx_px(f, px + i, py, R_STONE + 9); gfx_px(f, px + i, py + ph - 1, R_STONE + 5); }
    for (int i = 0; i < ph; i++) { gfx_px(f, px, py + i, R_STONE + 9); gfx_px(f, px + pw - 1, py + i, R_STONE + 5); }
    int low = G.hp <= G.max_hp / 4 && ((int)(G.t * 4.0) & 1);
    uint8_t hcol = G.poison_t > 0.0f ? (uint8_t)(R_MOSS + 13) : (low ? (uint8_t)(R_BLOOD + 15) : (uint8_t)(R_BLOOD + 12));
    gfx_glyph8(f, px + 5, py + 5, 3, hcol, -1, 1);
    snprintf(num, sizeof num, "%3d", G.hp);
    gfx_text8(f, px + 14, py + 5, num, C_WHITE, 1);
    int bx = px + 42, bw = 34, fill = (G.hp * bw + G.max_hp / 2) / G.max_hp;
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < bw; x++) {
            uint8_t c = x < fill ? (uint8_t)((G.poison_t > 0.0f ? R_MOSS : R_BLOOD) + (y == 0 ? 14 : 11)) : (uint8_t)(R_BLOOD + 3);
            if (G.ward_hp > 0.0f && y == 0 && x < (int)(G.ward_hp * bw / 40.0f)) c = (uint8_t)(R_BONE + 14);   /* the ward, a pale line over it */
            gfx_px(f, bx + x, py + 7 + y, c);
        }
    gfx_glyph8(f, px + gx, py + 5, '$', R_GOLD + 12, -1, 1);
    gfx_text8(f, px + gx + 10, py + 5, gold, R_GOLD + 10, 1);
    if (show_souls) {                                              /* a soul: a little wisp */
        int wx = px + sx, wy = py + 4;
        static const char *SOUL[8] = { "..#..", ".###.", "#####", "#####", ".###.", "..##.", "...#.", "....#" };
        for (int y = 0; y < 8; y++) for (int x = 0; x < 5; x++) if (SOUL[y][x] == '#') gfx_px(f, wx + x, wy + y, (uint8_t)(R_GLOW + (y < 4 ? 13 : 9)));
        int ready = RUN.gear.atk[AT_BFS] >= 0 && RUN.souls >= bfs_cost(RUN.gear.atk[AT_BFS]);
        gfx_text8(f, wx + 8, py + 5, souls, ready && ((int)(G.t * 3.0) & 1) ? C_WHITE : (uint8_t)(R_GLOW + 11), 1);
    }
    for (int c = 0, i = 0; c < NKEYS; c++) if (G.has_key[c] > 0) key_icon(f, px + kx + (i++) * 11, py + 6, c);
}

/* screen tints: red when hurt or dying, gold for treasure; index 0 stays black for the OLED */
static void tint(Frame *f) {
    float red = 0.30f * G.hurt, gold = 0.22f * G.gold, dark = 0.0f, chill = 0.3f * G.chill;
    float green = G.poison_t > 0.0f ? 0.20f + 0.07f * sinf((float)G.t * 4.0f) : 0.0f;          /* poisoned */
    float blue = G.slow_t > 0.0f && G.poison_t <= 0.0f ? 0.10f * fminf(1.0f, G.slow_t) : 0.0f;  /* webbed and slowed */
    float violet = G.slip_t > 0.0f ? 0.10f * fminf(1.0f, G.slip_t) : 0.0f;                      /* Time Slip */
    if (G.dead) { float k = smoothstepf(0.0f, 1.4f, (float)G.dead_t); red = fmaxf(red, 0.45f * k); dark = 0.35f * k; }
    else if (G.hp <= G.max_hp / 4) red = fmaxf(red, 0.08f * (0.5f + 0.5f * sinf((float)G.t * 5.7f)));
    if (G.dread_t > 0.0f) { float k = sinf(PI_F * G.dread_t / 0.8f); dark = fmaxf(dark, 0.45f * k); violet = fmaxf(violet, 0.18f * k); }   /* Dread: a shadow falls */
    if (G.bfs_t > 0.0f) dark = fmaxf(dark, G.bfs_t < 0.12f ? 0.97f : 0.9f * (1.0f - smoothstepf(0.12f, 1.3f, G.bfs_t)));   /* the light goes out of the room */
    if (red <= 0.0f && gold <= 0.0f && chill <= 0.0f && green <= 0.0f && blue <= 0.0f && violet <= 0.0f && dark <= 0.0f) return;
    for (int i = 1; i < 256; i++) {
        int glow = G.bfs_t > 0.0f && i >= R_GLOW && i < R_GLOW + 16;   /* the imploding keep their light */
        for (int c = 0; c < 3; c++) {
            float v = f->pal[i][c] * (glow || i == C_WHITE ? 1.0f : 1.0f - dark);
            v += (50.0f * (c == 0) - v) * red * (c == 0 ? 0.6f : 1.0f);
            if (c < 2) v += ((c ? 50.0f : 63.0f) - v) * gold; else v -= v * gold;
            v += ((c == 0 ? 30.0f : 52.0f) - v) * chill;                 /* a wraith's cold breath */
            v += ((c == 1 ? 40.0f : 8.0f) - v) * green;
            v += ((c == 2 ? 50.0f : 20.0f) - v) * blue;
            v += ((c == 1 ? 10.0f : 44.0f) - v) * violet;
            f->pal[i][c] = (uint8_t)clampi((int)(v + 0.5f), 0, 63);
        }
    }
}

/* a line of news at the top of the view, and the first-time hints */
static void render_messages(Frame *f) {
    if (G.msg_t > 0.0f && G.msg[0] && !(G.player && G.ui)) {
        if (G.msg_t > 0.3f || ((int)(G.msg_t * 20.0f) & 1)) {
            int w = gfx_text_width(G.msg, 1), x = (W - w) / 2;
            gfx_rect(f, x - 5, 14, w + 10, 14, 0);
            gfx_text8(f, x, 17, G.msg, R_GOLD + 11, 1);
        }
    }
    if (G.hint_t > 0.0f && G.player && !G.ui) {
        static const char *const HINT[3] = { "WASD WALK  MOUSE TURN  CLICK CASTS", "1-9 SPELLS  E OPENS  TAB INVENTORY", "ESC TO LEAVE OR SLEEPWALK" };
        for (int i = 0; i < 3; i++) {
            int w = gfx_text_width(HINT[i], 1), x = (W - w) / 2, y = 34 + i * 11;
            gfx_rect(f, x - 4, y - 2, w + 8, 11, 0);
            gfx_text8(f, x, y, HINT[i], R_STONE + 13, 1);
        }
    }
}

static void dim_view(Frame *f) {                                   /* the crypt goes dim behind a menu */
    for (int y = 0; y < H; y++)
        for (int x = (y & 1); x < W; x += 2) f->pix[y * W + x] = 0;
}

static void panel(Frame *f, int x, int y, int w, int h) {
    gfx_rect(f, x, y, w, h, 0);
    for (int i = 0; i < w; i++) { gfx_px(f, x + i, y, R_STONE + 11); gfx_px(f, x + i, y + h - 1, R_STONE + 6); gfx_px(f, x + i, y + 2, R_STONE + 6); gfx_px(f, x + i, y + h - 3, R_STONE + 4); }
    for (int i = 0; i < h; i++) { gfx_px(f, x, y + i, R_STONE + 11); gfx_px(f, x + w - 1, y + i, R_STONE + 6); if (i >= 2 && i < h - 2) { gfx_px(f, x + 2, y + i, R_STONE + 6); gfx_px(f, x + w - 3, y + i, R_STONE + 4); } }
}

/* Tab: what you wear (A and D change it) and how it adds up, and the spellbook */
static void render_inventory(Frame *f) {
    dim_view(f);
    panel(f, 4, 4, W - 8, H - 8);
    gfx_text16(f, 14, 9, "WARDROBE", R_GOLD + 12, 1);
    gfx_text8(f, 14, 26, "W/S PICK  A/D CHANGE  TAB CLOSE", R_STONE + 9, 1);
    bevel(f, 274, 8, 38, 46, 0);
    draw_face(f, 277, 11, 0);
    const Gear *g = &RUN.gear;
    for (int r = 0; r < INV_SLOTS; r++) {
        int y = 40 + r * 18, sel = r == G.ui_sel, id = inv_worn(r);
        char nm[40], label[16];
        if (r == INV_ROBE) { snprintf(label, sizeof label, "ROBE"); gear_item_name(GK_ROBE, id, g->robe_tier, nm, sizeof nm); }
        else if (r == INV_HOOD) { snprintf(label, sizeof label, "HOOD"); gear_item_name(GK_HOOD, id, g->hood_tier, nm, sizeof nm); }
        else {
            snprintf(label, sizeof label, "%s FINGER", FINGER_NAME[r - INV_F0]);
            if (id < 0) snprintf(nm, sizeof nm, "BARE");
            else gear_item_name(GK_RING, id, g->ring_own[id], nm, sizeof nm);
        }
        int col = r == INV_ROBE ? gear_robe_ramp(id, g->robe_tier) : (r == INV_HOOD ? gear_hood_ramp(id, g->hood_tier) : (id >= 0 ? gear_color(GK_RING, id) : R_STONE));
        if (sel) gfx_rect(f, 10, y - 2, 188, 18, R_IRON + 3);
        gfx_text8(f, 22, y, label, sel ? (uint8_t)C_WHITE : (uint8_t)(R_STONE + 10), 1);
        gfx_text8(f, 22, y + 8, nm, id < 0 && r >= INV_F0 ? (uint8_t)(R_STONE + 8) : ramp_at(col, 13), 1);
        if (sel) {
            int opt[8];
            if (inv_options(r, opt) > 1) { gfx_glyph8(f, 12, y + 8, '<', R_GOLD + 12, -1, 1); gfx_glyph8(f, 22 + gfx_text_width(nm, 1) + 4, y + 8, '>', R_GOLD + 12, -1, 1); }
        }
    }
    /* how it adds up, against how it was when the bag was opened */
    const Gear *g0 = &G.inv0;
    struct { const char *fmt; float v, v0; } ST[] = {
        { "ARMOR %.0f", (float)gear_armor(g), (float)gear_armor(g0) },
        { "FIRE WARD %.0f%%", gear_magic_resist(g) * 100.0f, gear_magic_resist(g0) * 100.0f },
        { "DODGE %.0f%%", gear_dodge(g) * 100.0f, gear_dodge(g0) * 100.0f },
        { "THORNS %.0f", (float)gear_thorns(g), (float)gear_thorns(g0) },
        { "MENDS %.2f/S", gear_regen(g), gear_regen(g0) },
        { "VENOM WARD %.0f%%", gear_poison_resist(g) * 100.0f, gear_poison_resist(g0) * 100.0f },
        { "MAX HEALTH %.0f", (float)gear_max_hp(g), (float)gear_max_hp(g0) },
        { "SOULS X%.2f", gear_soul_mult(g), gear_soul_mult(g0) },
        { "SPEED X%.2f", gear_haste(g), gear_haste(g0) },
        { "SPELLS X%.2f", gear_ember(g), gear_ember(g0) },
        { "GOLD SENSE %.0f", (float)gear_seek(g), (float)gear_seek(g0) },
    };
    for (int i = 0; i < (int)(sizeof ST / sizeof ST[0]); i++) {
        char line[32];
        snprintf(line, sizeof line, ST[i].fmt, ST[i].v);
        int better = ST[i].v > ST[i].v0 + 1e-4f, worse = ST[i].v < ST[i].v0 - 1e-4f;
        gfx_text8(f, 206, 58 + i * 8, line, better ? (uint8_t)(R_MOSS + 13) : (worse ? (uint8_t)(R_BLOOD + 13) : (uint8_t)(R_STONE + 12)), 1);
    }
    /* the spellbook: number keys for the attack spells, the runes as they stand */
    int y = 154, x = 14;
    for (int i = 10; i < W - 10; i++) gfx_px(f, i, 149, R_STONE + 5);
    gfx_text8(f, x, y, "SPELLS", R_GOLD + 10, 1);
    x += 56;
    for (int sp = 0; sp < NATK; sp++) {
        if (g->atk[sp] < 0) continue;
        char e[16]; snprintf(e, sizeof e, "%d %s", sp + 1, ATK_SHORT[sp]);
        int w = gfx_text_width(e, 1);
        if (x + w > W - 12) { x = 70; y += 9; }
        gfx_text8(f, x, y, e, sp == G.spell ? (uint8_t)(R_GOLD + 12) : (uint8_t)(R_STONE + 11), 1);
        x += w + 10;
    }
    y += 11; x = 14;
    gfx_text8(f, x, y, "RUNES", R_GOLD + 10, 1);
    x += 56;
    static const char *const DEF_SHORT[NDEF] = { "WARD", "BLINK", "MIRROR", "MEND", "PURIFY", "NOVA", "DREAD", "SLIP", "REAP" };
    int any = 0;
    for (int k = 0; k < NDEF; k++) {
        if (g->def[k] <= 0) continue;
        any = 1;
        int w = gfx_text_width(DEF_SHORT[k], 1);
        if (x + w > W - 12) { x = 70; y += 9; }
        gfx_text8(f, x, y, DEF_SHORT[k], G.def_cd[k] > 0.0f ? (uint8_t)(R_STONE + 8) : (uint8_t)(R_GLOW + 11), 1);
        x += w + 10;
    }
    if (!any) gfx_text8(f, x, y, "NONE YET", R_STONE + 8, 1);
}

/* Esc: the crypt makes its case for staying */
static void render_quit(Frame *f) {
    static const char *const LINE[] = {
        "THE MERCHANT WILL MISS YOU. THE MERCHANT WILL NEVER SAY SO.",
        "LEAVING? THE DEAD HAVE BEEN SO PATIENT WITH YOU.",
        "YOU STILL HAVE %d GOLD. IT SPENDS NOTHING UP THERE.",
        "OUT THERE IT IS JUST EMAIL. DOWN HERE YOU ARE A WIZARD.",
        "THE SKELETONS WERE JUST STARTING TO LIKE YOU.",
        "THE TORCHES WILL GO OUT WITHOUT YOU. ALL OF THEM.",
        "THE CAT IS WATCHING. THE CAT IS ALWAYS WATCHING.",
        "ONE MORE LEVEL. EVERYONE SAYS ONE MORE LEVEL.",
        "THE WRAITHS WILL TELL EVERYONE YOU RAN.",
        "YOUR DESK IS THE REAL TOMB, YOU KNOW.",
        "SOMEWHERE ON LEVEL %d A KEY IS STILL WAITING FOR YOU.",
        "WHO WILL PET THE MERCHANT WHILE YOU ARE GONE?",
        "THE SPIDERS SAY STAY AS LONG AS YOU LIKE. THE SPIDERS INSIST.",
        "GO, THEN. THE CRYPT WILL KEEP YOUR PLACE. AND YOUR NAME.",
        "REAL WIZARDS DO NOT HAVE MEETINGS.",
        "%d SOULS. SO CLOSE TO SOMETHING WONDERFUL.",
    };
    enum { NLINE = (int)(sizeof LINE / sizeof LINE[0]) };
    dim_view(f);
    int bw = 280, bh = 176, bx = (W - bw) / 2, by = (H - bh) / 2 - 6;
    panel(f, bx, by, bw, bh);
    gfx_text16(f, (W - gfx_text_width("LEAVE THE CRYPT?", 1)) / 2, by + 8, "LEAVE THE CRYPT?", R_BLOOD + 13, 1);
    char msg[96];
    int li = G.quit_line % NLINE, arg = li == 2 ? RUN.treasure : (li == 10 ? G.level : RUN.souls);
    snprintf(msg, sizeof msg, LINE[li], arg);
    int y = by + 30;                                               /* wrapped at word ends, 32 to a line */
    for (const char *p = msg; *p && y < by + 60; y += 10) {
        int n = (int)strlen(p);
        if (n > 32) { n = 32; while (n > 0 && p[n] != ' ') n--; if (n == 0) n = 32; }
        char ln[40]; memcpy(ln, p, (size_t)n); ln[n] = 0;
        gfx_text8(f, (W - gfx_text_width(ln, 1)) / 2, y, ln, R_STONE + 13, 1);
        p += n; while (*p == ' ') p++;
    }
    static const char *const OPT[Q_OPTIONS] = { "FLEE TO THE SURFACE", "STAY IN THE DARK", "SLEEPWALK", "CARVE A GRAVESTONE", "RISE FROM A GRAVE" };
    static const char *const NOTE[Q_OPTIONS] = { "YOUR LEVEL, GOLD AND GEAR ARE KEPT", "BACK TO THE CRYPT", "THE AUTOPILOT WALKS ON WITHOUT YOU",
                                                 "SAVE THIS RUN IN ONE OF 3 SLOTS", "LOAD A RUN FROM ONE OF 3 SLOTS" };
    for (int i = 10 + bx; i < bx + bw - 10; i += 2) gfx_px(f, i, by + 120, R_STONE + 5);   /* leaving, then the graves */
    for (int i = 0; i < Q_OPTIONS; i++) {
        int oy = by + 66 + i * 18 + (i >= Q_CARVE ? 4 : 0), sel = i == G.ui_sel;
        int w = gfx_text_width(OPT[i], 1), ox = (W - w) / 2;
        gfx_text16(f, ox, oy, OPT[i], sel ? (uint8_t)C_WHITE : (uint8_t)(R_STONE + 9), 1);
        if (sel) {                                                 /* a candle flame for a cursor */
            int fl = (int)(G.t * 10.0) & 1;
            for (int k = 0; k < 2; k++) {
                int cx = k ? ox + w + 8 : ox - 12;
                for (int yy = 0; yy < 9; yy++)
                    for (int xx = -2; xx <= 2; xx++)
                        if (abs(xx) * 3 <= yy + fl) gfx_px(f, cx + xx, oy + 3 + yy, (uint8_t)(R_FLAME + 15 - yy));
            }
        }
    }
    const char *note = NOTE[clampi(G.ui_sel, 0, Q_OPTIONS - 1)];
    gfx_text8(f, (W - gfx_text_width(note, 1)) / 2, by + bh - 14, note, R_GOLD + 9, 1);
}

/* a headstone, 22 wide and 26 tall, its numeral cut into it */
static void headstone(Frame *f, int x, int y, int i, int used) {
    int w = 22, h = 26, r = w / 2;
    for (int yy = 0; yy < h; yy++)
        for (int xx = 0; xx < w; xx++) {
            float dx = xx + 0.5f - r, dy = yy + 0.5f - r;
            if (yy < r && dx * dx + dy * dy > r * r) continue;       /* the rounded top */
            int edge = xx == 0 || xx == w - 1 || yy == h - 1 || (yy < r && dx * dx + dy * dy > (r - 1.2f) * (r - 1.2f));
            int k = used ? (edge ? 6 : 10 - (xx + yy) % 3 / 2) : (edge ? 4 : 6);
            gfx_px(f, x + xx, y + yy, (uint8_t)(R_STONE + k));
        }
    for (int xx = -3; xx < w + 3; xx++) gfx_px(f, x + xx, y + h, R_STONE + 3);   /* the ground it stands in */
    int tw = gfx_text_width(ROMAN[i], 1);
    gfx_text8(f, x + (w - tw) / 2, y + 9, ROMAN[i], used ? R_STONE + 3 : R_STONE + 2, 1);
}

/* CARVE A GRAVESTONE / RISE FROM A GRAVE: the three save slots */
static void render_graves(Frame *f) {
    static const char *const MONTH[12] = { "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC" };
    dim_view(f);
    int bw = 288, bh = 150, bx = (W - bw) / 2, by = (H - bh) / 2 - 6;
    panel(f, bx, by, bw, bh);
    const char *title = G.graves_mode ? "RISE FROM A GRAVE" : "CARVE A GRAVESTONE";
    gfx_text16(f, (W - gfx_text_width(title, 1)) / 2, by + 8, title, G.graves_mode ? R_BLOOD + 13 : R_GOLD + 12, 1);
    for (int i = 0; i < SLOT_COUNT; i++) {
        int y = by + 30 + i * 34, sel = i == G.ui_sel;
        Grave g = grave_info(i);
        if (sel) gfx_rect(f, bx + 6, y - 3, bw - 12, 33, R_IRON + 3);
        headstone(f, bx + 14, y, i, g.used);
        char l1[48], l2[48];
        if (g.used) {
            snprintf(l1, sizeof l1, "LEVEL %d  $%d  %d SOULS", g.level, g.gold, g.souls);
            time_t when = (time_t)g.when;
            struct tm *tm = g.when > 0 ? localtime(&when) : NULL;
            if (tm) snprintf(l2, sizeof l2, "%d %s %d  %02d:%02d  HP %d", tm->tm_mday, MONTH[tm->tm_mon % 12], tm->tm_year + 1900, tm->tm_hour, tm->tm_min, g.hp);
            else snprintf(l2, sizeof l2, "HP %d", g.hp);
        } else {
            snprintf(l1, sizeof l1, "UNMARKED");
            snprintf(l2, sizeof l2, "AN EMPTY GRAVE");
        }
        gfx_text8(f, bx + 46, y + 5, l1, sel ? (uint8_t)C_WHITE : (uint8_t)(g.used ? R_STONE + 12 : R_STONE + 9), 1);
        gfx_text8(f, bx + 46, y + 16, l2, R_STONE + 9, 1);
    }
    const char *note = G.graves_mode ? "ENTER RISES   ESC GOES BACK" : "ENTER CARVES   ESC GOES BACK";
    int col = R_GOLD + 9;
    if (G.graves_msg == GM_EMPTY) { note = "NOTHING LIES HERE"; col = R_STONE + 10; }
    else if (G.graves_msg == GM_ARMED && G.graves_arm == G.ui_sel) {
        note = G.graves_mode ? "ENTER AGAIN. THIS RUN WILL BE LOST" : "ENTER AGAIN TO CARVE OVER IT";
        col = R_BLOOD + 13;
    }
    gfx_text8(f, (W - gfx_text_width(note, 1)) / 2, by + bh - 14, note, col, 1);
}

static void render_ui(Frame *f) {
    if (G.ui == UI_INVENTORY) render_inventory(f);
    else if (G.ui == UI_QUIT) render_quit(f);
    else if (G.ui == UI_GRAVES) render_graves(f);
}

static void crawl_render(Frame *f) {
    f->w = W; f->h = H;
    memcpy(f->pal, crawl_pal, sizeof crawl_pal);
    f->fade = G.fade;
    if (G.tally) { render_tally(f); if (G.player && G.ui) render_ui(f); return; }
    g_camfog = fog_at(G.px, G.py);
    float ang = G.ang + G.kick;
    float dx = cosf(ang), dy = sinf(ang);
    float plx = -dy * PLANE_LEN, ply = dx * PLANE_LEN;
    float horizon = H / 2.0f + G.hurt * 3.0f;
    float camz = G.camz;
    G.camz -= 0.03f * G.hurt;
    render_floor(f, dx, dy, plx, ply, horizon);
    render_walls(f, dx, dy, plx, ply, horizon);
    render_sprites(f, dx, dy, horizon);
    render_beams(f, dx, dy, horizon);
    G.camz = camz;
    if (G.shop) render_shop_tags(f, dx, dy, horizon);
    if (!G.dead) { render_wards(f); render_hand(f); }
    if (G.shop) render_shop_card(f);
    if (G.shop || G.player) render_statusbar(f);                    /* a person gets the Wolfenstein bar */
    else { render_readout(f); render_face_popup(f); }
    if (!G.shop) render_card(f);
    render_messages(f);
    if (G.player && G.ui) render_ui(f);
    tint(f);
}

const Scene scene_crawl = { "CATACOMB", W, H, SND_PROG_CRAWL, crawl_enter, crawl_update, crawl_render,
                            crawl_control, crawl_status, crawl_input, crawl_save, crawl_load };
