/* A part of crawl_art.c (docs/PLAN-tidy.md): it is #included there in its place and never compiled on its own.
 * The puzzles' things; the things a message hides among; the Black Shrine. */

/* ---- puzzles' things (1.0.0b13, docs/PLAN-b13.md sections 6 and 7) ---- */
const char *const PZ_MARKS[16][7] = {
    { "...#...", "..###..", ".#####.", ".##.##.", ".#####.", ".#####.", "#######" },   /* a mitre */
    { ".#####.", "#######", "#.....#", "#.###.#", "#.###.#", "#######", ".#...#." },   /* a helm */
    { "..###..", ".#...#.", "#.....#", "#.....#", ".#...#.", "..###..", "......." },   /* a tonsure */
    { "#..#..#", "#.###.#", "#######", "#######", ".#####.", "#######", "......." },   /* a crown */
    { ".##....", "#..#...", "#..####", ".##..#.", ".....#.", "....##.", "......." },   /* a key */
    { "..###..", ".#.#.#.", "#..#..#", "#######", "#..#..#", ".#.#.#.", "..###.." },   /* a wheel */
    { "#.....#", ".#...#.", "..#.#..", "...#...", "..#.#..", ".#...#.", "#.....#" },   /* arrows, crossed */
    { "#.#.#.#", "#######", ".#####.", ".##.##.", ".#####.", ".#####.", "#######" },   /* a tower */
    { "..##...", ".####..", "#######", ".#####.", ".#...#.", ".#...#.", "......." },   /* a lamb */
    { "...#...", "..#.#..", "...#...", ".#####.", "..###..", "...#...", "..###.." },   /* a lamp */
    { "..###..", ".#...#.", "#..#..#", "#.###.#", "#..#..#", ".#...#.", "..###.." },   /* an eye */
    { ".#.#.#.", ".#.#.#.", ".#####.", "#######", ".#####.", "..###..", "..###.." },   /* a hand */
    { "#.....#", ".#...#.", "..#.#..", "...#...", "..#.#..", ".##.##.", "##...##" },   /* picks */
    { "..###..", ".##....", "##.....", "##.....", "##.....", ".##....", "..###.." },   /* a moon */
    { "...#...", "...#...", "#######", ".#####.", "..###..", ".##.##.", "#.....#" },   /* a star */
    { ".#####.", "#######", "#.#.#.#", "#######", ".#####.", ".#.#.#.", "......." },   /* a skull */
};
static void spr_pz_mark(int id, int k) {                           /* a mark, carved dark and edged pale, centred at (32, 30) */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 0; y < 7; y++)
        for (int x = 0; x < 7; x++)
            if (PZ_MARKS[k][y][x] == '#') { cv_px(29 + x, 27 + y, sh(R_BLOOD, 0.25f)); cv_px(30 + x, 28 + y, sh(R_BONE, 0.85f)); }
    cv_store(id);
}
static void spr_pz_pips(int id, int n) {                           /* one to nine pips, as on a die, centred at (32, 30) */
    static const char *const P[9] = { "....#....", "#.......#", "#...#...#", "#.#...#.#", "#.#.#.#.#", "#.##.##.#", "#.######.", "####.####", "#########" };
    cv_begin(cv_sprite, TS, TS);
    for (int k = 0; k < 9; k++)
        if (P[n - 1][k] == '#') { int x = 27 + (k % 3) * 4, y = 25 + (k / 3) * 4; cv_px(x, y, sh(R_GOLD, 0.9f)); cv_px(x + 1, y, sh(R_GOLD, 0.7f)); cv_px(x, y + 1, sh(R_GOLD, 0.6f)); cv_px(x + 1, y + 1, sh(R_GOLD, 0.4f)); }
    cv_store(id);
}
static void spr_pz_skull(void) {                                   /* a skull on the floor, its brow toward you (the mark goes there) */
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 61, 9, 1.8f, R_WARM + 1);
    cv_skull(32, 52, 1.15f, 0.25f, 0.85f);
    cv_outline(sh(R_BONE, 0.05f));
    cv_store(SP_PZ_SKULL);
}
static void spr_pz_plinth(void) {                                  /* a low plinth: what it takes stands on its top */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 46; y < 64; y++)
        for (int x = 20; x < 44; x++) cv_px(x, y, sh(R_STONE, 0.3f + 0.2f * (x < 23) + 0.15f * (y == 46) - 0.1f * (y > 60) + 0.06f * h01(x, y, 5)));
    for (int x = 18; x < 46; x++) { cv_px(x, 45, sh(R_STONE, 0.6f)); cv_px(x, 46, sh(R_STONE, 0.45f)); }
    cv_outline(sh(R_STONE, 0.02f));
    cv_store(SP_PZ_PLINTH);
}
static void spr_pz_dial(int id, int turn) {                        /* a bronze dial on a post, its pointer at one of eight */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 40; y < 64; y++) for (int x = 30; x < 34; x++) cv_px(x, y, sh(R_IRON, 0.3f + 0.2f * (x == 30)));
    cv_ellipse(32, 30, 11, 11, R_GOLD, 0.3f, 0.75f);
    cv_flat(32, 30, 8, 8, sh(R_GOLD, 0.2f));
    for (int k = 0; k < 8; k++) cv_px(32 + (int)lroundf(9.5f * sinf(k * 0.7854f)), 30 - (int)lroundf(9.5f * cosf(k * 0.7854f)), sh(R_BONE, 0.9f));
    float a = turn * 0.7854f;
    cv_line(32, 30, 32 + 7.0f * sinf(a), 30 - 7.0f * cosf(a), sh(R_BLOOD, 0.7f));
    cv_px(32, 30, sh(R_GOLD, 0.95f));
    cv_outline(sh(R_IRON, 0.02f));
    cv_store(id);
}
static void spr_pz_lever(int id, int down) {                       /* an iron lever in a slot in its stone block */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 48; y < 64; y++) for (int x = 24; x < 40; x++) cv_px(x, y, sh(R_STONE, 0.35f + 0.15f * (x < 27)));
    for (int x = 30; x < 34; x++) cv_px(x, 48, sh(R_STONE, 0.05f));
    float tx = down ? 42.0f : 22.0f, ty = down ? 44.0f : 30.0f;
    cv_capsule(32, 49, tx, ty, 1.4f, R_IRON, 0.25f, 0.65f);
    cv_ellipse(tx, ty, 2.6f, 2.6f, R_WOOD, 0.3f, 0.7f);
    cv_outline(sh(R_IRON, 0.02f));
    cv_store(id);
}
static void spr_pz_candle(int id, int lit) {                       /* a tall candle in an iron pricket */
    cv_begin(cv_sprite, TS, TS);
    cv_line(26, 63, 32, 52, sh(R_IRON, 0.35f)); cv_line(38, 63, 32, 52, sh(R_IRON, 0.35f));
    for (int y = 30; y < 52; y++) for (int x = 30; x < 34; x++) cv_px(x, y, sh(R_BONE, 0.6f + 0.25f * (x == 30)));
    for (int x = 28; x < 36; x++) cv_px(x, 52, sh(R_IRON, 0.5f));
    cv_px(32, 29, sh(R_IRON, 0.1f));
    if (lit) { cv_flat(32, 25, 2.2f, 4.0f, R_FLAME + 11); cv_flat(32, 26, 1.0f, 2.2f, R_FLAME + 15); }
    cv_store(id);
}
static void spr_pz_tablet(void) {                                  /* a carved tablet on a stand: something to read */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 54; y < 64; y++) for (int x = 28; x < 36; x++) cv_px(x, y, sh(R_STONE, 0.3f));
    for (int y = 22; y < 54; y++)
        for (int x = 18; x < 46; x++) {
            float top = 26.0f - sqrtf(fmaxf(0.0f, 14.0f * 14.0f - (x - 32.0f) * (x - 32.0f))) * 0.3f;
            if (y < top) continue;
            uint8_t c = sh(R_STONE, 0.45f + 0.1f * h01(x, y, 9));
            if (y > 30 && y < 50 && x > 21 && x < 43 && (y % 4) == 0 && h01(x, y, 3) > 0.35f) c = sh(R_STONE, 0.15f);   /* its lines */
            cv_px(x, y, c);
        }
    cv_outline(sh(R_STONE, 0.02f));
    cv_store(SP_PZ_TABLET);
}

/* a niche cut in the wall: an arched recess with a sill, a mark cut over it (drawn as a mark by the game) */
static void spr_pz_niche(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 14; y < 52; y++)
        for (int x = 18; x < 46; x++) {
            float dx = x + 0.5f - 32.0f, top = 26.0f - sqrtf(fmaxf(0.0f, 14.0f * 14.0f - dx * dx)) * 0.6f;
            if (y < top - 3.0f) continue;
            int edge = y < top || x < 21 || x > 42;
            cv_px(x, y, edge ? sh(R_STONE, 0.5f + 0.1f * (x < 24)) : sh(R_STONE, 0.04f + 0.03f * h01(x, y, 3)));
        }
    for (int x = 16; x < 48; x++) { cv_px(x, 52, sh(R_STONE, 0.65f)); cv_px(x, 53, sh(R_STONE, 0.4f)); cv_px(x, 54, sh(R_STONE, 0.2f)); }   /* its sill */
    cv_store(SP_PZ_NICHE);
}
/* a knight on his tomb, his helmed head turned: away, to the right, toward you, to the left */
static void spr_pz_effigy(int id, int turn) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 44; y < 64; y++) for (int x = 6; x < 58; x++) cv_px(x, y, sh(R_STONE, 0.3f + 0.15f * (y < 47) + 0.1f * (x < 9) - 0.12f * (y > 60) + 0.05f * h01(x, y, 7)));   /* the tomb chest */
    for (int x = 10; x < 54; x += 9) for (int y = 50; y < 60; y++) cv_px(x, y, sh(R_STONE, 0.12f));   /* its panels */
    cv_capsule(16, 40, 44, 40, 4.5f, R_IRON, 0.2f, 0.6f);             /* the knight lying */
    cv_capsule(40, 38, 50, 38, 2.0f, R_IRON, 0.25f, 0.55f);           /* his feet, and a hound under them */
    cv_ellipse(51, 41, 3.0f, 2.2f, R_STONE, 0.3f, 0.6f);
    cv_line(24, 38, 30, 34, sh(R_GHOST, 0.5f));                       /* the sword on his breast */
    float hx = 13.0f, hy = 35.0f;                                     /* the helm on its pillow */
    cv_ellipse(hx, hy + 3.0f, 5.0f, 2.0f, R_BLOOD, 0.25f, 0.5f);
    cv_ellipse(hx, hy, 4.2f, 4.4f, R_IRON, 0.25f, 0.75f);
    if (turn == 2) { cv_line(hx - 3, hy, hx + 3, hy, sh(R_IRON, 0.02f)); cv_line(hx, hy - 1, hx, hy + 3, sh(R_IRON, 0.02f)); }   /* the visor, at you */
    else if (turn == 1) { cv_line(hx + 1, hy, hx + 4, hy, sh(R_IRON, 0.02f)); cv_px((int)hx + 4, (int)hy + 1, sh(R_IRON, 0.02f)); }
    else if (turn == 3) { cv_line(hx - 4, hy, hx - 1, hy, sh(R_IRON, 0.02f)); cv_px((int)hx - 4, (int)hy + 1, sh(R_IRON, 0.02f)); }
    else { cv_line(hx - 2, hy - 3, hx + 2, hy - 3, sh(R_IRON, 0.85f)); }   /* the back of the helm, its crest */
    cv_outline(sh(R_STONE, 0.02f));
    cv_store(id);
}
/* a pan of bronze on its stand; and the great scale, its beam tipped left, level, or right */
static void spr_pz_pan(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 46; y < 64; y++) for (int x = 30; x < 34; x++) cv_px(x, y, sh(R_IRON, 0.3f + 0.2f * (x == 30)));
    cv_flat(32, 63, 8, 1.5f, sh(R_IRON, 0.2f));
    cv_ellipse(32, 45, 13, 3.5f, R_GOLD, 0.3f, 0.75f);
    cv_flat(32, 44, 11, 2.0f, sh(R_GOLD, 0.25f));
    cv_outline(sh(R_IRON, 0.02f));
    cv_store(SP_PZ_PAN);
}
static void spr_pz_scale(int id, int tilt) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 14; y < 64; y++) for (int x = 30; x < 34; x++) cv_px(x, y, sh(R_IRON, 0.3f + 0.25f * (x == 30)));
    cv_flat(32, 63, 10, 1.6f, sh(R_IRON, 0.2f));
    float d = (float)(tilt - 1) * 5.0f;
    cv_line(10, 16 + d, 54, 16 - d, sh(R_GOLD, 0.8f)); cv_line(10, 17 + d, 54, 17 - d, sh(R_GOLD, 0.45f));
    for (int k = 0; k < 2; k++) {
        float ex = k ? 54.0f : 10.0f, ey = k ? 16.0f - d : 16.0f + d;
        cv_line(ex, ey, ex - 5, ey + 16, sh(R_GOLD, 0.4f)); cv_line(ex, ey, ex + 5, ey + 16, sh(R_GOLD, 0.4f));
        cv_ellipse(ex, ey + 17, 7.0f, 2.2f, R_GOLD, 0.3f, 0.7f);
    }
    cv_ellipse(32, 14, 3, 3, R_GOLD, 0.4f, 0.9f);
    cv_outline(sh(R_IRON, 0.02f));
    cv_store(id);
}
/* a small reliquary: a gilt casket, a purple stone in its front (a red one on the judged), beads on its lid */
static void spr_pz_casket(void) {
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 10, 1.6f, R_WARM + 1);
    for (int y = 48; y < 62; y++) for (int x = 23; x < 41; x++) cv_px(x, y, sh(R_GOLD, 0.4f + 0.3f * (x < 27) + 0.15f * ((x + y) % 6 == 0)));
    for (int x = 22; x < 42; x++) { cv_px(x, 47, sh(R_GOLD, 0.85f)); cv_px(x, 46, sh(R_GOLD, 0.6f)); }
    cv_flat(32, 54, 2.0f, 2.0f, sh(R_PURPLE, 0.6f));
    cv_outline(sh(R_GOLD, 0.05f));
    cv_store(SP_PZ_CASKET);
}
/* the Order's banner on a pole: a cross on black, its foot ragged */
static void spr_pz_banner(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 4; y < 64; y++) cv_px(31, y, sh(R_WOOD, 0.35f)), cv_px(32, y, sh(R_WOOD, 0.55f));
    cv_line(18, 8, 46, 8, sh(R_WOOD, 0.5f));
    for (int y = 9; y < 46; y++)
        for (int x = 19; x < 46; x++) {
            if (x == 31 || x == 32) continue;
            if (y > 40 && h01(x, y, 13) < (float)(y - 40) / 6.0f) continue;   /* ragged */
            int cross = (x >= 30 && x <= 33) || (y >= 20 && y <= 23);
            cv_px(x, y, cross ? sh(R_BONE, 0.65f) : sh(R_BLOOD, 0.12f + 0.08f * h01(x, y, 2)));
        }
    cv_store(SP_PZ_BANNER);
}

/* a cistern let into the wall: a stone tank, its flood lines cut, water to a height (0 dry to 8 full) */
static void spr_pz_cistern(int id, int lvl) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 18; y < 64; y++)
        for (int x = 14; x < 50; x++) {
            int wall = x < 17 || x > 46 || y > 60;
            float wl = 60.0f - (float)lvl * 5.0f;
            uint8_t c = wall ? sh(R_STONE, 0.4f + 0.15f * (x < 16)) : (y >= wl ? sh(R_GHOST, 0.25f + 0.1f * ((x + y) & 1) + (y == (int)wl ? 0.4f : 0.0f)) : sh(R_STONE, 0.05f));
            if (!wall && (y - 20) % 5 == 0 && x < 21) c = sh(R_BONE, 0.6f);   /* the flood lines */
            cv_px(x, y, c);
        }
    cv_outline(sh(R_STONE, 0.02f));
    cv_store(id);
}
static void spr_pz_saint(void) {                                   /* a saint in grey stone, her hands held out for her emblem */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 56; y < 64; y++) for (int x = 20; x < 44; x++) cv_px(x, y, sh(R_STONE, 0.35f + 0.1f * (x < 23)));
    float robe[8] = { 24, 56, 40, 56, 37, 22, 27, 22 };
    cv_poly(robe, 4, R_STONE, 0.55f, 0.25f, 22, 34);
    cv_ellipse(32, 16, 4.5f, 5.0f, R_STONE, 0.3f, 0.7f);
    cv_capsule(28, 26, 22, 34, 1.6f, R_STONE, 0.3f, 0.6f); cv_capsule(36, 26, 42, 34, 1.6f, R_STONE, 0.3f, 0.6f);   /* arms out */
    cv_ellipse(32, 11, 6.5f, 2.0f, R_GOLD, 0.3f, 0.6f);               /* a halo, dull */
    cv_outline(sh(R_STONE, 0.02f));
    cv_store(SP_PZ_SAINT);
}
static void spr_pz_emblem(void) {                                  /* a small bronze emblem on the floor: its sign goes on its face */
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 8, 1.4f, R_WARM + 1);
    cv_ellipse(32, 55, 7.0f, 6.0f, R_GOLD, 0.25f, 0.65f);
    cv_outline(sh(R_GOLD, 0.05f));
    cv_store(SP_PZ_EMBLEM);
}
static void spr_pz_spout(void) {                                   /* a lion's-head spout, water from its mouth */
    cv_begin(cv_sprite, TS, TS);
    cv_ellipse(32, 36, 9, 9, R_STONE, 0.25f, 0.65f);
    cv_flat(32, 40, 3, 2, sh(R_STONE, 0.02f));
    for (int y = 42; y < 64; y++) cv_px(32 + (y & 1), y, sh(R_GHOST, 0.6f + 0.2f * (y & 1)));
    cv_outline(sh(R_STONE, 0.02f));
    cv_store(SP_PZ_SPOUT);
}
static void spr_pz_mill(int id, int turn) {                        /* a wooden mill wheel on its axle, paddles round it */
    cv_begin(cv_sprite, TS, TS);
    for (int a = 0; a < 40; a++) { float an = a * 0.157f; cv_px(32 + (int)lroundf(16.0f * cosf(an)), 40 + (int)lroundf(16.0f * sinf(an)), sh(R_WOOD, 0.4f)); }
    for (int k = 0; k < 8; k++) {
        float an = k * 0.785f + turn * 0.39f;
        cv_line(32, 40, 32 + 16.0f * cosf(an), 40 + 16.0f * sinf(an), sh(R_WOOD, 0.3f));
        cv_capsule(32 + 15.0f * cosf(an), 40 + 15.0f * sinf(an), 32 + 20.0f * cosf(an), 40 + 20.0f * sinf(an), 1.5f, R_WOOD, 0.3f, 0.6f);
    }
    cv_ellipse(32, 40, 2.5f, 2.5f, R_IRON, 0.3f, 0.6f);
    cv_outline(sh(R_WOOD, 0.02f));
    cv_store(id);
}
static void spr_pz_idol(void) {                                    /* a squat idol of black glass, gold in its eyes */
    cv_begin(cv_sprite, TS, TS);
    float body[8] = { 22, 63, 42, 63, 38, 30, 26, 30 };
    cv_poly(body, 4, R_IRON, 0.35f, 0.08f, 30, 33);
    cv_ellipse(32, 24, 8, 8, R_IRON, 0.1f, 0.45f);
    cv_px(29, 23, R_GOLD + 12); cv_px(35, 23, R_GOLD + 12); cv_px(29, 24, R_GOLD + 9); cv_px(35, 24, R_GOLD + 9);
    cv_outline(sh(R_IRON, 0.02f));
    cv_store(SP_PZ_IDOL);
}
static void spr_pz_lock(void) {                                    /* a great lock plate, a channel cut into its keyhole */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 28; y < 52; y++) for (int x = 22; x < 42; x++) cv_px(x, y, sh(R_IRON, 0.3f + 0.25f * (x < 25 || y < 31)));
    cv_flat(32, 38, 2.5f, 2.5f, sh(R_IRON, 0.02f)); for (int y = 38; y < 46; y++) cv_px(32, y, sh(R_IRON, 0.02f));
    for (int x = 10; x < 22; x++) cv_px(x, 38, sh(R_GOLD, 0.35f));
    cv_outline(sh(R_IRON, 0.02f));
    cv_store(SP_PZ_LOCK);
}
static void spr_pz_wheel(void) {                                   /* an iron sluice wheel on the wall, spoked; a sign on its hub */
    cv_begin(cv_sprite, TS, TS);
    for (int a = 0; a < 48; a++) { float an = a * 0.131f; cv_px(32 + (int)lroundf(14.0f * cosf(an)), 30 + (int)lroundf(14.0f * sinf(an)), sh(R_IRON, 0.5f)); cv_px(32 + (int)lroundf(13.0f * cosf(an)), 30 + (int)lroundf(13.0f * sinf(an)), sh(R_IRON, 0.3f)); }
    for (int k = 0; k < 6; k++) { float an = k * 1.047f; cv_line(32 + 6.0f * cosf(an), 30 + 6.0f * sinf(an), 32 + 13.0f * cosf(an), 30 + 13.0f * sinf(an), sh(R_IRON, 0.35f)); }
    cv_flat(32, 30, 6, 6, sh(R_STONE, 0.5f));
    cv_outline(sh(R_IRON, 0.02f));
    cv_store(SP_PZ_WHEEL);
}

/* the Hollows' and the Deep's puzzle pieces */
static void spr_pz_board(void) {                                   /* a slate board on two legs, chalked */
    cv_begin(cv_sprite, TS, TS);
    cv_line(20, 63, 22, 40, sh(R_WOOD, 0.4f)); cv_line(44, 63, 42, 40, sh(R_WOOD, 0.4f));
    for (int y = 16; y < 42; y++) for (int x = 14; x < 50; x++) cv_px(x, y, x < 16 || x > 47 || y < 18 || y > 39 ? sh(R_WOOD, 0.45f) : sh(R_STONE, 0.08f + 0.03f * h01(x, y, 17)));
    for (int k = 0; k < 5; k++) for (int x = 19; x < 44; x++) if (h01(x, k, 19) > 0.35f) cv_px(x, 21 + k * 4, sh(R_BONE, 0.7f));   /* the chalk */
    cv_store(SP_PZ_BOARD);
}
static void spr_pz_box(void) {                                     /* a counterweight box of iron-bound timber, open-topped */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 40; y < 64; y++) for (int x = 16; x < 48; x++) cv_px(x, y, (x - 16) % 8 == 0 || y == 40 || y == 63 ? sh(R_IRON, 0.4f) : sh(R_WOOD, 0.3f + 0.1f * h01(x, y, 3)));
    for (int y = 34; y < 40; y++) cv_line(18, y, 18, y, sh(R_IRON, 0.3f));
    cv_line(32, 0, 32, 40, sh(R_IRON, 0.35f));                        /* its chain up the shaft */
    cv_outline(sh(R_WOOD, 0.02f));
    cv_store(SP_PZ_BOX);
}
static void spr_pz_sack(void) {                                    /* a sack of ore, tied, tallied (the tally goes on it) */
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 11, 1.6f, R_WARM + 1);
    cv_ellipse(32, 53, 11, 9, R_WOOD, 0.3f, 0.65f);
    cv_ellipse(32, 44, 5, 3, R_WOOD, 0.35f, 0.6f);
    cv_line(28, 42, 36, 42, sh(R_WOOD, 0.1f));
    cv_outline(sh(R_WOOD, 0.02f));
    cv_store(SP_PZ_SACK);
}
static void spr_pz_hook(void) {                                    /* an iron hook on a strip of board */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 14; y < 24; y++) for (int x = 20; x < 44; x++) cv_px(x, y, sh(R_WOOD, 0.3f + 0.1f * (y == 14)));
    cv_line(32, 24, 32, 30, sh(R_IRON, 0.5f)); cv_line(32, 30, 35, 28, sh(R_IRON, 0.5f));
    cv_store(SP_PZ_HOOK);
}
static void spr_pz_tag(void) {                                     /* a brass tag on a thong, a number stamped on it */
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 6, 1.2f, R_WARM + 1);
    cv_ellipse(32, 56, 5, 4, R_GOLD, 0.25f, 0.7f);
    cv_line(32, 52, 30, 46, sh(R_WOOD, 0.3f));
    cv_px(31, 56, sh(R_GOLD, 0.1f)); cv_px(33, 56, sh(R_GOLD, 0.1f));
    cv_store(SP_PZ_TAG);
}
static void spr_pz_socket(void) {                                  /* a hollow cut in the rock to take a bone */
    cv_begin(cv_sprite, TS, TS);
    cv_ellipse(32, 30, 12, 9, R_STONE, 0.02f, 0.12f);
    for (int a = 0; a < 40; a++) { float an = a * 0.157f; cv_px(32 + (int)lroundf(12.0f * cosf(an)), 30 + (int)lroundf(9.0f * sinf(an)), sh(R_STONE, 0.5f)); }
    cv_store(SP_PZ_SOCKET);
}
static void spr_pz_fossil(void) {                                  /* a great bone, stone-grey and old */
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 14, 1.8f, R_WARM + 1);
    cv_bone(18, 56, 46, 54, 2.6f, 0.25f, 0.6f);
    cv_outline(sh(R_STONE, 0.02f));
    cv_store(SP_PZ_FOSSIL);
}
static void spr_pz_stone(void) {                                   /* a standing stone, rough, a mark cut in its face */
    cv_begin(cv_sprite, TS, TS);
    float body[8] = { 22, 63, 42, 63, 39, 12, 25, 14 };
    cv_poly(body, 4, R_STONE, 0.5f, 0.25f, 12, 51);
    for (int y = 14; y < 63; y++) for (int x = 22; x < 43; x++) if (h01(x, y, 29) > 0.93f) cv_px(x, y, sh(R_MOSS, 0.4f));
    cv_outline(sh(R_STONE, 0.02f));
    cv_store(SP_PZ_STONE);
}
static void spr_pz_lamp(int id, int lit) {                         /* a miner's lamp on a nail, dark or burning */
    cv_begin(cv_sprite, TS, TS);
    cv_line(32, 8, 32, 18, sh(R_IRON, 0.4f));
    for (int y = 18; y < 34; y++) for (int x = 27; x < 38; x++) cv_px(x, y, (x - 27) % 3 == 0 || y == 18 || y == 33 ? sh(R_IRON, 0.45f) : (lit ? (uint8_t)(R_FLAME + 8 + (y > 24) * 4) : sh(R_IRON, 0.08f)));
    cv_store(id);
}
static void spr_pz_prism(int id, int turn) {                       /* a prism of clear crystal on a black plinth, its bright face turned */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 50; y < 64; y++) for (int x = 24; x < 40; x++) cv_px(x, y, sh(R_IRON, 0.15f + 0.1f * (x < 27)));
    float tri[6] = { 22, 50, 42, 50, 32, 22 };
    cv_poly(tri, 3, R_GLOW, 0.25f, 0.55f, 22, 28);
    if (turn == 2) for (int y = 30; y < 50; y++) for (int x = 30 - (50 - y) / 6; x <= 34 + (50 - y) / 6; x++) cv_px(x, y, sh(R_GHOST, 0.85f));   /* its face, to you */
    else if (turn == 1) cv_line(36, 48, 40, 48, R_GLOW + 15), cv_line(34, 30, 41, 49, sh(R_GHOST, 0.9f));
    else if (turn == 3) cv_line(24, 48, 28, 48, R_GLOW + 15), cv_line(30, 30, 23, 49, sh(R_GHOST, 0.9f));
    else cv_line(32, 24, 32, 49, sh(R_GLOW, 0.2f));
    cv_outline(sh(R_IRON, 0.02f));
    cv_store(id);
}
static void spr_pz_geode(int id, int lit) {                        /* a split geode, dark inside, or full of light */
    cv_begin(cv_sprite, TS, TS);
    cv_ellipse(32, 50, 13, 12, R_STONE, 0.15f, 0.45f);
    for (int y = 42; y < 60; y++) for (int x = 24; x < 41; x++) if (hypotf(x - 32.0f, (y - 50.0f) * 1.2f) < 8.0f) cv_px(x, y, lit ? (uint8_t)(R_GLOW + 8 + ((x + y) & 3) * 2) : sh(R_PURPLE, 0.1f + 0.15f * ((x + y) & 1)));
    cv_outline(sh(R_STONE, 0.02f));
    cv_store(id);
}
static void spr_pz_throne(void) {                                  /* a throne of black glass, its back high, gold at its edges */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 6; y < 64; y++) for (int x = 18; x < 46; x++) {
        int back = y < 40 && x > 21 && x < 43, seat = y >= 40 && y < 48, legs = y >= 48 && (x < 23 || x > 40);
        if (!back && !seat && !legs) continue;
        cv_px(x, y, (x == 18 || x == 45 || y == 6) ? sh(R_GOLD, 0.7f) : sh(R_IRON, 0.08f + 0.06f * (x < 25)));
    }
    cv_outline(sh(R_IRON, 0.02f));
    cv_store(SP_PZ_THRONE);
}
static void spr_pz_statue(void) {                                  /* a statue of the dead court in black glass: who it is goes on its breast */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 56; y < 64; y++) for (int x = 24; x < 40; x++) cv_px(x, y, sh(R_IRON, 0.25f));
    float robe[8] = { 25, 56, 39, 56, 37, 22, 27, 22 };
    cv_poly(robe, 4, R_IRON, 0.35f, 0.1f, 22, 34);
    cv_ellipse(32, 17, 4.5f, 5.0f, R_IRON, 0.15f, 0.5f);
    cv_outline(sh(R_IRON, 0.01f));
    cv_store(SP_PZ_STATUE);
}
static void spr_pz_vent(int id, int open) {                        /* a vent in the rock, its iron shutter shut, or open with the cold breathing out */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 34; y < 56; y++) for (int x = 20; x < 44; x++) cv_px(x, y, x < 22 || x > 41 || y < 36 || y > 53 ? sh(R_IRON, 0.4f) : (open ? sh(R_GHOST, 0.2f + 0.15f * ((x + y) & 1)) : sh(R_IRON, 0.2f + 0.08f * ((y - 36) % 4 == 0))));
    if (open) for (int i = 0; i < 20; i++) cv_px(24 + (int)(h01(i, 1, 31) * 16.0f), 20 + (int)(h01(i, 2, 31) * 14.0f), sh(R_GHOST, 0.6f));
    cv_store(id);
}
static void spr_pz_slab(void) {                                    /* a square of the grid: a low slab, flush with the floor */
    cv_begin(cv_sprite, TS, TS);
    for (int y = 58; y < 64; y++) for (int x = 14; x < 50; x++) cv_px(x, y, sh(R_STONE, 0.45f + 0.15f * (y == 58)));
    cv_store(SP_PZ_SLAB);
}
static void spr_pz_numstone(void) {                                /* a rounded stone with a number of pips cut on it */
    cv_begin(cv_sprite, TS, TS);
    cv_flat(32, 62, 9, 1.4f, R_WARM + 1);
    cv_ellipse(32, 54, 9, 8, R_STONE, 0.3f, 0.7f);
    cv_outline(sh(R_STONE, 0.02f));
    cv_store(SP_PZ_NUMSTONE);
}
static void spr_pz_letter(int id, int ch) {                        /* a letter in brass, from the BIOS font, centred at (32, 30) */
    cv_begin(cv_sprite, TS, TS);
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++)
            if (font_bios8['A' + ch][r] >> (7 - c) & 1) { cv_px(28 + c, 26 + r, sh(R_GOLD, 0.9f)); cv_px(29 + c, 27 + r, sh(R_GOLD, 0.25f)); }
    cv_store(id);
}

/* where the hidden message lies (1.0.0b13, docs/PLAN-b13.md section 6a), a run each: a niche's candle-box, a satchel
 * drowned in the silt, a miner's boot on its side, a geode broken open */
static void spr_hide(int id, int run) {
    cv_begin(cv_sprite, TS, TS);
    if (run == 0) {                                                /* oak, its lid lifted at one end, two stubs and their wax */
        cv_flat(32, 62, 13, 1.6f, R_WARM + 1);
        for (int y = 48; y < 62; y++)
            for (int x = 21; x < 43; x++)
                cv_px(x, y, sh(R_WOOD, 0.28f + 0.22f * (x < 24) - 0.1f * (y > 59) + 0.08f * h01(x, y / 3, 41) + 0.1f * ((y - 48) % 5 == 0)));
        for (int x = 20; x < 44; x++) {
            int top = 45 - (x > 31 ? (x - 31) / 4 : 0);
            for (int y = top; y < 48; y++) cv_px(x, y, y == top ? sh(R_WOOD, 0.68f) : (y < top + 2 ? sh(R_WOOD, 0.46f) : sh(R_WOOD, 0.04f)));   /* and the dark under it */
        }
        for (int y = 51; y < 55; y++) for (int x = 30; x < 34; x++) cv_px(x, y, sh(R_IRON, 0.25f + 0.3f * (x == 30)));   /* its hasp */
        for (int k = 0; k < 2; k++) {
            int cx = 25 + k * 7, h = 6 + 3 * k;
            for (int y = 45 - h; y < 45; y++) for (int x = cx - 1; x <= cx + 1; x++) cv_px(x, y, sh(R_BONE, 0.55f + 0.2f * (x == cx - 1)));
            cv_px(cx, 44 - h, sh(R_WOOD, 0.05f));
            for (int y = 48; y < 50 + 3 * k; y++) cv_px(cx + 1, y, sh(R_BONE, 0.5f));
        }
        cv_outline(sh(R_WOOD, 0.02f));
    } else if (run == 1) {                                         /* black with water, its strap trailing, half sunk */
        cv_flat(32, 61, 17, 2.6f, sh(R_STONE, 0.14f));
        cv_ellipse(32, 54, 12, 8, R_WOOD, 0.08f, 0.35f);
        float flap[8] = { 21, 49, 43, 49, 40, 57, 24, 57 };
        cv_poly(flap, 4, R_WOOD, 0.38f, 0.16f, 49, 8);
        for (int y = 54; y < 58; y++) for (int x = 31; x < 34; x++) cv_px(x, y, sh(R_IRON, 0.4f + 0.3f * (y == 54)));   /* the buckle */
        for (int t = 0; t < 2; t++) { cv_line(22, 51 + t, 17, 44 + t, sh(R_WOOD, 0.3f)); cv_line(17, 44 + t, 24, 39 + t, sh(R_WOOD, 0.3f)); cv_line(24, 39 + t, 30, 43 + t, sh(R_WOOD, 0.25f)); }
        for (int x = 24; x < 41; x += 3) cv_px(x, 49 + (x & 1), sh(R_GHOST, 0.55f));   /* wet */
        for (int y = 58; y < 63; y++)                              /* the silt over its foot */
            for (int x = 16; x < 49; x++) if (h01(x, y, 43) < 0.25f + 0.18f * (float)(y - 58)) cv_px(x, y, sh(R_STONE, 0.18f + 0.12f * h01(x, y, 44)));
        cv_outline(sh(R_WOOD, 0.01f));
    } else if (run == 2) {                                         /* hobnailed, its laces loose, the shaft's mouth open */
        cv_capsule(42, 52, 27, 52, 5.0f, R_WOOD, 0.18f, 0.5f);     /* the foot */
        cv_capsule(23, 50, 19, 38, 6.0f, R_WOOD, 0.15f, 0.48f);    /* the shaft */
        cv_ellipse(44, 52, 4.5f, 4.6f, R_WOOD, 0.25f, 0.62f);      /* the toe cap */
        cv_flat(19, 34, 5.0f, 2.2f, sh(R_WOOD, 0.02f));
        for (int x = 17; x < 48; x++) for (int y = 57; y < 60; y++) cv_px(x, y, sh(R_IRON, 0.08f + 0.04f * (y == 57)));   /* the sole */
        for (int x = 19; x < 47; x += 4) cv_px(x, 60, sh(R_IRON, 0.6f));   /* its nails */
        for (int k = 0; k < 4; k++) { cv_line(27 + k * 3, 47, 29 + k * 3, 50, sh(R_BONE, 0.35f)); cv_line(29 + k * 3, 47, 27 + k * 3, 50, sh(R_BONE, 0.3f)); }
        cv_line(28, 47, 25, 42, sh(R_BONE, 0.3f)); cv_line(25, 42, 27, 39, sh(R_BONE, 0.3f));   /* a lace undone */
        cv_outline(sh(R_WOOD, 0.01f));
    } else {                                                       /* two halves, the crystals inside it dull without light */
        cv_ellipse(28, 51, 11, 10, R_STONE, 0.15f, 0.45f);
        for (int y = 43; y < 60; y++)
            for (int x = 20; x < 37; x++) {
                float d = hypotf(x + 0.5f - 28.5f, (y + 0.5f - 51.0f) * 1.05f);
                if (d < 6.6f) cv_px(x, y, sh(R_PURPLE, 0.25f + 0.5f * h01(x / 2, y / 2, 47) + 0.15f * (d > 5.4f)));
                else if (d < 7.6f) cv_px(x, y, sh(R_BONE, 0.45f));
            }
        cv_ellipse(45, 58, 7, 5, R_STONE, 0.12f, 0.4f);
        for (int x = 40; x < 51; x++) cv_px(x, 53 + abs(x - 45) / 3, sh(R_PURPLE, 0.55f));
        for (int i = 0; i < 6; i++) cv_px(18 + (int)(h01(i, 1, 48) * 32.0f), 61 + (int)(h01(i, 2, 48) * 2.0f), sh(R_PURPLE, 0.5f));   /* shards */
        cv_outline(sh(R_STONE, 0.02f));
    }
    cv_store(id);
}
/* the Black Shrine: a block of black stone, a niche, a skull in it, a stub of candle before it; its eyes are drawn
 * over it at full brightness, faint, or flaring when it shows */
static void spr_bshrine(void) {
    cv_begin(cv_sprite, TS, TS);
    for (int y = 12; y < 64; y++)
        for (int x = 19; x < 45; x++) {
            float top = 19.0f - (12.0f - fabsf(x + 0.5f - 32.0f)) * 0.42f;
            if (y < top) continue;
            int edge = y < top + 1.5f || x == 19 || x == 44;
            cv_px(x, y, sh(R_IRON, (edge ? 0.2f : 0.03f + 0.05f * h01(x, y / 2, 51)) + 0.05f * (x < 22) + 0.03f * (y > 59)));
        }
    for (int y = 23; y < 48; y++)
        for (int x = 24; x < 40; x++) {
            float dx = x + 0.5f - 32.0f, top = 31.0f - sqrtf(fmaxf(0.0f, 64.0f - dx * dx));
            if (y < top) continue;
            cv_px(x, y, (y < top + 1.0f || x == 24 || x == 39) ? sh(R_IRON, 0.22f) : sh(R_PURPLE, 0.02f));
        }
    cv_skull(32, 37, 0.85f, 0.3f, 0.62f);
    for (int k = -1; k <= 1; k += 2) cv_flat(32 + k * 2.1f, 37.8f, 1.5f, 1.7f, sh(R_IRON, 0.0f));   /* the sockets, black */
    for (int x = 18; x < 46; x++) for (int y = 60; y < 64; y++) cv_px(x, y, sh(R_IRON, 0.12f + 0.08f * (y == 60)));   /* its step */
    cv_flat(32, 59.5f, 4.0f, 1.0f, sh(R_BONE, 0.45f));            /* the candle's wax pool, and the stub */
    for (int y = 54; y < 59; y++) for (int x = 31; x < 34; x++) cv_px(x, y, sh(R_BONE, 0.55f + 0.2f * (x == 31)));
    cv_px(32, 53, sh(R_WOOD, 0.05f));
    cv_px(32, 52, R_FLAME + 12); cv_px(32, 51, R_FLAME + 15);
    cv_outline(sh(R_IRON, 0.0f));
    cv_store(SP_BSHRINE);
}
static void spr_bshrine_eyes(int id, int flare) {
    cv_begin(cv_sprite, TS, TS);
    for (int k = -1; k <= 1; k += 2) {
        int ex = 32 + k * 2 - (k < 0), ey = 38;
        if (!flare) { cv_px(ex, ey, R_PURPLE + 13); cv_px(ex, ey - 1, R_PURPLE + 8); continue; }
        for (int r = 1; r <= 5; r++) {                             /* rays, thinning */
            uint8_t c = (uint8_t)(R_PURPLE + 15 - r * 2);
            cv_px(ex - r, ey, c); cv_px(ex + r, ey, c);
            if (r <= 3) { cv_px(ex, ey - r, c); cv_px(ex, ey + r, c); }
            if (r <= 2) { cv_px(ex - r, ey - r, c); cv_px(ex + r, ey - r, c); }
        }
        cv_px(ex, ey, C_WHITE); cv_px(ex + 1, ey, R_PURPLE + 15); cv_px(ex, ey - 1, R_PURPLE + 15);
    }
    cv_store(id);
}

