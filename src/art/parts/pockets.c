/* A part of crawl_art.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The pockets' walls, floors and ceilings: the Backrooms and Severance; the exit. */

/* ---- the Backrooms (1.0.0b6) ----
 * Everything here is drawn in R_FLOOR (dark brown, mustard, pale yellow) and R_CEIL (off-white), which
 * only the floor, the ceiling and the fog use in the crypt; a Backrooms pocket recolours the two
 * (crawl_art_theme). The tube lights are C_WHITE, and R_FLAME's palest step for the diffuser. */
static float paper_shade(int x, int y, uint32_t seed) {
    int sx = x & 31;
    float v = 0.74f + (fbm(x / 16.0f, y / 16.0f, 4, seed) - 0.5f) * 0.10f;
    if (sx == 0) v -= 0.06f;                                       /* the seam between two strips */
    if (sx == 8 || sx == 24) v += 0.05f;                           /* pinstripes */
    int cy = (y + (x >> 5) * 8) & 15, dx = abs(sx - 16);
    if (dx <= 3 && (cy == dx + 4 || cy == dx + 5)) v -= 0.07f;      /* a small chevron, over and over */
    v -= 0.12f * smoothstepf(34.0f, 58.0f, (float)y) * fbm(x / 8.0f, y / 8.0f, 8, seed + 5);   /* damp, rising */
    if (y <= 1) v -= 0.12f;                                        /* the ceiling's shadow */
    if (y >= 59) v = y == 59 ? 0.64f : 0.44f + 0.04f * h01(x, y, seed + 7);   /* the skirting */
    return v;
}

static void gen_paper(int t, int kind, uint32_t seed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float v = paper_shade(x, y, seed);
            uint8_t c;
            if (kind == 1 && y < 59) {                             /* a leak from above: a stain, its tide line, runs */
                float dx = x - 40.0f, dy = (y - 2.0f) * 0.8f;
                float r = sqrtf(dx * dx + dy * dy) + (fbm(x / 8.0f, y / 8.0f, 8, seed + 9) - 0.5f) * 12.0f;
                if (r < 20.0f) v -= 0.08f + 0.08f * (r / 20.0f);
                if (r >= 18.5f && r < 21.0f) v -= 0.10f;
                for (int k = 0; k < 3; k++) {
                    float rx = 33.0f + k * 6.0f + 2.0f * sinf(y * 0.21f + k);
                    if (fabsf(x - rx) < 0.8f && y < 30 + k * 9) v -= 0.07f;
                }
            }
            c = sh(R_FLOOR, v);
            if (kind == 2 && x >= 29 && x <= 34 && y >= 48 && y <= 56) {   /* a socket, painted over */
                c = sh(R_CEIL, (x == 29 || y == 48) ? 0.86f : (x == 34 || y == 56) ? 0.52f : 0.74f);
                if ((x == 31 || x == 32) && (y == 50 || y == 51 || y == 53 || y == 54)) c = sh(R_CEIL, 0.18f);
            }
            tput(t, x, y, c);
        }
}

/* a doorway between pockets: off-white trim, and through it another yellow room under a tube */
static void gen_bdoor(int t, uint32_t seed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t c;
            int trim = (x >= 16 && x <= 47 && y >= 8) && !(x >= 18 && x <= 45 && y >= 10);
            if (trim) c = sh(R_CEIL, (x == 16 || y == 8) ? 0.86f : (x == 47 || x == 18 || y == 10) ? 0.56f : 0.74f);
            else if (!(x >= 18 && x <= 45 && y >= 10)) c = sh(R_FLOOR, paper_shade(x, y, seed));
            else {
                int face;
                float s = room_view(x, y, &face);
                if (s < 0.4f) c = sh(R_FLOOR, 0.66f + 0.03f * h01(x, y, seed));
                else if (face == RV_CEIL) c = (fabsf(x + 0.5f - 32.0f) < 3.5f * s && s > 0.5f && s < 0.68f) ? C_WHITE : sh(R_CEIL, 0.66f - 0.22f * s);
                else if (face == RV_FLOOR) c = sh(R_FLOOR, 0.40f + 0.06f * h01(x, y, seed + 1) - 0.10f * s);
                else c = sh(R_FLOOR, 0.62f - 0.18f * s);
            }
            tput(t, x, y, c);
        }
}

/* The chain's end: a doorway onto a run of doorways, each 0.7 the size of the one before, paler further
 * in until they burn white; four frames that drift in, so it never arrives. */
static void gen_visportal(int t, int frame, uint32_t seed) {
    const float r = 0.70f, lr = -logf(r), p = frame / 4.0f;
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t c;
            int trim = (x >= 16 && x <= 47 && y >= 8) && !(x >= 18 && x <= 45 && y >= 10);
            if (trim) c = sh(R_CEIL, (x == 16 || y == 8) ? 0.86f : (x == 47 || x == 18 || y == 10) ? 0.56f : 0.74f);
            else if (!(x >= 18 && x <= 45 && y >= 10)) c = sh(R_FLOOR, paper_shade(x, y, seed));
            else {
                int face;
                float s = fmaxf(room_view(x, y, &face), 1e-4f);
                float d = -logf(s) / lr + p, f = d - floorf(d);    /* how many doorways in, and where between two */
                int k = (int)floorf(d);
                float heat = clampf((d - 3.0f) / 3.0f, 0.0f, 1.0f);   /* from the fourth doorway on, light itself */
                static const uint8_t BY[4][4] = { { 0, 8, 2, 10 }, { 12, 4, 14, 6 }, { 3, 11, 1, 9 }, { 15, 7, 13, 5 } };
                int lit = heat * 2.0f * 16.0f > BY[y & 3][x & 3] + 0.5f;   /* the light comes on through an ordered dither */
                if (s < 0.02f) c = C_WHITE;
                else if (f < 0.12f) c = heat > 0.4f ? C_WHITE : sh(R_CEIL, 0.86f);   /* each doorway's trim */
                else if (lit) c = (uint8_t)(R_FLAME + clampi(13 + (int)(heat * 2.99f) - (face == RV_FLOOR), 12, 15));
                else if (face == RV_CEIL) c = (fabsf(x + 0.5f - 32.0f) < 3.5f * s && f > 0.45f && f < 0.62f) ? C_WHITE : sh(R_CEIL, 0.60f + 0.05f * k);
                else if (face == RV_FLOOR) c = sh(R_FLOOR, 0.38f + 0.04f * k + 0.05f * h01(x, y, seed + 2));
                else c = sh(R_FLOOR, 0.60f + 0.05f * k);
            }
            tput(t, x, y, c);
        }
}

static void gen_carpet(int t, uint32_t seed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float v = 0.47f + (h01(x, y, seed) - 0.5f) * 0.10f + (fbm(x / 4.0f, y / 4.0f, 16, seed + 1) - 0.5f) * 0.10f;
            float damp = fbm(x / 16.0f, y / 16.0f, 4, seed + 2);
            if (damp > 0.60f) v -= (damp - 0.60f) * 0.5f;            /* dark where it is wet through */
            tput(t, x, y, sh(R_FLOOR, v));
        }
}

/* ceiling tiles: a grid of T-bars at x 16 and 48 and y 0 and 32, so a panel can sit in the middle of a
 * cell and run its length; kind 0 plain, 1 stained, 2 a tube panel lit, 3 the panel dead */
static void gen_ceiling(int t, int kind, uint32_t seed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            int gx = (x + 16) & 31, gy = y & 31;
            uint8_t c;
            if (gx == 0 || gy == 0) c = sh(R_CEIL, 0.88f);         /* the T-bars */
            else {
                float v = 0.70f + (h01(x, y, seed) - 0.5f) * 0.06f - (gx == 31 || gy == 31 ? 0.08f : 0.0f);
                if (h01(x, y, seed + 3) > 0.94f) v -= 0.10f;         /* pinholes */
                c = sh(R_CEIL, v);
                if (kind == 1) {                                   /* a water stain on the middle tile */
                    float dx = x - 34.0f, dy = y - 15.0f, rr = sqrtf(dx * dx + dy * dy) + (fbm(x / 8.0f, y / 8.0f, 8, seed + 4) - 0.5f) * 8.0f;
                    if (gx > 1 && gx < 31 && gy > 1 && gy < 31 && x >= 17 && x <= 47 && y < 32 && rr < 11.0f)
                        c = sh(R_FLOOR, rr > 9.5f ? 0.46f : 0.66f + 0.04f * h01(x, y, seed + 5));
                }
            }
            if (kind >= 2 && x >= 17 && x <= 47 && y >= 1) {       /* the panel, the cell's length */
                int frame = x <= 18 || x >= 46 || y <= 2 || y >= 62;
                int tube = (x >= 23 && x <= 25) || (x >= 37 && x <= 39);
                if (kind == 2) c = frame ? sh(R_CEIL, 0.92f) : (tube || ((x + y) & 3) == 0) ? C_WHITE : (uint8_t)(R_FLAME + 15);
                else {
                    int end = y <= 8 || y >= 55;
                    c = frame ? sh(R_CEIL, 0.82f) : tube ? sh(R_CEIL, end ? 0.22f : 0.44f) : sh(R_CEIL, ((x + y) & 3) == 0 ? 0.52f : 0.58f);
                }
            }
            tput(t, x, y, c);
        }
}

/* ---- Severance (1.0.0b6) ----
 * Again in R_FLOOR and R_CEIL, which a Severance pocket makes a muted green (the carpet, the doors, the
 * partitions) and a cool white (walls, ceiling, frames, the water cooler). The lit panels are C_WHITE. */
static float white_shade(int x, int y, uint32_t seed) {
    float v = 0.86f + (h01(x, y, seed) - 0.5f) * 0.02f;
    if ((x & 31) == 0) v -= 0.05f;                                 /* where two panels of the wall meet */
    if (y <= 1) v -= 0.08f;
    if (y >= 58) v = y == 58 ? 0.80f : 0.60f + 0.02f * h01(x, y, seed + 1);   /* a grey skirting */
    return v;
}

static void gen_white(int t, int kind, uint32_t seed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t c = sh(R_CEIL, white_shade(x, y, seed));
            if (kind == 1 && x >= 22 && x <= 41 && y >= 18 && y <= 30) {   /* a placard: a line of print, an arrow */
                c = sh(R_CEIL, (x == 22 || y == 18) ? 0.92f : (x == 41 || y == 30) ? 0.5f : 0.74f);
                if (y == 21 && x >= 25 && x <= 38 && (x % 3) != 2) c = sh(R_CEIL, 0.3f);
                if ((y == 26 && x >= 25 && x <= 34) || (x >= 34 && x <= 37 && abs(y - 26) <= 37 - x)) c = sh(R_CEIL, 0.15f);
            }
            if (kind == 2 && x >= 16 && x <= 47 && y >= 11 && y <= 37) {   /* a picture, calmer than it needs to be */
                if (x <= 17 || x >= 46 || y <= 12 || y >= 36) c = sh(R_WOOD, (x == 16 || y == 11) ? 0.5f : 0.3f);
                else {
                    float hz = 27.0f + 1.5f * sinf(x * 0.21f) + 0.8f * sinf(x * 0.53f + 1.0f);
                    if (y < hz) c = sh(R_GHOST, 0.84f - (y - 13) * 0.012f);            /* a pale sky */
                    else c = sh(R_MOSS, 0.56f + 0.06f * h01(x, y, seed + 3) - (y - hz) * 0.006f);   /* a field */
                    float tx = x - 37.0f, ty = y - 21.0f;
                    if (tx * tx / 9.0f + ty * ty / 12.0f < 1.0f) c = sh(R_MOSS, 0.3f + 0.08f * h01(x, y, seed + 4));   /* one tree */
                    if (x == 37 && y >= 24 && y < hz + 1) c = sh(R_WOOD, 0.3f);
                }
            }
            tput(t, x, y, c);
        }
}

/* a doorway between pockets: a door more fit for a ward than an office, in a steel frame */
static void gen_sdoor(int t, uint32_t seed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            uint8_t c = sh(R_CEIL, white_shade(x, y, seed));
            if (x >= 15 && x <= 48 && y >= 6) c = sh(R_CEIL, (x == 15 || y == 6) ? 0.84f : x == 48 ? 0.48f : 0.68f);   /* the frame */
            if (x >= 18 && x <= 45 && y >= 9) {                    /* the door, pale green steel */
                c = sh(R_FLOOR, 0.80f + 0.02f * h01(x, y, seed + 2) - (x == 45 || y == 9 ? 0.12f : 0.0f));
                if (x >= 25 && x <= 38 && y >= 16 && y <= 29) {    /* wired glass, dark behind */
                    int edge = x == 25 || x == 38 || y == 16 || y == 29;
                    c = edge ? sh(R_CEIL, 0.42f) : (((x + y) & 3) == 0 || ((x - y) & 3) == 0) ? sh(R_CEIL, 0.36f) : sh(R_FLOOR, 0.14f);
                    if (!edge && x - 26 < 4 && y - 17 < 4 - (x - 26)) c = sh(R_CEIL, 0.5f);   /* a glint on it */
                }
                if (y >= 11 && y <= 13 && x >= 27 && x <= 36) c = sh(R_CEIL, y == 12 && (x & 1) ? 0.2f : 0.55f);   /* a placard */
                if (y >= 35 && y <= 38 && x >= 20 && x <= 43) c = sh(R_IRON, y == 35 ? 0.82f : y == 38 ? 0.38f : 0.62f);   /* the push bar */
                if (y >= 53 && y <= 60 && x >= 19 && x <= 44) c = sh(R_IRON, 0.58f + 0.05f * sinf(x * 1.7f) + (y == 53 ? 0.15f : 0.0f));   /* kick plate */
                if (x == 19 && (abs(y - 15) <= 1 || abs(y - 34) <= 1 || abs(y - 52) <= 1)) c = sh(R_IRON, 0.4f);   /* hinges */
            }
            tput(t, x, y, c);
        }
}

static void gen_scarpet(int t, uint32_t seed) {                   /* close office carpet: a tight weave, nothing spilled */
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float v = 0.50f + (h01(x, y, seed) - 0.5f) * 0.05f + ((((x >> 1) + (y >> 1)) & 1) ? 0.025f : 0.0f);
            tput(t, x, y, sh(R_FLOOR, v));
        }
}

/* ceiling panels, four to a cell; lit, every panel shines (C_WHITE, with a faint prism in R_GLOW) */
static void gen_sceiling(int t, int lit_up, uint32_t seed) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            int gx = x & 31, gy = y & 31;
            uint8_t c;
            if (gx == 0 || gy == 0) c = sh(R_CEIL, 0.70f);
            else if (gx == 31 || gy == 31) c = sh(R_CEIL, 0.80f);
            else if (lit_up && gx >= 3 && gx <= 28 && gy >= 3 && gy <= 28) c = ((x + 2 * y) % 5 == 0) ? (uint8_t)(R_GLOW + 15) : C_WHITE;
            else c = sh(R_CEIL, 0.90f + (h01(x, y, seed) - 0.5f) * 0.02f);
            tput(t, x, y, c);
        }
}

static void gen_exit(int t, int frame, int open) {
    for (int y = 0; y < TS; y++)
        for (int x = 0; x < TS; x++) {
            float dx = x + 0.5f - 32.0f, dy = y + 0.5f - 27.0f;
            int in_door = (y >= 27 && fabsf(dx) < 19.0f) || (y < 27 && dx * dx + dy * dy < 19.0f * 19.0f);
            int in_arch = (y >= 27 && fabsf(dx) < 24.0f) || (y < 27 && dx * dx + dy * dy < 24.0f * 24.0f);
            uint8_t c;
            if (!in_arch) {                                        /* the wall around it */
                c = sh(R_STONE, block_shade(x, y, 211) * 0.8f);
            } else if (!in_door) {                                 /* voussoirs of the arch */
                float a = atan2f(dy, dx), r = sqrtf(dx * dx + dy * dy);
                int seg = y >= 27 ? (int)(y / 9) + 40 : (int)((a + PI_F) / (PI_F / 9.0f));
                float v = 0.45f + 0.18f * h01(seg, 3, 213) + fbm(x / 5.0f, y / 5.0f, 8, 214) * 0.15f;
                if ((y < 27 && fmodf((a + PI_F) / (PI_F / 9.0f), 1.0f) < 0.12f) || (y >= 27 && y % 9 == 0)) v = 0.12f;
                if (r > 23.0f || (y >= 27 && fabsf(dx) > 23.0f)) v -= 0.12f;
                c = sh(R_STONE, v);
                int rune = (y < 27 ? (int)((a + PI_F) / (PI_F / 9.0f)) : y / 9 + 9) % 2 == 0;   /* runes cut into every other stone */
                float rx = y < 27 ? fmodf((a + PI_F) / (PI_F / 9.0f), 1.0f) : (fabsf(dx) - 19.0f) / 5.0f;
                float ry = y < 27 ? (r - 19.0f) / 5.0f : fmodf(y / 9.0f, 1.0f);
                if (rune && rx > 0.3f && rx < 0.7f && ry > 0.25f && ry < 0.75f && ((x * 3 + y * 5 + (int)(rx * 7)) % 3 == 0))
                    c = (uint8_t)(R_GLOW + (frame ? 15 : 11));
            } else if (open) {                                     /* thrown open: a deep blue swirl, a pale light at its heart */
                float d = sqrtf(dx * dx * 0.8f + (y < 27 ? dy * dy : 0.0f) + (y - 40.0f) * (y - 40.0f) * 0.15f);
                float sw = sinf(atan2f(y - 40.0f, dx) * 5.0f + d * 0.45f) * 0.5f + 0.5f;
                int lv = clampi((int)(4.5f - d * 0.25f + sw * 3.0f), 0, 7);
                c = (uint8_t)(R_GLOW + (d < 4.0f ? 9 : lv));
            } else {                                               /* two iron-bound doors */
                int seam = fabsf(dx) < 0.8f;
                float v = 0.22f + 0.1f * h01(x / 6, 1, 215) + fbm(x / 3.0f, y / 9.0f, 16, 216) * 0.1f;
                if ((y - 2) % 12 == 0) v += 0.18f;
                if ((y - 2) % 12 == 1) v -= 0.08f;
                c = seam ? (uint8_t)R_IRON : sh(R_IRON, v);
                if ((y - 2) % 12 == 0 && ((x + 2) % 6 == 0)) c = sh(R_IRON, 0.85f);   /* rivets */
                float rr = sqrtf(dx * dx + (y - 41.0f) * (y - 41.0f));           /* the glowing seal */
                if (fabsf(rr - 8.0f) < 0.9f) c = (uint8_t)(R_GLOW + (frame ? 14 : 12));
                if (rr < 8.0f && (fabsf(dx) < 0.7f || fabsf(y - 41.0f) < 0.7f)) c = (uint8_t)(R_GLOW + (frame ? 12 : 10));
            }
            tput(t, x, y, c);
        }
    for (int y = 0; y < 9; y++)                                    /* the skull keystone */
        for (int x = 26; x < 38; x++) {
            float nx = (x + 0.5f - 32.0f) / 5.5f, ny = (y + 0.5f - 4.0f) / 4.6f;
            if (nx * nx + ny * ny > 1.0f) continue;
            float v = 0.55f + 0.4f * (-nx * 0.4f - ny * 0.5f);
            if (((x == 29 || x == 30) || (x == 33 || x == 34)) && (y == 3 || y == 4)) { tput(t, x, y, open ? (uint8_t)(R_GLOW + 15) : 0); continue; }
            if (x == 31 && y == 6) v = 0.1f;
            tput(t, x, y, sh(R_BONE, v));
        }
}

