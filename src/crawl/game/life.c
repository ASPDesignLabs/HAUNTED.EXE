/* A part of scene_crawl.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The crypt's small life (docs/PLAN-ambience.md): the list, its dice, the settings' gates, where it may be; the kinds are added below as
 * they are built. None of it is saved, hurt, counted or in the way, and none of it draws on G.rs: before it is switched on, the dream's
 * log is the build before's, line for line. */

extern int g_life_move, g_life_sound;                              /* core/app.h */
#define LIFE_SPARE __attribute__((unused))                         /* until each kind that uses it is built */
LIFE_SPARE static int lrnd(int n) { return (int)(rng_next(&G.lrs) % (uint32_t)(n > 0 ? n : 1)); }
LIFE_SPARE static float lrndf(void) { return rng_f(&G.lrs); }

/* the owner's settings (core/app.h): movement and sound, 0 off, 1 reduced (what keeps still), 2 on */
static int life_move(void) { return g_life_move; }
LIFE_SPARE static int life_sound(void) { return g_life_sound; }

static int life_off(void) {                                        /* CRAWL_LIFE=0: none, for the proof and for the frames before */
#ifdef CRAWL_DEBUG
    if (getenv("CRAWL_LIFE") && atoi(getenv("CRAWL_LIFE")) == 0) return 1;
#endif
    return 0;
}

/* a level it may be on (not the shop, nor a pocket: the Backrooms, Severance, her rooms), and a cell it may begin in (the mice's: not a
 * lesson's trigger nor a trial's floor) */
static int life_level_ok(void) { return !G.shop && !PK.depth && G.faced; }
LIFE_SPARE static int life_cell_ok(int x, int y) { return mo_cell_ok(x, y); }

LIFE_SPARE static int life_add(int kind, float x, float y, float z) {         /* a new one, or -1 when the list is full */
    for (int i = 0; i < MAXLIFE; i++)
        if (!G.life[i].alive) {
            Life *o = &G.life[i];
            memset(o, 0, sizeof *o);
            o->alive = 1; o->kind = kind; o->x = x; o->y = y; o->z = z; o->ref = -1;
            if (i >= G.nlife) G.nlife = i + 1;
            return i;
        }
    return -1;
}

static void life_level(void) {
    G.nlife = 0; memset(G.life, 0, sizeof G.life);
    G.lrs = ((G.rs ^ 0x1F3D5Bu) * 2654435769u + (uint32_t)G.level * 40503u) | 1u;   /* its own dice: G.rs read, never drawn on */
    if (life_off() || !life_level_ok()) return;
    /* the kinds are made here as each is built (docs/PLAN-ambience.md, steps 3 to 6) */
}

static void life_update(float dt) {
    if (!G.nlife || !life_level_ok() || life_move() == 0) return;
    for (int i = 0; i < G.nlife; i++) {
        Life *o = &G.life[i];
        if (!o->alive) continue;
        o->t += dt;
    }
}
