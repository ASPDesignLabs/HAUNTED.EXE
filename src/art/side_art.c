/* The side passages' art (docs/PLAN-side.md): tiles and the caster's figure, a draft. Everything is made here in CATACOMB's
 * palette (crawl.h's ramps), as the rest of the art is: the tiles in run I's stone, the figure from a rig of a few joints a frame,
 * each pixel kept with the part it is (side_fig_part) so the gear worn can colour it later. */
#include "side_art.h"
#include "crawl.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

uint8_t side_tile[NST][STS * STS];
uint8_t side_fig[NSF][SF_W * SF_H];
uint8_t side_fig_part[NSF][SF_W * SF_H];
const char *const SIDE_FRAME_NAME[NSF] = {
    "IDLE", "IDLE", "WALK", "WALK", "WALK", "WALK", "WALK", "WALK", "RUN", "RUN", "RUN", "RUN", "RUN", "RUN",
    "RISE", "APEX", "FALL", "CROUCH", "CREEP", "CREEP", "CAST", "CAST", "HURT", "DIE", "DIE", "DIE", "DIE", "DIE",
};

static uint32_t hs(int a, int b, int c) {
    uint32_t h = (uint32_t)a * 374761393u + (uint32_t)b * 668265263u + (uint32_t)c * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}
static uint8_t rc(int ramp, int k) { int top = ramp == R_GOLD ? NGOLD - 1 : 15; return (uint8_t)(ramp + (k < 0 ? 0 : k > top ? top : k)); }
static void tp(uint8_t *t, int x, int y, uint8_t c) { if (x >= 0 && y >= 0 && x < STS && y < STS) t[y * STS + x] = c; }
static void tclear(uint8_t *t) { memset(t, C_CLEAR, STS * STS); }

/* ---- the tiles ---- */

/* courses of stone: blocks in a running bond, lit from above and the left, mortar between */
static void stone(uint8_t *t, int ramp, int base, int course, int blockw, int look, int seed) {
    for (int y = 0; y < STS; y++)
        for (int x = 0; x < STS; x++) {
            int row = y / course, ry = y % course;
            int off = (row & 1) ? blockw / 2 + look * 3 : look * 5;
            int bx = (x + off) % blockw, bid = (x + off) / blockw + row * 7;
            uint8_t c;
            if (ry == course - 1 || bx == blockw - 1) c = rc(ramp, base - 5);   /* mortar */
            else {
                int k = base + (int)(hs(bid, row, seed) % 3) - 1 + (int)(hs(x, y, seed) % 5 == 0) - (int)(hs(y, x, seed + 9) % 7 == 0);
                if (ry == 0) k += 2; else if (ry == course - 2) k -= 2;
                if (bx == 0) k += 1; else if (bx == blockw - 2) k -= 1;
                c = rc(ramp, k);
            }
            t[y * STS + x] = c;
        }
}
static void lip(uint8_t *t, int seed) {                            /* dust and moss on a top edge */
    for (int x = 0; x < STS; x++) {
        int d = 1 + (int)(hs(x, 3, seed) % 3);
        for (int y = 0; y < d; y++) tp(t, x, y, y == 0 ? rc(R_MOSS, 9 + (int)(hs(x, y, seed) % 3)) : rc(R_MOSS, 6));
        if (hs(x, 7, seed) % 5 == 0) tp(t, x, d, rc(R_MOSS, 5));
    }
}
static void brick(uint8_t *t, int seed) {
    for (int y = 0; y < STS; y++)
        for (int x = 0; x < STS; x++) {
            int row = y / 4, ry = y % 4, off = (row & 1) * 4, bx = (x + off) % 8, bid = (x + off) / 8 + row * 5;
            uint8_t c;
            if (ry == 3 || bx == 7) c = rc(R_WARM, 4);
            else {
                int k = 8 + (int)(hs(bid, row, seed) % 3) - 1 + (ry == 0) - (ry == 2);
                if (hs(x, y, seed) % 9 == 0) k -= 2;
                c = rc(R_BRICK, k);
            }
            t[y * STS + x] = c;
        }
}
static void ledge(uint8_t *t) {
    tclear(t);
    for (int x = 0; x < STS; x++)
        for (int y = 1; y <= 5; y++) {
            int k = y == 1 ? 12 : y == 5 ? 5 : 8 + (int)(hs(x / 3, y, 4) % 2) - (hs(x, y, 5) % 6 == 0);
            tp(t, x, y, rc(R_WOOD, k));
        }
    for (int x = 0; x < STS; x += 8) tp(t, x + 3, 3, rc(R_IRON, 12));   /* the nails */
    for (int s = 0; s < 2; s++) {                                  /* the corbels under it */
        int x0 = s ? 11 : 2;
        for (int y = 6; y <= 9; y++)                               /* an iron bracket into the wall behind */
            for (int x = x0; x < x0 + 4 - (y - 6); x++) tp(t, x, y, rc(R_IRON, y == 6 ? 10 : x == x0 ? 8 : 5));
    }
}
static void spikes(uint8_t *t) {
    tclear(t);
    for (int s = 0; s < 4; s++) {
        int cx = s * 4 + 2;
        for (int y = 6; y < STS; y++) {
            int half = (y - 6) * 2 / 9;
            for (int x = cx - half - 1; x <= cx + half; x++) tp(t, x, y, rc(R_IRON, x < cx ? 11 : x == cx ? 13 : 6));
        }
        tp(t, cx, 6, rc(R_IRON, 15));
    }
    tp(t, 6, 7, rc(R_BLOOD, 9)); tp(t, 6, 8, rc(R_BLOOD, 7)); tp(t, 7, 9, rc(R_BLOOD, 6));
    for (int x = 0; x < STS; x++) tp(t, x, 15, rc(R_IRON, 3));
}
static void water(uint8_t *t, int frame, int deep) {
    tclear(t);
    for (int x = 0; x < STS; x++) {
        float w = deep ? -9.0f : 3.0f + 1.3f * sinf((float)(x + frame * 4) * 6.2832f / 16.0f) + 0.5f * sinf((float)(x * 3 - frame * 7) * 0.55f);
        int top = (int)floorf(w + 0.5f);
        for (int y = top < 0 ? 0 : top; y < STS; y++) {
            int k = y == top ? 14 - (int)(hs(x, frame, 12) % 3) : y == top + 1 ? 9 : 5 - (y > 9) - (y > 13) + (hs(x, y + frame, 11) % 9 == 0);
            tp(t, x, y, y == top && hs(x, frame, 13) % 4 == 0 ? rc(R_BONE, 13) : rc(R_MOSS, k));
        }
    }
    for (int b = 0; b < 3; b++) {                                  /* bubbles, rising a pixel or two a frame */
        int bx = (int)(hs(b, 1, 21) % 14) + 1, by = 14 - (int)((hs(b, 2, 21) % 9 + (uint32_t)frame * (2 + b)) % 10);
        if (!deep && by < 6) continue;
        tp(t, bx, by, rc(R_MOSS, 11)); tp(t, bx, by - 1, rc(R_MOSS, 8));
    }
}
static void fall_block(uint8_t *t, int cracked) {
    for (int y = 0; y < STS; y++)
        for (int x = 0; x < STS; x++) {
            int k = 8 + (int)(hs(x, y, 31) % 3) - 1;
            if (y == 0 || x == 0) k = 12; else if (y == 15 || x == 15) k = 4; else if (y == 1 || x == 1) k = 10; else if (y == 14 || x == 14) k = 6;
            t[y * STS + x] = rc(R_WARM, k);
        }
    static const int8_t crack[][2] = { { 5, 2 }, { 6, 3 }, { 6, 4 }, { 7, 5 }, { 8, 6 }, { 8, 7 }, { 7, 8 }, { 9, 8 }, { 10, 9 } };
    static const int8_t more[][2] = { { 11, 3 }, { 11, 4 }, { 10, 5 }, { 3, 10 }, { 4, 11 }, { 5, 11 }, { 6, 12 }, { 10, 10 }, { 11, 11 }, { 12, 13 } };
    for (unsigned i = 0; i < sizeof crack / sizeof crack[0]; i++) tp(t, crack[i][0], crack[i][1], rc(R_WARM, 2));
    if (cracked) for (unsigned i = 0; i < sizeof more / sizeof more[0]; i++) tp(t, more[i][0], more[i][1], rc(R_WARM, 2));
    for (int x = 3; x < 13; x += 9) { tp(t, x, 3, rc(R_IRON, 12)); tp(t, x, 12, rc(R_IRON, 12)); }   /* the rivets of its iron straps */
}
static void back_wall(uint8_t *t, int look) {
    stone(t, R_STONE, 5, 8, 16, look, 40 + look);
}
static void back_niche(uint8_t *t) {
    back_wall(t, 0);
    for (int y = 3; y < 14; y++)
        for (int x = 4; x < 12; x++) {
            int dx = x * 2 - 15, dy = y - 6;
            if (y < 6 && dx * dx + dy * dy * 16 > 64) continue;
            tp(t, x, y, rc(R_STONE, y == 13 ? 6 : 1));
        }
    for (int y = 8; y < 13; y++) for (int x = 6; x < 10; x++) {    /* a skull in it */
        int dx = x * 2 - 15, dy = y * 2 - 20;
        if (dx * dx + dy * dy > 18) continue;
        tp(t, x, y, rc(R_BONE, 7 + (x < 8) * 2 - (y > 10)));
    }
    tp(t, 7, 10, rc(R_STONE, 0)); tp(t, 8, 10, rc(R_STONE, 0));
}
static void back_skulls(uint8_t *t) {
    back_wall(t, 1);
    for (int row = 0; row < 2; row++)
        for (int s = 0; s < 3; s++) {
            int cx = s * 5 + 3 + row * 2, cy = row * 8 + 4;
            for (int y = cy - 2; y <= cy + 2; y++)
                for (int x = cx - 2; x <= cx + 2; x++) {
                    if ((x - cx) * (x - cx) + (y - cy) * (y - cy) > 5) continue;
                    tp(t, x, y, rc(R_BONE, 6 + (x < cx) + (y < cy) - (y > cy)));
                }
            tp(t, cx - 1, cy, rc(R_STONE, 0)); tp(t, cx + 1, cy, rc(R_STONE, 0));
        }
}
static void far_wall(uint8_t *t) { stone(t, R_STONE, 3, 16, 16, 0, 77); }
static void plat(uint8_t *t, int part) {
    tclear(t);
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < STS; x++) {
            int k = y == 0 ? 12 : y == 7 ? 4 : y == 1 ? 10 : 8 + (int)(hs(x, y, 51 + part) % 3) - 1;
            if (part == 0 && x == 0) k = 11;
            if (part == 2 && x == 15) k = 5;
            tp(t, x, y, rc(R_STONE, k));
        }
    for (int x = 0; x < STS; x++) { tp(t, x, 3, rc(R_IRON, 7)); tp(t, x, 4, rc(R_IRON, 4)); }   /* an iron band round it */
    if (part != 1) {                                               /* the ring a chain hangs it from */
        int cx = part == 0 ? 3 : 12;
        tp(t, cx, 0, rc(R_IRON, 13)); tp(t, cx - 1, 1, rc(R_IRON, 10)); tp(t, cx + 1, 1, rc(R_IRON, 8));
    }
}
static void chain(uint8_t *t) {
    tclear(t);
    for (int y = 0; y < STS; y++) {
        int link = (y / 4) & 1, ry = y % 4;
        if (link) { tp(t, 7, y, rc(R_IRON, 11)); tp(t, 8, y, rc(R_IRON, 7)); }
        else if (ry == 0 || ry == 3) { tp(t, 7, y, rc(R_IRON, 10)); tp(t, 8, y, rc(R_IRON, 8)); }
        else { tp(t, 6, y, rc(R_IRON, 11)); tp(t, 9, y, rc(R_IRON, 6)); }
    }
}
static void doorway(uint8_t *top, uint8_t *bot, int goal) {        /* a round arch two tiles high, a light going away inside it */
    uint8_t d[STS * 2 * STS];
    for (int y = 0; y < 2 * STS; y++)
        for (int x = 0; x < STS; x++) {
            float ax = (float)x - 7.5f, ay = (float)y - 7.5f;
            float r = y < 8 ? sqrtf(ax * ax + ay * ay) : fabsf(ax);    /* the arch above, straight jambs below */
            uint8_t c;
            if (r > 7.6f) c = C_CLEAR;
            else if (r > 5.4f) {                                    /* the frame's stones */
                int vous = y < 8 ? (int)((atan2f(ay, ax) + 3.1416f) * 2.2f) : y / 4;
                int k = 10 + (int)(hs(vous, goal, 61) % 2) + (ax < 0.0f) - (r > 7.0f) * 3;
                if (y >= 8 && y % 4 == 3) k = 4;
                c = rc(R_STONE, k);
                if (goal && r > 6.7f) c = rc(R_GOLD, 7 + (ax < 0.0f) * 3);
            } else {                                                /* the way going in: frames into the light */
                float rr = y < 8 ? r : fmaxf(fabsf(ax), (31.0f - (float)y) * 0.3f);
                int ring = (int)(rr * 1.1f);
                int k = 9 - ring - ((int)rr % 2 == 0);
                c = k < 1 ? rc(R_STONE, 0) : rc(R_WARM, k);
            }
            d[y * STS + x] = c;
        }
    if (goal) {                                                    /* the keystone: a skull in gold */
        for (int y = 0; y < 3; y++) for (int x = 6; x < 10; x++) d[y * STS + x] = rc(R_GOLD, 10 - y * 2 + (x < 8));
        d[1 * STS + 7] = d[1 * STS + 8] = rc(R_STONE, 0);
    }
    for (int x = 0; x < STS; x++) d[31 * STS + x] = rc(R_STONE, 6);   /* the threshold */
    memcpy(top, d, STS * STS); memcpy(bot, d + STS * STS, STS * STS);
}
static void door_lit(uint8_t *t) {                                 /* the light inside, over the dark, for the flicker */
    tclear(t);
    for (int y = 0; y < STS; y++) for (int x = 4; x < 12; x++) {
        float ax = (float)x - 7.5f;
        if (fabsf(ax) < 2.5f - (float)y * 0.08f) tp(t, x, y, rc(R_FLAME, 2 + (fabsf(ax) < 1.0f) * 2));
    }
}
static void lever(uint8_t *t, int on) {
    tclear(t);
    for (int y = 12; y < STS; y++) for (int x = 4; x < 12; x++) tp(t, x, y, rc(R_IRON, y == 12 ? 11 : y == 15 ? 3 : 7 - (x > 9)));
    float a = on ? 0.6f : -0.6f;
    for (int i = 0; i < 9; i++) {
        int x = (int)floorf(8.0f + sinf(a) * (float)i + 0.5f), y = 12 - (int)floorf(cosf(a) * (float)i + 0.5f);
        tp(t, x, y, rc(R_WOOD, 9)); tp(t, x + 1, y, rc(R_WOOD, 6));
    }
    int kx = (int)floorf(8.0f + sinf(a) * 9.0f + 0.5f), ky = 12 - (int)floorf(cosf(a) * 9.0f + 0.5f);
    for (int dy = -1; dy <= 1; dy++) for (int dx = -1; dx <= 1; dx++)
        if (dx * dx + dy * dy <= 1) tp(t, kx + dx, ky + dy, on ? rc(R_MOSS, 12 - dy - dx) : rc(R_BLOOD, 11 - dy - dx));
}
static void key(uint8_t *t, int colour) {
    tclear(t);
    int ramp = colour == 0 ? R_GOLD : colour == 1 ? R_IRON : R_BLOOD, hi = colour == 0 ? 11 : 13;
    for (int y = 5; y <= 11; y++) for (int x = 2; x <= 8; x++) {   /* the bow, a ring */
        int dx = x - 5, dy = y - 8, r = dx * dx + dy * dy;
        if (r > 10 || r < 3) continue;
        tp(t, x, y, rc(ramp, hi - (dy > 0) * 3 - (dx > 0) * 2));
    }
    for (int x = 9; x <= 14; x++) { tp(t, x, 7, rc(ramp, hi)); tp(t, x, 8, rc(ramp, hi - 4)); }
    tp(t, 13, 9, rc(ramp, hi - 3)); tp(t, 13, 10, rc(ramp, hi - 5)); tp(t, 11, 9, rc(ramp, hi - 3));
}
static void life(uint8_t *t) {                                     /* a votive lamp: a clay bowl, its flame */
    tclear(t);
    for (int y = 11; y <= 14; y++) for (int x = 3; x <= 12; x++) {
        int w = 5 - (y - 11);
        if (abs(x * 2 - 15) > w * 2 + 1) continue;
        tp(t, x, y, rc(R_WARM, y == 11 ? 12 : 9 - (y - 11) * 2 + (x < 8)));
    }
    for (int y = 3; y <= 10; y++) for (int x = 6; x <= 9; x++) {
        float ax = (float)x - 7.5f, h = (float)(10 - y) / 7.0f;
        if (fabsf(ax) > 1.9f * (1.0f - h * h)) continue;
        tp(t, x, y, rc(R_FLAME, h < 0.3f ? 15 : h < 0.6f ? 11 : 7));
    }
}
static void torch(uint8_t *t, int frame) {
    tclear(t);
    for (int y = 9; y <= 14; y++) tp(t, 7, y, rc(R_IRON, 9)), tp(t, 8, y, rc(R_IRON, 5));
    for (int x = 5; x <= 10; x++) tp(t, x, 9, rc(R_IRON, 11));
    for (int y = 1; y <= 8; y++) for (int x = 4; x <= 11; x++) {
        float ax = (float)x - 7.5f + 0.6f * sinf((float)(frame * 2 + y) * 1.3f), h = (float)(8 - y) / 8.0f;
        float wdt = 2.6f * (1.0f - h) * (0.8f + 0.2f * (float)((frame + y) % 3));
        if (fabsf(ax) > wdt) continue;
        tp(t, x, y, rc(R_FLAME, fabsf(ax) < wdt * 0.4f ? 15 - (int)(h * 4) : 9 - (int)(h * 5)));
    }
}

/* ---- the builder's kit (27 November): drafts, to go to the owner as a sheet ---- */
static void slate(uint8_t *t, int top) {                           /* thin dark slabs, wet: a sheen running down them */
    for (int y = 0; y < STS; y++)
        for (int x = 0; x < STS; x++) {
            int row = y / 4, ry = y % 4;
            uint8_t c = ry == 3 ? rc(R_STONE, 2) : rc(R_STONE, 6 + (int)(hs(x / 5 + row * 3, row, 91) % 3) - (ry == 2));
            if (ry < 3 && (x + y * 2 + (int)(hs(row, 0, 92) % 7)) % 11 == 0) c = rc(R_GHOST, 8 + (ry == 0) * 2);
            t[y * STS + x] = c;
        }
    if (top)
        for (int x = 0; x < STS; x++) {
            tp(t, x, 0, rc(R_GHOST, hs(x, 1, 93) % 3 == 0 ? 10 : 13));
            if (hs(x, 2, 94) % 5 == 0) { tp(t, x, 1, rc(R_GHOST, 10)); tp(t, x, 2, rc(R_GHOST, 7)); }
        }
}
static void web(uint8_t *t) {                                      /* strands out from a middle, and rings between them, broken here and there */
    tclear(t);
    for (int a = 0; a < 8; a++) {
        float an = (float)a * 0.7854f + 0.3f;
        for (float r = 0.0f; r < 12.0f; r += 0.5f) tp(t, (int)floorf(8.0f + cosf(an) * r), (int)floorf(8.0f + sinf(an) * r), rc(R_BONE, 12 - (int)(r / 6.0f)));
    }
    for (int ring = 0; ring < 3; ring++) {
        float rr = 2.5f + (float)ring * 2.8f;
        for (float an = 0.0f; an < 6.2832f; an += 0.06f)
            if (hs((int)(an * 10.0f), ring, 95) % 5) tp(t, (int)floorf(8.0f + cosf(an) * rr), (int)floorf(8.0f + sinf(an) * rr * 0.9f), rc(R_BONE, 10 - ring));
    }
}
static void crack(uint8_t *t) {                                    /* the stone, split through: dark seams and a chip or two loose */
    stone(t, R_STONE, 8, 8, 8, 1, 96);
    static const int8_t seam[][2] = { { 2, 0 }, { 3, 1 }, { 3, 2 }, { 4, 3 }, { 6, 4 }, { 7, 5 }, { 7, 6 }, { 6, 7 }, { 7, 8 }, { 9, 9 }, { 10, 10 },
                                      { 10, 11 }, { 12, 12 }, { 13, 13 }, { 13, 14 }, { 14, 15 }, { 8, 5 }, { 9, 4 }, { 11, 3 }, { 12, 2 }, { 7, 9 }, { 5, 10 }, { 4, 11 }, { 3, 13 } };
    for (unsigned i = 0; i < sizeof seam / sizeof seam[0]; i++) { tp(t, seam[i][0], seam[i][1], rc(R_STONE, 1)); tp(t, seam[i][0] + 1, seam[i][1], rc(R_STONE, 11)); }
    for (int k = 0; k < 3; k++) { int x = 2 + (int)(hs(k, 4, 97) % 12), y = 2 + (int)(hs(k, 5, 97) % 12); tp(t, x, y, rc(R_STONE, 3)); }
}
static void bell_stone(uint8_t *t, int set) {                      /* a dressed block, a bell cut in its face */
    int ramp = set ? R_PURPLE : R_GOLD;
    for (int y = 0; y < STS; y++)
        for (int x = 0; x < STS; x++) {
            int k = 9 + (int)(hs(x, y, 98 + set) % 3) - 1;
            if (y == 0 || x == 0) k = 13; else if (y == 15 || x == 15) k = 4; else if (y == 1 || x == 1) k = 11; else if (y == 14 || x == 14) k = 6;
            t[y * STS + x] = rc(R_STONE, k);
        }
    for (int y = 4; y <= 11; y++)                                  /* the bell: a dome, flared at the foot, its clapper under */
        for (int x = 4; x <= 11; x++) {
            float ax = (float)x - 7.5f, half = y < 6 ? 1.5f + (float)(y - 4) : y < 10 ? 2.6f + (float)(y - 6) * 0.25f : 3.6f;
            if (fabsf(ax) <= half) tp(t, x, y, set ? rc(ramp, 12 - (ax > 0.0f) * 3) : (uint8_t)(ramp + 9 - (ax > 0.0f) * 3));
        }
    tp(t, 7, 12, rc(R_STONE, 2)); tp(t, 8, 12, rc(R_STONE, 2));
    for (int x = 3; x <= 12; x++) tp(t, x, 3, rc(R_STONE, 5));    /* the cut's shadow */
}
static void ghost_stone(uint8_t *t) {                              /* pale stone, half there: a third of its pixels let the dark through */
    for (int y = 0; y < STS; y++)
        for (int x = 0; x < STS; x++) {
            int k = 9 + (int)(hs(x / 4, y / 4, 99) % 3) - 1;
            if (y % 8 == 7 || (x + (y / 8) * 4) % 8 == 7) k = 5;
            if (y == 0 || x == 0) k = 13;
            t[y * STS + x] = hs(x, y, 100) % 3 == 0 && y > 0 && x > 0 ? C_CLEAR : rc(R_GHOST, k);
        }
}
static void belt(uint8_t *t, int frame) {                          /* links of bone along the top, over an iron frame with its rollers */
    tclear(t);
    for (int x = 0; x < STS; x++) {
        int along = (x + frame * 2) % 8;
        for (int y = 0; y < 4; y++) {
            int k = along == 7 ? 4 : y == 0 ? 13 : y == 3 ? 6 : 10 - (along == 0);
            tp(t, x, y, rc(R_BONE, k));
        }
        if (along == 3) tp(t, x, 1, rc(R_BONE, 7));                /* a knuckle's mark */
    }
    for (int y = 4; y < STS; y++) for (int x = 0; x < STS; x++) tp(t, x, y, rc(R_IRON, y == 4 ? 9 : y == 15 ? 2 : 4 + (int)(hs(x, y, 101) % 2)));
    for (int r = 0; r < 2; r++) {                                  /* the rollers, their spokes turning */
        int cx = r ? 12 : 4, cy = 9;
        for (int y = -3; y <= 3; y++) for (int x = -3; x <= 3; x++) if (x * x + y * y <= 10) tp(t, cx + x, cy + y, rc(R_IRON, x * x + y * y >= 7 ? 10 : 6));
        float an = (float)frame * 0.785f;
        for (int k = -2; k <= 2; k++) tp(t, cx + (int)lroundf(cosf(an) * (float)k), cy + (int)lroundf(sinf(an) * (float)k), rc(R_IRON, 13));
    }
}
static void press_head(uint8_t *t) {                               /* a block of stone in iron bands, its foot dark and worn */
    for (int y = 0; y < STS; y++)
        for (int x = 0; x < STS; x++) {
            int k = 9 + (int)(hs(x, y, 102) % 3) - 1;
            if (x == 0) k = 11; else if (x == 15) k = 5;
            uint8_t c = rc(R_STONE, k);
            if (y <= 1 || y == 8 || y == 9) c = rc(R_IRON, y == 0 || y == 8 ? 11 : 6);
            if (y >= 14) c = rc(R_STONE, y == 15 ? 2 : 4);
            t[y * STS + x] = c;
        }
    for (int x = 2; x < STS; x += 6) { tp(t, x, 1, rc(R_IRON, 14)); tp(t, x, 9, rc(R_IRON, 14)); }   /* rivets */
}
static void tomb(uint8_t *t, int half) {                           /* a lid over a carved chest; the left half a cross, the right a line of letters */
    tclear(t);
    for (int y = 2; y < STS; y++)
        for (int x = 0; x < STS; x++) {
            int edge = half ? x == 15 : x == 0, k;
            if (y <= 4) k = y == 2 ? 13 : y == 4 ? 6 : 11;         /* the lid, over the chest a pixel each side */
            else k = 8 + (int)(hs(x, y, 103 + half) % 2) - (y == 15) * 3;
            if (edge && y > 4) k -= 2;
            t[y * STS + x] = rc(R_STONE, k);
        }
    for (int x = half ? 0 : 1; x < (half ? 15 : 16); x++) tp(t, x, 1, rc(R_STONE, 10));
    if (!half) { for (int y = 6; y <= 13; y++) tp(t, 9, y, rc(R_STONE, 4)); for (int x = 6; x <= 12; x++) tp(t, x, 8, rc(R_STONE, 4)); }
    else for (int x = 2; x <= 12; x++) if (hs(x, 0, 104) % 3) { tp(t, x, 9, rc(R_STONE, 4)); if (hs(x, 1, 104) % 2) tp(t, x, 11, rc(R_STONE, 5)); }
}
static void springboard(uint8_t *t, int down) {                    /* a board on three ribs of bone over a wooden foot; thrown down, the ribs squashed */
    tclear(t);
    int top = down ? 10 : 6;
    for (int x = 1; x < 15; x++) { tp(t, x, 14, rc(R_WOOD, 8)); tp(t, x, 15, rc(R_WOOD, 4)); }
    for (int x = 2; x < 14; x++) { tp(t, x, top, rc(R_WOOD, 12)); tp(t, x, top + 1, rc(R_WOOD, 7)); }
    int span = 13 - (top + 2);
    for (int r = 0; r < 3; r++) {
        int y = top + 2 + (span * r) / 3 + 1;
        for (int x = 3; x < 13; x++) {
            float bow = 1.5f * sinf((float)(x - 3) / 9.0f * 3.1416f);
            tp(t, x, y + (int)lroundf(down ? bow * 0.4f : bow), rc(R_BONE, 12 - r));
        }
    }
}
static void bell_hung(uint8_t *t, int swing) {                     /* a bronze bell on an iron bracket from the wall; swung, it leans */
    tclear(t);
    for (int x = 3; x <= 12; x++) tp(t, x, 0, rc(R_IRON, 10));
    tp(t, 7, 1, rc(R_IRON, 12)); tp(t, 8, 1, rc(R_IRON, 8));
    int lean = swing == 1 ? -1 : swing == 2 ? 1 : 0;
    for (int y = 2; y <= 13; y++) {
        float half = y < 5 ? 1.5f + (float)(y - 2) * 0.7f : y < 11 ? 3.6f + (float)(y - 5) * 0.2f : 5.4f;
        int shift = lean * (y - 2) / 6;
        for (int x = 1; x <= 14; x++) {
            float ax = (float)x - 7.5f - (float)shift;
            if (fabsf(ax) > half) continue;
            int k = y == 11 ? 7 : 9 - (ax > 0.0f) * 3 + (fabsf(ax) < 1.0f) * 2 - (y > 11) * 3;
            tp(t, x, y, (uint8_t)(R_GOLD + (k < 0 ? 0 : k > 12 ? 12 : k)));
        }
    }
    tp(t, 7 + lean * 2, 14, rc(R_IRON, 11)); tp(t, 8 + lean * 2, 14, rc(R_IRON, 7)); tp(t, 7 + lean * 2, 15, rc(R_IRON, 9));   /* the clapper */
}
static void climb_chain(uint8_t *t) {                              /* heavy links, side on and face on in turn */
    tclear(t);
    for (int y = 0; y < STS; y++) {
        int link = (y / 4) & 1, ry = y % 4;
        if (link) { tp(t, 7, y, rc(R_IRON, 12)); tp(t, 8, y, rc(R_IRON, 8)); tp(t, 9, y, rc(R_IRON, 5)); }
        else if (ry == 0 || ry == 3) for (int x = 6; x <= 9; x++) tp(t, x, y, rc(R_IRON, 11 - (x > 7) * 3));
        else { tp(t, 5, y, rc(R_IRON, 12)); tp(t, 6, y, rc(R_IRON, 9)); tp(t, 9, y, rc(R_IRON, 7)); tp(t, 10, y, rc(R_IRON, 4)); }
    }
}

/* ---- vermin (docs/PLAN-vermin.md): drawn a pixel at a time (the second pass, 28 November), facing right, feet on the tile's foot.
 * The letters: k d m l h the fur dark to light; b its belly; s S the skin (ears, feet, tail), its inside pale; n the nose; e an eye
 * (lit); w a whisker; i j J the spider's shell dark to light, I a pale band at a joint, r its mother's mark, E its lesser eyes, f fangs */
static void vermin_grid(uint8_t *t, const char *const *g) {
    tclear(t);
    for (int y = 0; y < STS; y++)
        for (int x = 0; x < STS; x++) {
            int c;
            switch (g[y][x]) {
            case 'k': c = rc(R_WARM, 2); break;  case 'd': c = rc(R_WARM, 5); break;  case 'm': c = rc(R_WARM, 8); break;
            case 'l': c = rc(R_WARM, 10); break; case 'h': c = rc(R_WARM, 12); break; case 'b': c = rc(R_BONE, 8); break;
            case 's': c = rc(R_SKIN, 6); break;  case 'S': c = rc(R_SKIN, 10); break; case 'n': c = rc(R_SKIN, 13); break;
            case 'e': c = rc(R_FLAME, 11); break; case 'w': c = rc(R_BONE, 6); break;
            case 'i': c = rc(R_IRON, 4); break;  case 'j': c = rc(R_IRON, 8); break;  case 'J': c = rc(R_IRON, 11); break;
            case 'I': c = rc(R_BONE, 7); break;  case 'r': c = rc(R_BLOOD, 9); break; case 'E': c = rc(R_FLAME, 6); break;
            case 'f': c = rc(R_BONE, 10); break;
            default: continue;
            }
            tp(t, x, y, (uint8_t)c);
        }
}
static const char *const RAT_TILE[2][STS] = {
    { "................", "................", "................", "................", "................", "................",
      "................", "................", "..........sS....", "......kddmmlS...", "....kdmmmmllhl..", "...kdmmmmlllhehw",
      "s.kdmmmmmmllllln", ".s.kdbbbbbmmdk..", "...dk.....kd....", "..ss.......ss..." },   /* at full stretch */
    { "................", "................", "................", "................", "................", "................",
      "................", "................", "..........sS....", ".....kkddmmlS...", "...kddmmmmllhl..", "..kdmmmmmlllhehw",
      ".kdmmmmmmmllllln", "ss.kdbbbbbbmdk..", ".....dk..kd.....", ".....ss..ss....." },   /* gathered, its back arched */
};
static const char *const LING_TILE[2][STS] = {
    { "................", "................", "................", "................", "......I.........", "......j...I..I..", "...I.jj...j..j..", "...jJJJJj.j.j.j.", "...JJrrrjjJJj.j.", "..jjjjrjjJJjeEj.", "..IjjrrrijjjjEIj", "..jjjiiiijiiif.j", "..j.j......j...j", ".j.j.......j....", ".j.j.......j....", ".j.........j...." },
    { "................", "................", "................", "................", "..........I.....", "...I..I...j.....", "...jj.j...j..I..", "..j.JJJJj.jj.j..", "..jJJrrrjjJJjj..", "..jjjjrjjJJjeEj.", ".jIjjrrrijjjjEI.", ".j.jjiiiijiiffj.", ".j...j......j.j.", "j...j.......j..j", "j...j.......j..j", "....j..........j" }
};
static void rat_tile(uint8_t *t, int frame) { vermin_grid(t, RAT_TILE[frame & 1]); }
static void ling_tile(uint8_t *t, int frame) { vermin_grid(t, LING_TILE[frame & 1]); }

/* ---- the figure: a hooded caster in a robe, from a rig ---- */

typedef struct { float x, y; } V2;
typedef struct {
    float bob, lean, crouch;                                       /* the hips' drop, the body's lean forward, how far down */
    float th[2], sh[2];                                            /* each leg (0 behind, 1 before): thigh and shin, from straight down */
    float ua[2], fa[2];                                            /* each arm: upper and fore */
    float rot, flat;                                               /* falling over: turned back about the feet; lying, the drop */
    int cast;                                                      /* the front hand alight */
} Pose;

static uint8_t PART[SF_H][SF_W], DEPTH[SF_H][SF_W];
static V2 rot_about(V2 p, V2 c, float a) {
    float s = sinf(a), co = cosf(a), dx = p.x - c.x, dy = p.y - c.y;
    return (V2){ c.x + dx * co - dy * s, c.y + dx * s + dy * co };
}
static V2 limb(V2 from, float a, float len) { return (V2){ from.x + sinf(a) * len, from.y + cosf(a) * len }; }
static void plot(float fx, float fy, int part, int depth) {
    int x = (int)floorf(fx), y = (int)floorf(fy);
    if (x < 0 || y < 0 || x >= SF_W || y >= SF_H) return;
    if (PART[y][x] && DEPTH[y][x] > depth) return;
    PART[y][x] = (uint8_t)part; DEPTH[y][x] = (uint8_t)depth;
}
static void disc(V2 c, float r, int part, int depth) {
    for (int y = (int)floorf(c.y - r - 1); y <= (int)ceilf(c.y + r + 1); y++)
        for (int x = (int)floorf(c.x - r - 1); x <= (int)ceilf(c.x + r + 1); x++) {
            float dx = (float)x + 0.5f - c.x, dy = (float)y + 0.5f - c.y;
            if (dx * dx + dy * dy <= r * r) plot((float)x, (float)y, part, depth);
        }
}
static void seg(V2 a, V2 b, float r, int part, int depth) {
    float len = hypotf(b.x - a.x, b.y - a.y);
    int n = (int)(len * 3.0f) + 1;
    for (int i = 0; i <= n; i++) { float k = (float)i / (float)n; disc((V2){ a.x + (b.x - a.x) * k, a.y + (b.y - a.y) * k }, r, part, depth); }
}
static void poly(const V2 *p, int n, int part, int depth) {        /* a filled shape, by the crossing rule */
    float x0 = 1e9f, x1 = -1e9f, y0 = 1e9f, y1 = -1e9f;
    for (int i = 0; i < n; i++) { x0 = fminf(x0, p[i].x); x1 = fmaxf(x1, p[i].x); y0 = fminf(y0, p[i].y); y1 = fmaxf(y1, p[i].y); }
    for (int y = (int)floorf(y0); y <= (int)ceilf(y1); y++)
        for (int x = (int)floorf(x0); x <= (int)ceilf(x1); x++) {
            float px = (float)x + 0.5f, py = (float)y + 0.5f;
            int in = 0;
            for (int i = 0, j = n - 1; i < n; j = i++)
                if ((p[i].y > py) != (p[j].y > py) && px < (p[j].x - p[i].x) * (py - p[i].y) / (p[j].y - p[i].y) + p[i].x) in = !in;
            if (in) plot((float)x, (float)y, part, depth);
        }
}

/* The figure at half its first height (the owner, 26 November 2026): every length times SK, the limbs and the hood a little
 * fuller than that (SR) so it still reads at thirteen pixels; the head a touch large, as small figures in old games have it. */
#define SK 0.5f
#define SR 0.68f
#define LEG_T (5.0f * SK)
#define LEG_S (5.5f * SK)
static void draw_pose(const Pose *P, uint8_t *out, uint8_t *parts) {
    memset(PART, 0, sizeof PART); memset(DEPTH, 0, sizeof DEPTH);
    V2 foot0 = { SF_W / 2.0f, SF_H - 1.0f };                       /* the pivot for falling over */
    float drop = (P->crouch * 6.0f + P->bob) * SK;
    V2 hip = { SF_W / 2.0f, SF_H - 1.2f - (LEG_T + LEG_S) + drop };
    V2 knee[2], foot[2];
    for (int l = 0; l < 2; l++) { knee[l] = limb(hip, P->th[l], LEG_T); foot[l] = limb(knee[l], P->sh[l], LEG_S); }
    V2 shoulder = { hip.x + sinf(P->lean) * 8.5f * SK, hip.y - cosf(P->lean) * 8.5f * SK };
    V2 neck = { hip.x + sinf(P->lean) * 9.5f * SK, hip.y - cosf(P->lean) * 9.5f * SK };
    V2 head = { neck.x + sinf(P->lean) * 3.6f * SK + 0.3f, neck.y - cosf(P->lean) * 3.6f * SK };
    V2 elbow[2], hand[2];
    for (int a = 0; a < 2; a++) { elbow[a] = limb(shoulder, P->ua[a] + P->lean * 0.3f, 4.2f * SK); hand[a] = limb(elbow[a], P->fa[a] + P->lean * 0.3f, 3.8f * SK); }
    /* turned about the feet, and dropped as it lies */
    V2 *all[] = { &hip, &knee[0], &knee[1], &foot[0], &foot[1], &shoulder, &neck, &head, &elbow[0], &elbow[1], &hand[0], &hand[1] };
    for (unsigned i = 0; i < sizeof all / sizeof all[0]; i++) { *all[i] = rot_about(*all[i], foot0, -P->rot); all[i]->y += P->flat - sinf(P->rot) * 3.0f * SK; all[i]->x += sinf(P->rot) * 13.0f * SK; }
    /* behind: the far arm and leg, darker */
    seg(shoulder, elbow[0], 1.3f * SR, FP_ROBE, 1); seg(elbow[0], hand[0], 1.1f * SR, FP_ROBE, 1); disc(hand[0], 1.0f * SR, FP_SKIN, 1);
    seg(hip, knee[0], 1.4f * SR, FP_ROBE, 1); seg(knee[0], foot[0], 1.1f * SR, FP_BOOT, 1);
    seg(foot[0], (V2){ foot[0].x + 2.0f * SK * cosf(P->rot), foot[0].y - 2.0f * SK * sinf(P->rot) }, 0.9f * SR, FP_BOOT, 1);
    /* the robe: shoulders to a hem that swings with the knees */
    float tilt = -P->rot;
    V2 hem_b = { fminf(knee[0].x, knee[1].x) - 1.6f * SR, fmaxf(knee[0].y, knee[1].y) + 1.0f * SK };
    V2 hem_f = { fmaxf(knee[0].x, knee[1].x) + 1.6f * SR, fmaxf(knee[0].y, knee[1].y) + 0.5f * SK };
    if (P->rot > 0.4f) { hem_b = (V2){ knee[0].x - 1.0f * SR, knee[0].y + 1.5f * SK }; hem_f = (V2){ knee[1].x + 1.0f * SR, knee[1].y + 1.5f * SK }; }
    V2 sb = rot_about((V2){ shoulder.x - 2.6f * SR, shoulder.y + 0.5f * SK }, shoulder, tilt), sf = rot_about((V2){ shoulder.x + 2.4f * SR, shoulder.y + 0.5f * SK }, shoulder, tilt);
    V2 wb = rot_about((V2){ hip.x - 3.2f * SR, hip.y }, hip, tilt), wf = rot_about((V2){ hip.x + 3.0f * SR, hip.y }, hip, tilt);
    V2 robe[] = { sb, sf, wf, hem_f, hem_b, wb };
    poly(robe, 6, FP_ROBE, 2);
    for (float k = 0.0f; k <= 1.0f; k += 0.08f) plot(hem_b.x + (hem_f.x - hem_b.x) * k, hem_b.y + (hem_f.y - hem_b.y) * k - 0.5f, FP_TRIM, 3);
    seg(rot_about((V2){ hip.x - 3.0f * SR, hip.y - 0.6f }, hip, tilt), rot_about((V2){ hip.x + 3.0f * SR, hip.y - 0.6f }, hip, tilt), 0.45f, FP_BELT, 3);
    /* before: the near leg below the hem */
    seg(knee[1], foot[1], 1.1f * SR, FP_BOOT, 4);
    seg(foot[1], (V2){ foot[1].x + 2.2f * SK * cosf(P->rot), foot[1].y - 2.2f * SK * sinf(P->rot) }, 0.9f * SR, FP_BOOT, 4);
    /* the hood, its face in shadow, a peak behind */
    disc(head, 3.4f * 0.72f, FP_HOOD, 5);
    V2 peak = rot_about((V2){ head.x - 3.2f * 0.62f, head.y - 1.6f * 0.62f }, head, tilt);
    seg(head, peak, 1.4f * SR, FP_HOOD, 5);
    V2 face = rot_about((V2){ head.x + 1.2f, head.y + 0.4f }, head, tilt);
    disc(face, 1.5f * 0.7f, FP_FACE, 6);
    plot(face.x + 0.5f * cosf(tilt), face.y + 0.5f * sinf(tilt) + 0.7f, FP_SKIN, 7);   /* the chin, in what light reaches it */
    /* the near arm, over all */
    seg(shoulder, elbow[1], 1.35f * SR, FP_ROBE, 8); seg(elbow[1], hand[1], 1.15f * SR, FP_ROBE, 8);
    disc(hand[1], 1.05f * SR, P->cast ? FP_GLOW : FP_SKIN, 9);
    if (P->cast) disc(hand[1], 1.6f, FP_GLOW, 0);

    /* colour: each part its ramp, lit from above and before; the far side darker; an outline round all */
    for (int y = 0; y < SF_H; y++)
        for (int x = 0; x < SF_W; x++) {
            int p = PART[y][x], i = y * SF_W + x;
            parts[i] = (uint8_t)p;
            if (!p) { out[i] = C_CLEAR; continue; }
            int up = y > 0 && PART[y - 1][x] == p && DEPTH[y - 1][x] == DEPTH[y][x];
            int dn = y < SF_H - 1 && PART[y + 1][x] == p && DEPTH[y + 1][x] == DEPTH[y][x];
            int fr = x < SF_W - 1 && PART[y][x + 1] == p, bk = x > 0 && PART[y][x - 1] == p;
            int k = 0, far = DEPTH[y][x] == 1;
            switch (p) {
            case FP_HOOD: k = 8; break;
            case FP_FACE: k = 2; break;
            case FP_ROBE: k = 7; break;
            case FP_TRIM: k = 8; break;
            case FP_SKIN: k = 9; break;
            case FP_BOOT: k = 6; break;
            case FP_BELT: k = 8; break;
            case FP_GLOW: k = DEPTH[y][x] ? 14 : 9; break;
            }
            if (p != FP_GLOW && p != FP_FACE) k += (!up) * 2 + (!fr) * 1 - (!dn) * 2 - (!bk) * 1;
            if (far) k -= 3;
            int ramp = p == FP_HOOD || p == FP_ROBE || p == FP_FACE ? R_PURPLE : p == FP_TRIM ? R_GOLD : p == FP_SKIN ? R_SKIN : p == FP_BOOT ? R_WOOD
                     : p == FP_BELT ? R_WOOD : R_GLOW;
            out[i] = rc(ramp, p == FP_TRIM ? k : k);
        }
    for (int y = 0; y < SF_H; y++)                                 /* the outline, in the clear round the figure */
        for (int x = 0; x < SF_W; x++) {
            int i = y * SF_W + x;
            if (PART[y][x]) continue;
            int n = (x > 0 && PART[y][x - 1] && PART[y][x - 1] != FP_GLOW) || (x < SF_W - 1 && PART[y][x + 1] && PART[y][x + 1] != FP_GLOW)
                 || (y > 0 && PART[y - 1][x] && PART[y - 1][x] != FP_GLOW) || (y < SF_H - 1 && PART[y + 1][x] && PART[y + 1][x] != FP_GLOW);
            if (n) { out[i] = rc(R_STONE, 0); parts[i] = FP_LINE; }
        }
}

static void make_figure(void) {
    Pose P;
    const float PI = 3.14159265f;
    for (int f = 0; f < NSF; f++) {
        memset(&P, 0, sizeof P);
        P.ua[0] = P.ua[1] = 0.15f; P.fa[0] = P.fa[1] = 0.45f;
        P.th[0] = -0.08f; P.th[1] = 0.08f; P.sh[0] = -0.08f; P.sh[1] = 0.08f;
        if (f <= SF_IDLE1) {
            P.bob = (float)(f - SF_IDLE0) * 0.6f; P.ua[1] = 0.2f; P.fa[1] = 0.7f + 0.1f * (float)(f - SF_IDLE0);
        } else if (f >= SF_WALK0 && f <= SF_WALK5) {
            float ph = (float)(f - SF_WALK0) / 6.0f * 2.0f * PI;
            for (int l = 0; l < 2; l++) {
                float p = ph + (float)l * PI;
                P.th[l] = 0.42f * sinf(p);
                P.sh[l] = P.th[l] - 0.55f * fmaxf(0.0f, cosf(p));
                P.ua[l] = -0.40f * sinf(p) + 0.1f; P.fa[l] = P.ua[l] + 0.35f;
            }
            P.bob = 0.6f * fabsf(cosf(ph)); P.lean = 0.05f;
        } else if (f >= SF_RUN0 && f <= SF_RUN5) {
            float ph = (float)(f - SF_RUN0) / 6.0f * 2.0f * PI;
            for (int l = 0; l < 2; l++) {
                float p = ph + (float)l * PI;
                P.th[l] = 0.75f * sinf(p) + 0.1f;
                P.sh[l] = P.th[l] - 1.1f * fmaxf(0.0f, cosf(p)) - 0.2f;
                P.ua[l] = -0.85f * sinf(p) + 0.2f; P.fa[l] = P.ua[l] + 1.3f;
            }
            P.bob = 1.1f * fabsf(cosf(ph)) - 0.4f; P.lean = 0.28f;
        } else if (f == SF_RISE) {
            P.th[1] = 0.95f; P.sh[1] = -0.2f; P.th[0] = -0.25f; P.sh[0] = -0.55f;
            P.ua[0] = -1.1f; P.fa[0] = -0.6f; P.ua[1] = 2.3f; P.fa[1] = 2.6f; P.lean = 0.12f; P.bob = -1.0f;
        } else if (f == SF_APEX) {
            P.th[1] = 0.7f; P.sh[1] = 0.1f; P.th[0] = 0.15f; P.sh[0] = -0.5f;
            P.ua[0] = -1.4f; P.fa[0] = -1.0f; P.ua[1] = 1.6f; P.fa[1] = 1.9f; P.lean = 0.05f; P.bob = -1.0f;
        } else if (f == SF_FALL) {
            P.th[1] = 0.35f; P.sh[1] = 0.15f; P.th[0] = -0.2f; P.sh[0] = -0.1f;
            P.ua[0] = -2.0f; P.fa[0] = -2.2f; P.ua[1] = 2.0f; P.fa[1] = 2.3f; P.lean = -0.05f; P.bob = -1.0f;
        } else if (f >= SF_CROUCH && f <= SF_CREEP1) {
            float s = f == SF_CREEP0 ? 0.25f : f == SF_CREEP1 ? -0.25f : 0.0f;
            P.crouch = 1.0f; P.lean = 0.45f;
            P.th[0] = 1.0f - s; P.sh[0] = -0.9f; P.th[1] = 1.35f + s; P.sh[1] = -0.35f;
            P.ua[1] = 0.9f; P.fa[1] = 1.5f; P.ua[0] = 0.5f; P.fa[0] = 1.2f;
        } else if (f == SF_CAST0 || f == SF_CAST1) {
            P.ua[1] = f == SF_CAST0 ? 1.1f : 1.55f; P.fa[1] = f == SF_CAST0 ? 1.4f : 1.6f; P.cast = 1;
            P.ua[0] = -0.4f; P.fa[0] = -0.1f; P.th[0] = -0.3f; P.sh[0] = -0.3f; P.th[1] = 0.3f; P.sh[1] = 0.2f; P.lean = 0.1f;
        } else if (f == SF_HURT) {
            P.lean = -0.35f; P.ua[0] = -0.9f; P.ua[1] = 1.2f; P.fa[1] = 2.0f; P.th[1] = 0.3f; P.sh[1] = 0.1f; P.bob = 0.5f;
        } else {                                                    /* falling over, backwards, and lying */
            int k = f - SF_DIE0;
            static const float ROT[5] = { 0.15f, 0.45f, 0.95f, 1.40f, 1.57f };
            P.rot = ROT[k]; P.flat = 0.0f;
            P.lean = -0.25f - 0.1f * (float)k;
            P.th[1] = 0.25f + 0.15f * (float)k; P.sh[1] = 0.05f; P.th[0] = -0.1f; P.sh[0] = -0.15f;
            P.ua[0] = -1.0f - 0.3f * (float)k; P.fa[0] = -0.6f - 0.3f * (float)k; P.ua[1] = 1.4f + 0.3f * (float)k; P.fa[1] = 1.9f + 0.2f * (float)k;
            if (k >= 3) { P.ua[0] = k == 3 ? -1.2f : -0.2f; P.fa[0] = k == 3 ? -0.9f : 0.1f; P.ua[1] = k == 3 ? 1.0f : 0.25f; P.fa[1] = k == 3 ? 1.2f : 0.5f; }
            P.crouch = k <= 1 ? 0.15f * (float)k : 0.0f;
        }
        draw_pose(&P, side_fig[f], side_fig_part[f]);
    }
}

/* a crypt wall's 64 pixels square made one tile: each 4 by 4 its commonest colour (2e: the breath's slab and the cold crack) */
static void from_tex(uint8_t *t, int tex) {
    for (int y = 0; y < STS; y++) for (int x = 0; x < STS; x++) {
        uint8_t seen[16]; int count[16], n = 0, best = 0;
        for (int k = 0; k < 16; k++) {
            uint8_t c = crawl_tex[tex][(x * 4 + (k & 3)) * TS + y * 4 + (k >> 2)];
            int j = 0;
            while (j < n && seen[j] != c) j++;
            if (j == n) { seen[n] = c; count[n++] = 0; }
            count[j]++;
            if (count[j] > count[best]) best = j;
        }
        t[y * STS + x] = seen[best];
    }
}
void side_art_traps(void) {
    from_tex(side_tile[ST_HZ_SLAB], T_HZ_SLAB); from_tex(side_tile[ST_HZ_SLAB_OPEN], T_HZ_SLAB_OPEN); from_tex(side_tile[ST_HZ_COLD], T_HZ_COLD);
    from_tex(side_tile[ST_HZ_GRATE], T_GRATE); from_tex(side_tile[ST_HZ_SLUICE], T_HZ_SLUICE); from_tex(side_tile[ST_HZ_SLUICE_OPEN], T_HZ_SLUICE_OPEN);
    from_tex(side_tile[ST_HZ_SAINT], T_HZ_SAINT); from_tex(side_tile[ST_HZ_SAINT_LIT], T_HZ_SAINT_LIT);
}
void side_art_init(void) {
    static int done;
    if (done) return;
    done = 1;
    stone(side_tile[ST_STONE0], R_STONE, 9, 8, 8, 0, 1);
    stone(side_tile[ST_STONE1], R_STONE, 9, 8, 8, 1, 2);
    stone(side_tile[ST_STONE2], R_STONE, 8, 8, 16, 2, 3);
    stone(side_tile[ST_STONE_TOP], R_STONE, 9, 8, 8, 0, 4); lip(side_tile[ST_STONE_TOP], 4);
    brick(side_tile[ST_BRICK], 5);
    brick(side_tile[ST_BRICK_TOP], 6); lip(side_tile[ST_BRICK_TOP], 6);
    ledge(side_tile[ST_LEDGE]);
    spikes(side_tile[ST_SPIKES]);
    for (int f = 0; f < 4; f++) water(side_tile[ST_WATER0 + f], f, 0);
    water(side_tile[ST_WATER_DEEP], 0, 1);
    fall_block(side_tile[ST_FALL], 0); fall_block(side_tile[ST_FALL_CRACK], 1);
    back_wall(side_tile[ST_BACK0], 0); back_wall(side_tile[ST_BACK1], 1);
    back_niche(side_tile[ST_BACK_NICHE]); back_skulls(side_tile[ST_BACK_SKULLS]);
    far_wall(side_tile[ST_FAR]);
    for (int p = 0; p < 3; p++) plat(side_tile[ST_PLAT_L + p], p);
    doorway(side_tile[ST_DOOR_T], side_tile[ST_DOOR_B], 0);
    doorway(side_tile[ST_GOAL_T], side_tile[ST_GOAL_B], 1);
    door_lit(side_tile[ST_DOOR_LIT]);
    lever(side_tile[ST_SWITCH0], 0); lever(side_tile[ST_SWITCH1], 1);
    for (int k = 0; k < 3; k++) key(side_tile[ST_KEY0 + k], k);
    life(side_tile[ST_LIFE]);
    for (int f = 0; f < 3; f++) torch(side_tile[ST_TORCH0 + f], f);
    chain(side_tile[ST_CHAIN]);
    slate(side_tile[ST_SLATE], 0); slate(side_tile[ST_SLATE_TOP], 1);
    web(side_tile[ST_WEB]);
    crack(side_tile[ST_CRACK]);
    bell_stone(side_tile[ST_STONE_A], 0); bell_stone(side_tile[ST_STONE_B], 1);
    ghost_stone(side_tile[ST_GHOST]);
    for (int f = 0; f < 4; f++) belt(side_tile[ST_BELT0 + f], f);
    press_head(side_tile[ST_PRESS]);
    tomb(side_tile[ST_TOMB_L], 0); tomb(side_tile[ST_TOMB_R], 1);
    springboard(side_tile[ST_SPRING0], 0); springboard(side_tile[ST_SPRING1], 1);
    for (int k = 0; k < 3; k++) bell_hung(side_tile[ST_BELL0 + k], k);
    climb_chain(side_tile[ST_CLIMB]);
    rat_tile(side_tile[ST_RAT0], 0); rat_tile(side_tile[ST_RAT1], 1); ling_tile(side_tile[ST_LING0], 0); ling_tile(side_tile[ST_LING1], 1);
    make_figure();
}
