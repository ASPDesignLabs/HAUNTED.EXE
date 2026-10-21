#include "haunt_art.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

ArtCell art[TROWS][TCOLS];

#define CELLW 9.0f
#define CELLH 16.0f

/* CP437 */
#define B_FULL 0xDB
#define B_DARK 0xB2
#define B_MED  0xB1
#define B_LITE 0xB0
#define B_TOP  0xDF
#define B_BOT  0xDC
#define L_HORZ 0xC4
#define L_VERT 0xB3


static float hash01(int x, int y, int s) {
    uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u + (uint32_t)s * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (float)(h & 0xFFFF) / 65535.0f;
}

static void put(int x, int y, uint8_t ch, uint8_t fg, uint8_t bg) {
    if (x < 0 || x >= TCOLS || y < 0 || y >= TROWS) return;
    art[y][x].ch = ch; art[y][x].fg = fg; art[y][x].bg = bg;
}


/* ------------------------------------------------------------------ */
/* Half-block canvas: 80x50 "pixels", two per character cell (the      */
/* ANSI-art trick: '▀' with the top colour as foreground and the bottom */
/* colour as background).  A foreground layer sits over the background; */
/* overlay characters (rain, fog, silk) replace a cell only where the   */
/* foreground is empty.                                                  */
/* ------------------------------------------------------------------ */

#define PW TCOLS
#define PH (TROWS * 2)
#define NOPIX 0xFF

static uint8_t cbg[PH][PW], cfg[PH][PW];
static uint8_t ovc[TROWS][TCOLS], ovf[TROWS][TCOLS];
static int g_events;

static const uint8_t BAYER4[4][4] = { {0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5} };

static void cv_begin(void) {
    memset(cbg, 0, sizeof cbg);
    memset(cfg, NOPIX, sizeof cfg);
    memset(ovc, 0, sizeof ovc);
}
static void bset(int x, int y, uint8_t c) { if ((unsigned)x < PW && (unsigned)y < PH) cbg[y][x] = c; }
static void fset(int x, int y, uint8_t c) { if ((unsigned)x < PW && (unsigned)y < PH) cfg[y][x] = c; }
static void ovset(int cx, int cy, uint8_t ch, uint8_t fg) {
    if ((unsigned)cx < TCOLS && (unsigned)cy < TROWS) { ovc[cy][cx] = ch; ovf[cy][cx] = fg; }
}

static void cv_flush(void) {
    for (int r = 0; r < TROWS; r++)
        for (int x = 0; x < PW; x++) {
            uint8_t ft = cfg[2 * r][x], fb = cfg[2 * r + 1][x];
            uint8_t t = ft != NOPIX ? ft : cbg[2 * r][x], b = fb != NOPIX ? fb : cbg[2 * r + 1][x];
            if (ft == NOPIX && fb == NOPIX && ovc[r][x]) put(x, r, ovc[r][x], ovf[r][x], cbg[2 * r][x]);
            else if (t == b) put(x, r, B_FULL, t, t);
            else put(x, r, B_TOP, t, b);
        }
}

/* a value 0..1 spread over n ramp entries, ordered-dithered */
static uint8_t dpick(int base, int n, float v, int x, int y) {
    float f = clampf(v, 0.0f, 1.0f) * (n - 1);
    int i = (int)f;
    if ((f - i) * 16.0f > BAYER4[y & 3][x & 3] + 0.5f) i++;
    return (uint8_t)(base + clampi(i, 0, n - 1));
}

static float vnoise(float x, float y, uint32_t s) {
    int x0 = (int)floorf(x), y0 = (int)floorf(y);
    float fx = x - x0, fy = y - y0;
    fx = fx * fx * (3 - 2 * fx); fy = fy * fy * (3 - 2 * fy);
    float a = hash01(x0, y0, (int)s), b = hash01(x0 + 1, y0, (int)s), c = hash01(x0, y0 + 1, (int)s), d = hash01(x0 + 1, y0 + 1, (int)s);
    return (a + (b - a) * fx) * (1 - fy) + (c + (d - c) * fx) * fy;
}
static float fbm2(float x, float y, uint32_t s) {
    return vnoise(x, y, s) * 0.55f + vnoise(x * 2.1f, y * 2.1f, s + 7) * 0.3f + vnoise(x * 4.3f, y * 4.3f, s + 13) * 0.15f;
}

static void fline(float x0, float y0, float x1, float y1, uint8_t c, int w) {
    int n = (int)(fmaxf(fabsf(x1 - x0), fabsf(y1 - y0)) * 1.5f) + 1;
    for (int i = 0; i <= n; i++) {
        float u = (float)i / n;
        int x = (int)floorf(x0 + (x1 - x0) * u + 0.5f), y = (int)floorf(y0 + (y1 - y0) * u + 0.5f);
        fset(x, y, c);
        if (w > 1) { fset(x + 1, y, c); fset(x, y + 1, c); }
        if (w > 2) { fset(x - 1, y, c); fset(x + 1, y + 1, c); }
    }
}


int art_events(void) { int e = g_events; g_events = 0; return e; }

/* ------------------------------------------------------------------ */
/* The manor on the hill, in a storm                                   */
/* ------------------------------------------------------------------ */

enum {
    M_SKY = 64, M_CLOUD = 72, M_MOON = 76, M_STONE = 79, M_ROOF = 84, M_WIN = 87, M_GROUND = 90,
    M_BLACK = 95, M_TOMB = 96, M_FENCE = 99, M_FENCE_HI = 100, M_RAIN = 101, M_BOLT = 102,
    M_FOG = 103, M_DOORLIGHT = 105, M_PATH = 106, M_HILLS = 107
};

static double g_last_t = -1.0;
static double g_next_bolt = 5.5, g_bolt_t = -10.0;
static float g_flash;
static float g_bolt[24][2]; static int g_nbolt;

static void manor_events(double t) {
    if (t < g_last_t) { g_next_bolt = 5.5; g_bolt_t = -10.0; }          /* a new showing */
    double c0 = fmod(g_last_t < 0 ? 0 : g_last_t, 30.0), c1 = fmod(t, 30.0);
    if (g_last_t >= 0 && t >= g_last_t && c0 < 12.0 && c1 >= 12.0) g_events |= 2;   /* the door creaks open */
    g_last_t = t;
    if (t >= g_next_bolt) {
        g_bolt_t = t;
        g_next_bolt = t + 8.0 + hash01((int)t, 3, 9) * 9.0;
        g_events |= 1;
        float x = 34.0f + hash01((int)t, 5, 1) * 28.0f, y = 0.0f;
        g_nbolt = 0;
        while (y < 30.0f && g_nbolt < 24) {
            g_bolt[g_nbolt][0] = x; g_bolt[g_nbolt][1] = y; g_nbolt++;
            y += 2.0f + hash01(g_nbolt, (int)t, 2) * 2.5f;
            x += (hash01(g_nbolt, (int)t, 3) - 0.5f) * 5.0f;
        }
    }
    double since = t - g_bolt_t;
    g_flash = 0.0f;
    if (since >= 0.0 && since < 0.5)
        g_flash = since < 0.07 ? 1.0f : (since < 0.13 ? 0.2f : (since < 0.22 ? 0.85f : 0.45f * (1.0f - (float)(since - 0.22) / 0.28f)));
}

static void tri_bg(float ax, float ay, int base_y, float half_base, uint8_t dark, uint8_t lit) {
    for (int y = (int)ay; y <= base_y; y++) {
        float hw = half_base * (y - ay) / (base_y - ay + 0.001f);
        for (int x = (int)floorf(ax - hw); x <= (int)ceilf(ax + hw); x++)
            bset(x, y, x + 0.5f > ax ? lit : dark);
    }
}

static void wall_bg(int x0, int y0, int x1, int y1) {
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) {
            int lit = x >= x1 - 1 ? 2 : (x == x0 ? 0 : 1);
            if (y % 3 == 0 && hash01(x, y, 4) > 0.3f) lit = lit > 0 ? lit - 1 : 0;
            if (hash01(x, y, 8) > 0.93f) lit = 0;
            bset(x, y, (uint8_t)(M_STONE + 1 + lit));
        }
}

static void window_px(int x, int y, int w, int h, float glow, int figure) {
    for (int yy = y; yy < y + h; yy++)
        for (int xx = x; xx < x + w; xx++) {
            uint8_t c = glow <= 0.05f ? (uint8_t)M_STONE : dpick(M_WIN, 3, glow * (yy == y ? 0.8f : 1.0f), xx, yy);
            if (figure && yy >= y + 1 && xx == x + w / 2) c = M_BLACK;
            if (figure && yy == y && xx == x + w / 2 && glow > 0.05f) c = M_BLACK;
            bset(xx, yy, c);
        }
}

static void tomb(int x, int base, int w, int h, int cross) {
    if (cross) {
        for (int y = base - h; y <= base; y++) bset(x + w / 2, y, M_TOMB + 1);
        for (int xx = x; xx < x + w; xx++) bset(xx, base - h + 1, M_TOMB + 2);
        return;
    }
    for (int y = base - h; y <= base; y++)
        for (int xx = x; xx < x + w; xx++) {
            if (y == base - h && (xx == x || xx == x + w - 1)) continue;
            bset(xx, y, (uint8_t)(M_TOMB + (xx == x ? 0 : (xx == x + w - 1 ? 2 : 1))));
        }
}

static void art_manor(double t) {
    manor_events(t);
    double c = fmod(t, 30.0);
    cv_begin();
    /* sky, moon, clouds */
    for (int y = 0; y < 38; y++)
        for (int x = 0; x < PW; x++) bset(x, y, dpick(M_SKY, 8, powf(y / 37.0f, 1.3f), x, y));
    const float mx = 66.0f, my = 8.0f, mr = 5.0f;
    for (int y = 0; y < 20; y++)
        for (int x = 54; x < 79; x++) {
            float dx = (x + 0.5f - mx) / mr, dy = (y + 0.5f - my) / (mr * 1.12f), d = sqrtf(dx * dx + dy * dy);
            if (d <= 1.0f) {
                float v = 0.95f - 0.35f * d * d + 0.15f * (dx - dy);
                if ((fabsf(dx + 0.35f) < 0.22f && fabsf(dy + 0.1f) < 0.25f) || (fabsf(dx - 0.3f) < 0.15f && fabsf(dy - 0.4f) < 0.15f)) v -= 0.4f;
                bset(x, y, dpick(M_MOON, 3, v, x, y));
            } else if (d < 1.5f) bset(x, y, dpick(M_SKY, 8, 0.75f - (d - 1.0f), x, y));
        }
    for (int y = 0; y < 26; y++)
        for (int x = 0; x < PW; x++) {
            float n = fbm2((x * 0.6f + (float)t * 0.9f) / 9.0f, y / 4.0f, 51) - 0.08f * (y > 14 ? (y - 14) * 0.3f : 0.0f);
            if (n < 0.54f) continue;
            float near_moon = 1.0f / (1.0f + ((x - mx) * (x - mx) + (y - my) * (y - my)) / 80.0f);
            bset(x, y, dpick(M_CLOUD, 4, (n - 0.54f) * 2.2f + near_moon * 0.6f, x, y));
        }
    /* far hills */
    for (int x = 0; x < PW; x++) {
        int h = 31 + (int)(2.5f * sinf(x * 0.08f + 1.0f) + 1.5f * sinf(x * 0.21f));
        for (int y = h; y < 38; y++) bset(x, y, M_HILLS);
    }
    /* the hill the manor stands on */
    for (int x = 0; x < PW; x++) {
        float top = 33.0f + (x > 50 ? (x - 50) * 0.18f : 0.0f) + (x < 6 ? (6 - x) * 0.4f : 0.0f) + 0.6f * sinf(x * 0.7f);
        for (int y = (int)top; y < PH; y++) {
            float v = 0.75f - (y - top) * 0.045f + (hash01(x, y, 6) - 0.5f) * 0.25f;
            bset(x, y, dpick(M_GROUND, 5, v, x, y));
        }
    }
    /* graveyard */
    tomb(52, 37, 3, 4, 0); tomb(56, 38, 3, 5, 1); tomb(60, 38, 3, 4, 0); tomb(64, 39, 2, 3, 0); tomb(57, 41, 3, 4, 0);
    tomb(48, 40, 2, 3, 0); tomb(62, 42, 3, 3, 1);
    /* the door, and the path down to the gate */
    float door = 0.0f;
    if (c >= 12.0 && c < 18.0) door = c < 13.5 ? (float)(c - 12.0) / 1.5f : (c < 16.5 ? 1.0f : 1.0f - (float)(c - 16.5) / 1.5f);
    for (int y = 34; y < PH; y++) {
        float cx = 30.5f + (y - 34) * 0.18f + sinf(y * 0.4f) * 0.6f, w = 1.0f + (y - 34) * 0.08f;
        for (int x = (int)(cx - w); x <= (int)(cx + w); x++) {
            float spill = door * fmaxf(0.0f, 1.0f - (y - 34) / 9.0f);
            bset(x, y, spill > 0.15f && BAYER4[y & 3][x & 3] < spill * 16 ? (uint8_t)M_DOORLIGHT : (uint8_t)M_PATH);
        }
    }
    /* the manor */
    wall_bg(16, 18, 44, 33);
    for (int y = 12; y <= 18; y++) {                                       /* mansard roof */
        float hw = 11.0f + (y - 12) * 0.7f;
        for (int x = (int)(30 - hw); x <= (int)(30 + hw); x++) bset(x, y, (uint8_t)(M_ROOF + ((y & 1) ? 0 : 1) + (x > 36 ? 1 : 0) - (x > 36 && (y & 1) == 0 ? 1 : 0)));
    }
    for (int x = 19; x <= 41; x += 4) bset(x, 11, M_ROOF + 2);              /* cresting */
    wall_bg(26, 6, 34, 18);                                                  /* central tower */
    tri_bg(30.0f, -1.0f, 5, 5.5f, M_ROOF, M_ROOF + 2);
    wall_bg(8, 10, 15, 33);                                                  /* left tower */
    tri_bg(11.5f, 1.0f, 9, 5.0f, M_ROOF, M_ROOF + 2);
    wall_bg(44, 16, 49, 33);                                                 /* right turret */
    tri_bg(46.5f, 9.0f, 15, 4.0f, M_ROOF, M_ROOF + 2);
    for (int y = 7; y <= 11; y++) { bset(20, y, M_STONE + 1); bset(21, y, M_STONE + 2); bset(39, y, M_STONE + 1); bset(40, y, M_STONE + 2); }
    for (int i = 0; i < 5; i++) {                                           /* chimney smoke */
        float u = (float)fmod(t * 0.25 + i * 0.2, 1.0);
        int sx = 40 + (int)(u * 9.0f + sinf(u * 7.0f + (float)t) * 1.5f), sy = 6 - (int)(u * 7.0f);
        if (u > 0.08f) { bset(sx, sy, M_CLOUD + 1); bset(sx + 1, sy, M_CLOUD); }
    }
    /* windows: a light carried from room to room, and a figure in the tower */
    static const int WX[10] = { 18, 22, 37, 41, 18, 22, 37, 41, 29, 11 };
    static const int WY[10] = { 21, 21, 21, 21, 27, 27, 27, 27, 9, 13 };
    int carrier = ((int)(t / 2.3)) % 4;
    for (int i = 0; i < 10; i++) {
        float fl = 0.62f + 0.18f * sinf((float)t * 3.0f + i * 1.7f) * sinf((float)t * 7.7f + i);
        float glow;
        if (i < 4) glow = i == carrier ? 1.0f : 0.0f;
        else if (i == 5 || i == 6) glow = 0.0f;
        else glow = fl;
        if (i == 9) continue;
        window_px(WX[i], WY[i], 2, i >= 8 ? 2 : 3, glow, 0);
    }
    {                                                                       /* the tower room, and who stands in it */
        float glow = 0.55f + 0.15f * sinf((float)t * 2.1f), who = (c > 20.0 && c < 26.0) ? 1.0f : 0.0f;
        if (who > 0.0f) glow = 0.9f;
        window_px(10, 13, 3, 4, glow, 0);
        if (who > 0.0f) { bset(11, 14, M_BLACK); bset(10, 15, M_BLACK); bset(11, 15, M_BLACK); bset(12, 15, M_BLACK); bset(11, 16, M_BLACK); bset(10, 16, M_BLACK); bset(12, 16, M_BLACK); }
    }
    window_px(11, 22, 2, 3, 0.55f, 0);
    window_px(46, 21, 1, 3, 0.7f, 0);
    for (int y = 29; y <= 33; y++)                                           /* front door */
        for (int x = 29; x <= 31; x++) {
            if (y == 29 && x != 30) continue;
            float open_w = door * 1.6f;
            bset(x, y, fabsf(x - 30.0f) < open_w ? (uint8_t)M_DOORLIGHT : (uint8_t)(M_ROOF + 1));
        }
    for (int x = 28; x <= 32; x++) bset(x, 34, M_STONE + 2);
    /* foreground: the dead tree and the iron fence */
    fline(73, 49, 71, 30, M_BLACK, 3); fline(71, 30, 70, 21, M_BLACK, 2);
    fline(71, 30, 64, 23, M_BLACK, 2); fline(64, 23, 59, 20, M_BLACK, 1);
    fline(71, 27, 77, 20, M_BLACK, 2); fline(77, 20, 79, 16, M_BLACK, 1);
    fline(70, 22, 66, 15, M_BLACK, 1); fline(66, 15, 62, 12, M_BLACK, 1); fline(66, 15, 68, 10, M_BLACK, 1);
    fline(70, 21, 73, 13, M_BLACK, 1); fline(73, 13, 75, 8, M_BLACK, 1); fline(62, 21, 61, 17, M_BLACK, 1);
    for (int x = 0; x < PW; x++) {
        if (x >= 27 && x <= 36) continue;
        fset(x, 45, M_FENCE); fset(x, 48, M_FENCE);
        if (x % 3 == 0) {
            for (int y = 44; y < PH; y++) fset(x, y, (uint8_t)(x % 6 == 0 ? M_FENCE_HI : M_FENCE));
            fset(x, x % 6 == 0 ? 42 : 43, M_FENCE_HI);
        }
    }
    fline(27, 44, 25, 49, M_FENCE_HI, 1); fline(37, 44, 39, 49, M_FENCE_HI, 1);    /* the gate stands open */
    /* bats leaving the tower */
    for (int i = 0; i < 7; i++) {                                         /* out of the attic window, some across the moon */
        double st = 3.0 + i * 0.8;
        float u = (float)((c - st) / 5.5);
        if (u < 0.0f || u > 1.0f) continue;
        float dir = (i & 1) ? 1.0f : -0.8f;
        int bx = (int)floorf(30.0f + dir * u * 44.0f + sinf(u * 7.0f + i) * 1.5f + 0.5f);
        int by = (int)floorf(10.0f - 4.5f * sinf(u * PI_F) + (i >> 1) * 1.2f + sinf(u * 11.0f + i) * 0.8f + 0.5f);
        if (((int)(t * 8.0 + i)) & 1) { fset(bx - 1, by - 1, M_BLACK); fset(bx - 2, by - 1, M_BLACK); fset(bx + 1, by - 1, M_BLACK); fset(bx + 2, by - 1, M_BLACK); }
        else { fset(bx - 1, by, M_BLACK); fset(bx - 2, by + 1, M_BLACK); fset(bx + 1, by, M_BLACK); fset(bx + 2, by + 1, M_BLACK); }
        fset(bx, by, M_BLACK);
    }
    /* lightning */
    if (g_flash > 0.5f)
        for (int i = 0; i + 1 < g_nbolt; i++) fline(g_bolt[i][0], g_bolt[i][1], g_bolt[i + 1][0], g_bolt[i + 1][1], M_BOLT, 1);
    /* rain and ground fog as character overlays */
    for (int i = 0; i < 90; i++) {
        float y = (float)fmod(hash01(i, 1, 21) * 25.0 + t * 13.0, 25.0);
        float x = (float)fmod(hash01(i, 2, 21) * 80.0 - t * 4.0 - y * 0.35 + 800.0, 80.0);
        ovset((int)x, (int)y, '/', M_RAIN);
    }
    for (int r = 20; r < TROWS; r++)
        for (int x = 0; x < TCOLS; x++) {
            float n = vnoise((x + (float)t * 0.9f) / 7.0f, r / 2.0f, 33);
            if (n > 0.58f) ovset(x, r, n > 0.72f ? B_MED : B_LITE, (uint8_t)(n > 0.66f ? M_FOG + 1 : M_FOG));
        }
    cv_flush();
}

static void manor_palette(uint8_t pal[256][3]) {
    static const uint8_t BASE[44][3] = {
        {1,0,3},{2,1,6},{4,2,9},{6,3,12},{8,5,15},{11,7,18},{14,9,21},{18,12,24},     /* sky      */
        {6,5,9},{10,9,14},{15,14,20},{22,21,28},                                      /* clouds   */
        {40,38,26},{52,50,36},{62,61,48},                                             /* moon     */
        {3,3,6},{6,6,10},{9,9,14},{13,13,19},{18,18,25},                              /* stone    */
        {2,2,4},{4,4,7},{7,7,11},                                                     /* roof     */
        {30,16,2},{50,32,6},{63,52,16},                                               /* windows  */
        {1,3,2},{2,5,3},{3,7,4},{5,10,6},{7,13,8},                                    /* ground   */
        {2,1,1},                                                                      /* black    */
        {10,10,13},{16,16,20},{24,24,29},                                             /* tombs    */
        {3,3,4},{10,10,13},                                                           /* fence    */
        {14,16,26},{55,58,63},                                                        /* rain, bolt */
        {8,8,12},{13,13,18},                                                          /* fog      */
        {63,40,10},{9,7,6},{3,2,6},                                                   /* door light, path, hills */
    };
    float f = g_flash;
    for (int i = 0; i < 44; i++) {
        float r = BASE[i][0], g = BASE[i][1], b = BASE[i][2];
        if (i < 12) { r += (24 - r) * f * 0.75f; g += (26 - g) * f * 0.75f; b += (44 - b) * f * 0.75f; }
        else if (i != M_WIN - 64 && i != M_WIN - 63 && i != M_WIN - 62 && i != M_BOLT - 64 && i != M_DOORLIGHT - 64) {
            r = r * (1.0f + 1.6f * f) + 4 * f; g = g * (1.0f + 1.6f * f) + 4 * f; b = b * (1.0f + 1.8f * f) + 7 * f;
        }
        pal[64 + i][0] = (uint8_t)clampi((int)r, 0, 63);
        pal[64 + i][1] = (uint8_t)clampi((int)g, 0, 63);
        pal[64 + i][2] = (uint8_t)clampi((int)b, 0, 63);
    }
}

/* ------------------------------------------------------------------ */
/* The spider in the candle-lit corner                                 */
/* ------------------------------------------------------------------ */

enum {
    S_WALL = 64, S_BEAM = 80, S_WEB = 84, S_SPIDER = 88, S_HOURGLASS = 94, S_EYES = 95,
    S_WAX = 96, S_FLAME = 98, S_COCOON = 101, S_DUST = 104, S_FLOOR = 105, S_BONE = 109, S_THREAD = 112
};

static float g_candle = 1.0f, g_eyes = 0.5f;

static float candle_flicker(double t) {
    return 0.86f + 0.09f * sinf((float)t * 13.0f) * sinf((float)t * 5.3f + 1.0f) + 0.05f * sinf((float)t * 31.0f);
}

/* The web is lettered the way it would be done by hand in an ANSI editor:
 * radials in | / - \, rings in underscores and stair-steps. It is built
 * once, in cell units; only its colours animate. */
static uint8_t g_webch[TROWS][TCOLS];
static float g_webr[TROWS][TCOLS], g_weba[TROWS][TCOLS];
static int g_web_ready;

#define HUB_C 24
#define HUB_R 9
static const float WEB_HUB[2] = { HUB_C + 0.5f, HUB_R * 2.0f + 1.0f };   /* canvas pixels */
/* radials: direction in cells, length in steps */
static const int SPOKE[8][3] = { {0, -1, 7}, {1, -1, 7}, {1, 0, 22}, {1, 1, 9}, {0, 1, 10}, {-1, 1, 10}, {-1, 0, 20}, {-1, -1, 7} };
static const float RING[5] = { 0.22f, 0.38f, 0.54f, 0.70f, 0.86f };
#define COCOON_X (HUB_C + 12.5f)                                            /* canvas pixels */
#define COCOON_Y 22.6f

static void wput(int c, int r, uint8_t ch) { if ((unsigned)c < TCOLS && r >= 2 && r < 21) g_webch[r][c] = ch; }
static int iround(float v) { return (int)floorf(v + 0.5f); }

/* one thread between two cell centres; flat runs use `flat` */
static void web_thread(float ax, float ay, float bx, float by, uint8_t flat) {
    if (ax > bx) { float t = ax; ax = bx; bx = t; t = ay; ay = by; by = t; }
    float dx = bx - ax, dy = by - ay, adx = fabsf(dx), ady = fabsf(dy);
    if (ady <= 0.25f * adx) {
        for (int c = iround(ax); c <= iround(bx); c++) wput(c, iround(ay + dy * (c - ax) / dx), flat);
    } else if (ady < 0.75f * adx) {                                         /* shallow: _/ or \_ stair-steps */
        int pr = iround(ay);
        for (int c = iround(ax); c <= iround(bx); c++) {
            int r = iround(ay + dy * (c - ax) / dx);
            if (r == pr) wput(c, r, '_');
            else if (r > pr) wput(c, r, '\\');
            else wput(c, pr, '/');
            pr = r;
        }
    } else {                                                                /* steep and diagonal, row by row */
        if (ay > by) { float t = ax; ax = bx; bx = t; t = ay; ay = by; by = t; dx = -dx; dy = -dy; }
        uint8_t diag = dx * dy > 0 ? '\\' : '/';
        int pc = iround(ax), steep = adx <= 0.75f * ady;
        for (int r = iround(ay); r <= iround(by); r++) {
            int c = iround(ax + dx * (r - ay) / dy);
            wput(c, r, (!steep || c != pc) ? diag : L_VERT);
            pc = c;
        }
    }
}

static void spoke_pt(int i, float f, float *x, float *y) {
    *x = HUB_C + SPOKE[i][0] * SPOKE[i][2] * f;
    *y = HUB_R + SPOKE[i][1] * SPOKE[i][2] * f;
}

static void web_build(void) {
    memset(g_webch, 0, sizeof g_webch);
    for (int k = 0; k < 5; k++)                                             /* the rings */
        for (int i = 0; i < 8; i++) {
            int j = (i + 1) % 8;
            if ((i == 2 || i == 3) && k >= 3) continue;                     /* torn out */
            if (hash01(k, i, 91) < 0.08f) continue;
            float x0, y0, x1, y1;
            spoke_pt(i, RING[k], &x0, &y0); spoke_pt(j, RING[k], &x1, &y1);
            web_thread(x0, y0, x1, y1, '_');
        }
    for (int i = 0; i < 8; i++) {                                           /* the radials */
        float x, y, f = i == 3 ? 0.62f : 1.0f;
        spoke_pt(i, f, &x, &y);
        web_thread((float)HUB_C, (float)HUB_R, x, y, L_HORZ);
        if (i == 3) { wput(iround(x), iround(y) + 1, L_VERT); wput(iround(x), iround(y) + 2, ','); }
    }
    {                                                                       /* a torn ring left dangling */
        float x, y;
        spoke_pt(2, RING[3], &x, &y);
        wput(iround(x) - 1, iround(y) + 1, L_VERT); wput(iround(x) - 1, iround(y) + 2, L_VERT); wput(iround(x) - 1, iround(y) + 3, '\'');
    }
    wput(HUB_C, HUB_R, '*');
    static const int STRAND[5][2] = { {49, 3}, {57, 2}, {63, 4}, {71, 2}, {76, 3} };
    for (int i = 0; i < 5; i++) {                                           /* old dusty strands off the beam */
        for (int r = 2; r < 2 + STRAND[i][1]; r++) wput(STRAND[i][0], r, L_VERT);
        wput(STRAND[i][0], 2 + STRAND[i][1], '\'');
    }
    for (int r = 0; r < TROWS; r++)
        for (int c = 0; c < TCOLS; c++) {
            float mx = (c - HUB_C) * 9.0f, my = (r - HUB_R) * 16.0f;
            g_webr[r][c] = sqrtf(mx * mx + my * my);
            g_weba[r][c] = atan2f(my, mx);
        }
    g_web_ready = 1;
}

/* Her shadow on the wall: the same drawing, projected away from the flame. */
static uint8_t g_shad[PH][PW];
static int g_to_shadow;

static void sp_px(int x, int y, uint8_t c) {
    if ((unsigned)x >= PW || (unsigned)y >= PH) return;
    if (g_to_shadow) g_shad[y][x] = 1; else cfg[y][x] = c;
}
static void sp_line(float x0, float y0, float x1, float y1, uint8_t c, int w) {
    int n = (int)(fmaxf(fabsf(x1 - x0), fabsf(y1 - y0)) * 1.5f) + 1;
    for (int i = 0; i <= n; i++) {
        float u = (float)i / n;
        int x = (int)floorf(x0 + (x1 - x0) * u + 0.5f), y = (int)floorf(y0 + (y1 - y0) * u + 0.5f);
        sp_px(x, y, c);
        if (w > 1) { sp_px(x + 1, y, c); sp_px(x, y + 1, c); }
        if (w > 2) { sp_px(x - 1, y, c); sp_px(x + 1, y + 1, c); sp_px(x, y - 1, c); }
    }
}
static float shad_at(int x, int y) {
    float s = 0;
    for (int j = -1; j <= 1; j++)
        for (int i = -1; i <= 1; i++) {
            int xx = x + i, yy = y + j;
            if ((unsigned)xx < PW && (unsigned)yy < PH && g_shad[yy][xx]) s += (float)((2 - abs(i)) * (2 - abs(j)));
        }
    return s / 16.0f;
}

typedef struct { float x, y, s, th, walk; } SpPose;

static const float CANDLE_X = 66.0f, CANDLE_Y = 29.0f;

/* candlelight reaching a point of the room, 0..~1 */
static float candle_light(float x, float y) {
    float d2 = (x - CANDLE_X) * (x - CANDLE_X) + (y - CANDLE_Y) * (y - CANDLE_Y) * 1.3f;
    return g_candle / (1.0f + d2 / 560.0f);
}

/* A shaded ellipse of her body, in her own frame (head towards -y). */
static void sp_ell(const SpPose *p, float lx, float ly, float rx, float ry, float sheen) {
    float c = cosf(p->th), s = sinf(p->th);
    float cx = p->x + p->s * (lx * c - ly * s), cy = p->y + p->s * (lx * s + ly * c);
    float R = fmaxf(rx, ry) * p->s + 1.0f;
    float att = g_candle / (1.0f + ((cx - CANDLE_X) * (cx - CANDLE_X) + (cy - CANDLE_Y) * (cy - CANDLE_Y)) / 1400.0f);
    float Lx = CANDLE_X - cx, Ly = CANDLE_Y - cy, Ll = sqrtf(Lx * Lx + Ly * Ly) + 1e-3f;
    Lx /= Ll; Ly /= Ll;
    for (int y = (int)floorf(cy - R); y <= (int)ceilf(cy + R); y++)
        for (int x = (int)floorf(cx - R); x <= (int)ceilf(cx + R); x++) {
            float dx = (x + 0.5f - cx) / p->s, dy = (y + 0.5f - cy) / p->s;
            float u = (dx * c + dy * s) / rx, v = (-dx * s + dy * c) / ry, d = u * u + v * v;
            if (d > 1.0f) continue;
            float nx = u * c - v * s, ny = u * s + v * c;                 /* outward normal, world frame */
            float face = fmaxf(0.0f, nx * Lx + ny * Ly);
            float val = 0.05f + sheen * (1.0f - d) + att * 1.05f * face * face * (0.4f + 0.6f * d);
            if (face > 0.7f && d > 0.2f && d < 0.6f) val += 0.25f * att;  /* the gloss on her back */
            sp_px(x, y, dpick(S_SPIDER, 6, val, x, y));
        }
}

static void sp_at(const SpPose *p, float lx, float ly, float *wx, float *wy) {
    float c = cosf(p->th), s = sinf(p->th);
    *wx = p->x + p->s * (lx * c - ly * s);
    *wy = p->y + p->s * (lx * s + ly * c);
}

static void draw_spider(const SpPose *p) {
    static const float A1[4] = { -0.75f, -0.30f, 0.10f, 0.50f }, A2[4] = { -1.35f, -0.70f, 0.55f, 1.15f };
    static const float L1[4] = { 3.6f, 3.4f, 3.2f, 3.6f }, L2[4] = { 4.4f, 4.0f, 3.8f, 4.6f };
    int w = p->s < 1.5f ? 1 : (p->s < 2.6f ? 2 : 3);
    float att = candle_light(p->x, p->y);
    uint8_t leg = dpick(S_SPIDER, 6, 0.12f + 0.45f * att, 0, 0), knee = dpick(S_SPIDER, 6, 0.2f + 0.8f * att, 0, 0);
    for (int side = -1; side <= 1; side += 2)
        for (int i = 0; i < 4; i++) {
            float wig = sinf(p->walk + (float)(((i + (side > 0)) & 1)) * PI_F + i * 0.4f) * 0.18f;
            float a1 = A1[i] + wig, a2 = A2[i] + wig * 1.4f;
            float cxl = side * 1.0f, cyl = -1.4f + (i - 1.5f) * 0.55f;
            float kxl = cxl + side * L1[i] * cosf(a1), kyl = cyl + L1[i] * sinf(a1);
            float fxl = kxl + side * L2[i] * cosf(a2), fyl = kyl + L2[i] * sinf(a2);
            float x0, y0, x1, y1, x2, y2;
            sp_at(p, cxl, cyl, &x0, &y0); sp_at(p, kxl, kyl, &x1, &y1); sp_at(p, fxl, fyl, &x2, &y2);
            sp_line(x0, y0, x1, y1, leg, w);
            sp_line(x1, y1, x2, y2, leg, w > 1 ? w - 1 : 1);
            sp_px((int)floorf(x1 + 0.5f), (int)floorf(y1 + 0.5f), knee);
        }
    sp_ell(p, 0.0f, 2.4f, 2.4f, 3.0f, 0.10f);                               /* abdomen */
    sp_ell(p, 0.0f, -1.5f, 1.6f, 1.5f, 0.06f);                              /* head */
    if (g_to_shadow) return;
    float x, y;
    if (p->s < 1.5f) {                                                      /* the red mark, the eyes */
        sp_at(p, 0.0f, 2.2f, &x, &y); sp_px((int)x, (int)y, S_HOURGLASS);
        sp_at(p, 0.0f, 3.2f, &x, &y); sp_px((int)x, (int)y, S_HOURGLASS);
        sp_at(p, -0.95f, -2.3f, &x, &y); sp_px((int)floorf(x + 0.5f), (int)floorf(y + 0.5f), S_EYES);
        sp_at(p, 0.95f, -2.3f, &x, &y); sp_px((int)floorf(x + 0.5f), (int)floorf(y + 0.5f), S_EYES);
        return;
    }
    {                                                                       /* the hourglass */
        float c = cosf(p->th), s = sinf(p->th), hx, hy, R = 1.3f * p->s + 1.0f;
        sp_at(p, 0.0f, 2.6f, &hx, &hy);
        for (int yy = (int)floorf(hy - R); yy <= (int)ceilf(hy + R); yy++)
            for (int xx = (int)floorf(hx - R); xx <= (int)ceilf(hx + R); xx++) {
                float dx = (xx + 0.5f - hx) / p->s, dy = (yy + 0.5f - hy) / p->s;
                float u = dx * c + dy * s, v = (-dx * s + dy * c) / 1.15f;
                if (fabsf(v) <= 1.0f && fabsf(u) <= 0.12f + fabsf(v) * 0.5f) sp_px(xx, yy, S_HOURGLASS);
            }
    }
    static const float EYE[6][2] = { {-0.45f, -2.6f}, {0.45f, -2.6f}, {-1.3f, -2.4f}, {1.3f, -2.4f}, {-0.85f, -1.9f}, {0.85f, -1.9f} };
    for (int i = 0; i < 6; i++) {
        sp_at(p, EYE[i][0], EYE[i][1], &x, &y);
        sp_px((int)floorf(x + 0.5f), (int)floorf(y + 0.5f), S_EYES);
    }
    for (int side = -1; side <= 1; side += 2) {                             /* fangs */
        float x0, y0, x1, y1;
        sp_at(p, side * 0.45f, -3.0f, &x0, &y0); sp_at(p, side * 0.2f, -3.7f, &x1, &y1);
        sp_line(x0, y0, x1, y1, (uint8_t)(S_SPIDER + 3), 1);
    }
}

static SpPose spider_pose(double t, float c) {
    const float hx = WEB_HUB[0], hy = WEB_HUB[1];
    const float bx = COCOON_X - 3.5f, by = hy;                              /* beside the fly */
    const float out = 1.85f, dx = 38.0f, dy = 23.0f, ds = 2.3f;
    float tf = (float)t;
    SpPose p = { hx, hy, 1.0f, 0.0f, sinf(tf * 1.3f) * 0.3f };
    if (c < 5.0f) {}
    else if (c < 5.6f) { p.th = out * smoothstepf(5.0f, 5.6f, c); p.walk = tf * 7.0f; }
    else if (c < 9.0f) { float u = smoothstepf(5.6f, 9.0f, c); p.x = lerpf(hx, bx, u); p.y = lerpf(hy, by, u); p.th = out; p.walk = tf * 11.0f; }
    else if (c < 11.5f) { p.x = bx; p.y = by; p.th = out; p.walk = sinf(tf * 24.0f) * 0.6f; }
    else if (c < 12.1f) { p.x = bx; p.y = by; p.th = out + PI_F * smoothstepf(11.5f, 12.1f, c); p.walk = tf * 7.0f; }
    else if (c < 15.0f) { float u = smoothstepf(12.1f, 15.0f, c); p.x = lerpf(bx, hx, u); p.y = lerpf(by, hy, u); p.th = out + PI_F; p.walk = tf * 11.0f; }
    else if (c < 15.6f) { p.th = lerpf(out + PI_F, TAU_F, smoothstepf(15.0f, 15.6f, c)); p.walk = tf * 7.0f; }
    else if (c < 17.0f) {}
    else if (c < 17.8f) { p.th = PI_F * smoothstepf(17.0f, 17.8f, c); p.walk = tf * 7.0f; }
    else if (c < 23.0f) {                                                   /* coming down to see you */
        float u = smoothstepf(17.8f, 23.0f, c);
        p.x = lerpf(hx, dx, u); p.y = lerpf(hy, dy, u); p.s = lerpf(1.0f, ds, u); p.th = PI_F; p.walk = sinf(tf * 2.0f) * 0.3f;
    } else if (c < 25.5f) {
        float sway = smoothstepf(23.0f, 23.8f, c) * (1.0f - smoothstepf(24.8f, 25.5f, c));
        p.x = dx + sinf(tf * 1.4f) * 0.8f * sway; p.y = dy; p.s = ds; p.th = PI_F; p.walk = sinf(tf * 3.0f) * 0.5f;
    } else if (c < 27.5f) {
        float u = smoothstepf(25.5f, 27.5f, c);
        p.x = lerpf(dx, hx, u); p.y = lerpf(dy, hy, u); p.s = lerpf(ds, 1.0f, u); p.th = PI_F; p.walk = tf * 4.0f;
    } else if (c < 28.3f) { p.th = PI_F + PI_F * smoothstepf(27.5f, 28.3f, c); p.walk = tf * 7.0f; }
    return p;
}

static void art_spider(double t) {
    if (!g_web_ready) web_build();
    if (t < g_last_t) g_last_t = -1.0;
    float c = (float)fmod(t, 30.0), c_prev = g_last_t < 0 ? -1.0f : (float)fmod(g_last_t, 30.0);
    if (c_prev >= 0 && c_prev < 17.8f && c >= 17.8f) g_events |= ART_EV_SPIDER;    /* she lets go */
    g_last_t = t;
    g_candle = candle_flicker(t);
    SpPose sp = spider_pose(t, c);
    g_eyes = clampf((sp.s - 1.0f) / 1.3f, 0.0f, 1.0f);
    float cocx = COCOON_X, cocy = COCOON_Y;
    if (c > 9.0f && c < 11.5f) cocx += sinf((float)t * 40.0f) * 0.7f;

    cv_begin();
    memset(g_shad, 0, sizeof g_shad);
    g_to_shadow = 1;                                                        /* her shadow, thrown by the flame */
    {
        float k = 1.06f + 0.55f * (sp.s - 1.0f) / 1.3f;
        SpPose sh = sp;
        sh.x = CANDLE_X + (sp.x - CANDLE_X) * k; sh.y = CANDLE_Y + (sp.y - CANDLE_Y) * k; sh.s = sp.s * k;
        draw_spider(&sh);
        SpPose cs = { CANDLE_X + (cocx - CANDLE_X) * 1.06f, CANDLE_Y + (cocy - CANDLE_Y) * 1.06f, 1.06f, 0.0f, 0.0f };
        sp_ell(&cs, 0.0f, 0.0f, 1.5f, 2.6f, 0.0f);
    }
    g_to_shadow = 0;

    for (int y = 4; y < 42; y++)                                            /* stone wall */
        for (int x = 4; x < PW; x++) {
            int row = y / 4, bx = (x + (row & 1) * 4) / 9;
            int mortar = (y % 4 == 0) || ((x + (row & 1) * 4) % 9 == 0);
            float L = candle_light((float)x, (float)y) * (1.0f - 0.6f * shad_at(x, y)) + 0.04f;
            float grime = 0.75f + 0.5f * vnoise(x * 0.18f, y * 0.22f, 11);
            float v = L * grime * (mortar ? 0.45f : 0.85f + 0.25f * hash01(bx, row, 4)) + (hash01(x, y, 5) - 0.5f) * 0.06f;
            bset(x, y, dpick(S_WALL, 16, v, x, y));
        }
    for (int y = 0; y < 4; y++)                                             /* beam overhead, post in the corner */
        for (int x = 0; x < PW; x++) bset(x, y, (uint8_t)(S_BEAM + (y == 3 ? 0 : (y == 0 ? 1 : 2)) + (hash01(x / 3, y, 9) > 0.8f ? -1 : 0)));
    for (int y = 0; y < 42; y++)
        for (int x = 0; x < 4; x++) bset(x, y, (uint8_t)(S_BEAM + (x == 3 ? 0 : (x == 0 ? 1 : 2))));
    for (int y = 42; y < PH; y++)                                           /* floor boards */
        for (int x = 0; x < PW; x++) {
            float d2 = (x - CANDLE_X) * (x - CANDLE_X) * 0.5f + (y - 42.0f) * (y - 42.0f) * 6.0f;
            float L = g_candle * (1.0f - 0.5f * shad_at(x, y)) / (1.0f + d2 / 300.0f) + 0.05f;
            float v = (y % 2 == 0) ? L * 0.5f : L * (0.8f + 0.2f * hash01(x / 11, y, 3));
            bset(x, y, dpick(S_FLOOR, 4, v, x, y));
        }

    /* the candle in its dish */
    for (int x = 62; x <= 70; x++) bset(x, 41, S_BEAM + 3);
    bset(61, 41, S_BEAM + 1); bset(71, 41, S_BEAM + 1);
    for (int y = 32; y <= 40; y++)
        for (int x = 65; x <= 67; x++) bset(x, y, (uint8_t)(x == 67 ? S_WAX : S_WAX + 1));
    bset(68, 34, S_WAX + 1); bset(68, 35, S_WAX); bset(64, 37, S_WAX); bset(64, 38, S_WAX); bset(68, 40, S_WAX);
    fset(66, 31, S_SPIDER);
    int lean = g_candle > 0.94f ? 1 : 0;
    fset(66, 30, (uint8_t)(S_FLAME + 1)); fset(66, 29, (uint8_t)(S_FLAME + (g_candle > 0.9f ? 2 : 1)));
    fset(66 + lean, 28, S_FLAME); if (g_candle > 0.88f) fset(66 + lean, 27, S_FLAME);
    fset(65, 30, S_FLAME); fset(67, 30, S_FLAME);

    /* last year's visitor */
    static const char *SKULL[6] = { ".#####.", "#######", "#oo#oo#", "###^###", ".#####.", ".#.#.#." };
    for (int r = 0; r < 6; r++)
        for (int k = 0; k < 7; k++) {
            char ch = SKULL[r][k];
            if (ch == '.') continue;
            int x = 71 + k, y = 42 + r;
            float lit = (1.0f - k / 7.0f) * 0.7f + (r == 0 ? 0.3f : 0.0f);
            bset(x, y, ch == '#' ? (uint8_t)(S_BONE + (lit > 0.55f ? 2 : (lit > 0.25f ? 1 : 0))) : (uint8_t)S_FLOOR);
        }

    /* the web, coloured by the candle and a glint that slides along the silk */
    for (int r = 0; r < TROWS; r++)
        for (int x = 0; x < TCOLS; x++) {
            if (!g_webch[r][x]) continue;
            float L = candle_light(x + 0.5f, r * 2.0f + 1.0f) * (1.0f - 0.6f * shad_at(x, r * 2 + 1));
            float b = 1.0f + 3.0f * L;
            if (sinf(g_webr[r][x] * 0.028f - (float)t * 1.1f + g_weba[r][x] * 2.0f) > 0.9f) b += 1.5f;
            ovset(x, r, g_webch[r][x], (uint8_t)(S_WEB + clampi((int)b, 0, 3)));
        }

    /* the fly, wrapped */
    for (int y = (int)(cocy - 3); y <= (int)(cocy + 3); y++)
        for (int x = (int)(cocx - 2); x <= (int)(cocx + 2); x++) {
            float u = (x + 0.5f - cocx) / 1.5f, v = (y + 0.5f - cocy) / 2.6f, d = u * u + v * v;
            if (d > 1.0f) continue;
            float val = 0.35f + 0.4f * (u * 0.7f - v * 0.3f + 0.3f) + 0.25f * (1.0f - d);
            fset(x, y, dpick(S_COCOON, 3, val, x, y));
        }

    /* her dragline, when she comes down */
    if (c >= 17.8f && c < 27.5f) {
        float tx, ty;
        sp_at(&sp, 0.0f, 5.3f, &tx, &ty);
        for (int r = 2; r < (int)(ty / 2.0f); r++) ovset((int)tx, r, L_VERT, S_THREAD);
    }
    draw_spider(&sp);

    for (int i = 0; i < 16; i++) {                                          /* dust drifting through the light */
        float dx = 50.0f + hash01(i, 1, 41) * 28.0f + sinf((float)t * 0.3f + i) * 4.0f;
        float dy = (float)fmod(hash01(i, 2, 41) * 40.0 - t * (0.6 + hash01(i, 3, 41)) + 400.0, 40.0);
        ovset((int)dx, (int)(dy / 2.0f), 0xFA, S_DUST);
    }
    cv_flush();
}

static void spider_palette(uint8_t pal[256][3]) {
    float fl = g_candle;
    for (int i = 0; i < 16; i++) {                                          /* candle-lit stone: brown-black to warm ochre */
        float v = i / 15.0f;
        pal[S_WALL + i][0] = (uint8_t)clampi((int)(2 + 44 * v * fl), 0, 63);
        pal[S_WALL + i][1] = (uint8_t)clampi((int)(1 + 32 * v * fl), 0, 63);
        pal[S_WALL + i][2] = (uint8_t)clampi((int)(2 + 20 * v * v * fl), 0, 63);
    }
    static const uint8_t REST[S_THREAD + 1 - S_BEAM][3] = {
        {3,2,1},{6,4,2},{10,7,4},{18,13,8},                 /* beam, dish  */
        {15,14,15},{25,24,24},{38,36,33},{60,58,52},        /* web         */
        {2,1,3},{5,3,6},{11,6,9},{24,13,12},{42,25,16},{58,44,30}, /* spider */
        {52,6,4},{63,14,8},                                 /* hourglass, eyes */
        {40,36,28},{56,52,42},                              /* wax         */
        {63,24,2},{63,46,10},{63,62,40},                    /* flame       */
        {20,18,16},{32,30,26},{46,43,38},                   /* cocoon      */
        {32,28,20},                                         /* dust        */
        {2,1,1},{5,3,2},{9,6,3},{14,9,5},                   /* floor       */
        {16,12,8},{30,24,16},{46,38,26},                    /* bone        */
        {44,44,46},                                         /* her thread  */
    };
    for (int i = 0; i < S_THREAD + 1 - S_BEAM; i++)
        for (int k = 0; k < 3; k++) pal[S_BEAM + i][k] = REST[i][k];
    float e = 0.55f + 0.45f * g_eyes;
    pal[S_EYES][0] = (uint8_t)clampi((int)(63 * e), 0, 63);
    pal[S_EYES][1] = (uint8_t)clampi((int)(14 * e), 0, 63);
    pal[S_EYES][2] = (uint8_t)clampi((int)(8 * e), 0, 63);
    for (int k = 0; k < 3; k++) {
        pal[S_FLAME + 0][k] = (uint8_t)clampi((int)(pal[S_FLAME + 0][k] * fl), 0, 63);
        pal[S_FLAME + 1][k] = (uint8_t)clampi((int)(pal[S_FLAME + 1][k] * fl), 0, 63);
        for (int i = 0; i < 3; i++) pal[S_BONE + i][k] = (uint8_t)clampi((int)(pal[S_BONE + i][k] * fl), 0, 63);
    }
}

/* ------------------------------------------------------------------ */
/* Shared pieces for the zombies, the skeletons and the ghost          */
/* ------------------------------------------------------------------ */

static double cyc_prev, cyc_now;
/* where we are in a looping timeline, remembering the previous frame so events fire once */
static double cycle_begin(double t, double period) {
    if (t < g_last_t) g_last_t = -1.0;
    cyc_prev = g_last_t < 0 ? -1.0 : fmod(g_last_t, period);
    cyc_now = fmod(t, period);
    if (cyc_now < cyc_prev) cyc_prev = -1.0;
    g_last_t = t;
    return cyc_now;
}
static void ev(double at, int bit) { if (cyc_prev < at && cyc_now >= at) g_events |= bit; }

static float g_clip = 1e9f;                         /* foreground rows below this are hidden (in the earth) */
static void cset(int x, int y, uint8_t c) { if (y <= g_clip) fset(x, y, c); }

/* a lit ball on the foreground; (lx, ly) points towards the light */
static void f_ell(float cx, float cy, float rx, float ry, int base, int n, float lx, float ly, float lo, float hi) {
    for (int y = (int)floorf(cy - ry); y <= (int)ceilf(cy + ry); y++)
        for (int x = (int)floorf(cx - rx); x <= (int)ceilf(cx + rx); x++) {
            float nx = (x + 0.5f - cx) / rx, ny = (y + 0.5f - cy) / ry, d = nx * nx + ny * ny;
            if (d > 1.0f) continue;
            float l = clampf(0.5f + 0.6f * (nx * lx + ny * ly), 0.0f, 1.0f) * (0.75f + 0.25f * (1.0f - d));
            cset(x, y, dpick(base, n, lo + (hi - lo) * l, x, y));
        }
}

/* a lit limb; thin ones become a plain line so they never vanish */
static void f_cap(float x0, float y0, float x1, float y1, float r, int base, int n, float lx, float ly, float lo, float hi) {
    if (r < 0.75f) {
        int k = (int)(fmaxf(fabsf(x1 - x0), fabsf(y1 - y0)) * 1.5f) + 1;
        for (int i = 0; i <= k; i++) {
            float u = (float)i / k;
            int x = (int)floorf(x0 + (x1 - x0) * u + 0.5f), y = (int)floorf(y0 + (y1 - y0) * u + 0.5f);
            cset(x, y, dpick(base, n, (lo + hi) * 0.5f, x, y));
        }
        return;
    }
    float vx = x1 - x0, vy = y1 - y0, len2 = vx * vx + vy * vy + 1e-6f;
    for (int y = (int)floorf(fminf(y0, y1) - r); y <= (int)ceilf(fmaxf(y0, y1) + r); y++)
        for (int x = (int)floorf(fminf(x0, x1) - r); x <= (int)ceilf(fmaxf(x0, x1) + r); x++) {
            float px = x + 0.5f, py = y + 0.5f;
            float u = clampf(((px - x0) * vx + (py - y0) * vy) / len2, 0.0f, 1.0f);
            float ox = (px - (x0 + vx * u)) / r, oy = (py - (y0 + vy * u)) / r;
            if (ox * ox + oy * oy > 1.0f) continue;
            cset(x, y, dpick(base, n, lo + (hi - lo) * clampf(0.5f + 0.6f * (ox * lx + oy * ly), 0.0f, 1.0f), x, y));
        }
}

/* a plain foreground line, w pixels thick */
static void f_line(float x0, float y0, float x1, float y1, uint8_t c, int w) {
    int k = (int)(fmaxf(fabsf(x1 - x0), fabsf(y1 - y0)) * 1.5f) + 1;
    for (int i = 0; i <= k; i++) {
        float u = (float)i / k;
        int x = (int)floorf(x0 + (x1 - x0) * u + 0.5f), y = (int)floorf(y0 + (y1 - y0) * u + 0.5f);
        cset(x, y, c);
        if (w > 1) { cset(x + 1, y, c); cset(x, y + 1, c); }
    }
}

/* ------------------------------------------------------------------ */
/* The graveyard: they dig themselves out and come for you             */
/* ------------------------------------------------------------------ */

enum {
    Z_SKY = 64, Z_MOON = 72, Z_CLOUD = 75, Z_HILL = 78, Z_GRASS = 79, Z_STONE = 84, Z_DIRT = 89,
    Z_SKIN = 93, Z_CLOTH = 98, Z_EYE = 102, Z_MOUTH = 103, Z_BLACK = 104, Z_FOG = 105, Z_MAUS = 107,
    Z_LAMP = 111, Z_TEETH = 112, Z_STAR = 113
};
#define Z_HORIZON 24.0f
static float g_zeyes;

/* a zombie standing with its feet at (x, yf), s = 1 is about 21 pixels tall */
static void zombie(float x, float yf, float s, float walk, float reach, float mouth, float sway, uint32_t seed) {
    const float lx = 0.55f, ly = -0.83f;                           /* moonlight, from the upper right */
    float st = sinf(walk), hip = yf - 10.5f * s, sh = yf - 17.2f * s, hd = yf - 20.6f * s;
    float bx = x + sway * s;
    f_cap(bx - 1.4f * s, hip, x - 1.9f * s + st * 1.1f * s, yf - 0.4f * s, 1.15f * s, Z_CLOTH, 4, lx, ly, 0.0f, 0.5f);
    f_cap(bx + 1.4f * s, hip, x + 1.9f * s - st * 1.1f * s, yf - 0.4f * s, 1.15f * s, Z_CLOTH, 4, lx, ly, 0.0f, 0.5f);
    float tcx = bx, tcy = (hip + sh) * 0.5f + 0.6f * s, trx = 3.3f * s, try_ = 4.4f * s;   /* the torso, in rags */
    for (int y = (int)floorf(tcy - try_); y <= (int)ceilf(tcy + try_); y++)
        for (int xx = (int)floorf(tcx - trx); xx <= (int)ceilf(tcx + trx); xx++) {
            float nx = (xx + 0.5f - tcx) / trx, ny = (y + 0.5f - tcy) / try_, d = nx * nx + ny * ny;
            if (d > 1.0f) continue;
            float h = hash01((int)((xx - bx) / fmaxf(s, 0.5f)), (int)((y - yf) / fmaxf(s, 0.5f)), (int)seed);
            if (ny > 0.65f && h < 0.45f) continue;                     /* ragged hem */
            float l = clampf(0.5f + 0.6f * (nx * lx + ny * ly), 0.0f, 1.0f);
            if (h > 0.82f) cset(xx, y, dpick(Z_SKIN, 5, 0.25f + 0.6f * l, xx, y));   /* rotten skin through the holes */
            else cset(xx, y, dpick(Z_CLOTH, 4, 0.2f + 0.8f * l, xx, y));
            if (fabsf(xx + 0.5f - tcx) < 0.6f * s && ny < 0.2f && ny > -0.8f) cset(xx, y, dpick(Z_SKIN, 5, 0.3f + 0.4f * l, xx, y));   /* open shirt */
        }
    float hx = bx + sinf(walk * 0.5f) * 0.5f * s;                   /* the head, lolling */
    f_ell(hx, hd, 2.5f * s, 2.9f * s, Z_SKIN, 5, lx, ly, 0.1f, 0.95f);
    for (int side = -1; side <= 1; side += 2) {
        float ex = hx + side * 1.0f * s, ey = hd - 0.3f * s;
        if (s > 1.4f) f_ell(ex, ey, 0.8f * s, 0.7f * s, Z_SKIN, 5, lx, ly, 0.0f, 0.15f);   /* sunken sockets */
        cset((int)floorf(ex + 0.5f), (int)floorf(ey + 0.5f), Z_EYE);
        if (s > 2.2f) cset((int)floorf(ex + 0.5f) + side, (int)floorf(ey + 0.5f), Z_EYE);
    }
    float my = hd + 1.5f * s, mw = 0.9f * s, mh = (0.35f + mouth * 0.75f) * s;
    if (s > 1.0f) {
        for (int y = (int)floorf(my - mh); y <= (int)ceilf(my + mh); y++)
            for (int xx = (int)floorf(hx - mw); xx <= (int)ceilf(hx + mw); xx++) {
                float nx = (xx + 0.5f - hx) / mw, ny = (y + 0.5f - my) / mh;
                if (nx * nx + ny * ny <= 1.0f) cset(xx, y, (s > 1.8f && y == (int)floorf(my - mh + 0.5f) && (xx & 1)) ? (uint8_t)Z_TEETH : (uint8_t)Z_MOUTH);
            }
    } else cset((int)floorf(hx + 0.5f), (int)floorf(my + 0.5f), Z_MOUTH);
    for (int i = 0; i < 5; i++)                                      /* lank hair */
        cset((int)floorf(hx + (i - 2) * 0.9f * s + 0.5f), (int)floorf(hd - 2.6f * s + (i & 1) * 0.6f * s), Z_BLACK);
    for (int side = -1; side <= 1; side += 2) {                     /* arms reaching out at you, in front of it all */
        float sx = bx + side * 3.1f * s, sy = sh + 0.8f * s;
        float hx2 = bx + side * (4.6f + reach * 0.6f) * s + sinf(walk * 0.5f + side) * 0.4f * s;
        float hy = sh + (2.2f - reach * 2.6f) * s + (side > 0 ? 0.4f : -0.2f) * s;
        float ex = sx + side * 1.2f * s, ey = (sy + hy) * 0.5f + 0.8f * s;
        f_cap(sx, sy, ex, ey, 1.05f * s, Z_CLOTH, 4, lx, ly, 0.15f, 0.75f);
        f_cap(ex, ey, hx2, hy, 0.85f * s, Z_SKIN, 5, lx, ly, 0.3f, 0.95f);
        f_ell(hx2, hy, 1.35f * s, 1.15f * s, Z_SKIN, 5, lx, ly, 0.35f, 1.0f);
        for (int f = -1; f <= 2; f++)                                /* hooked fingers */
            f_line(hx2 + (f - 0.5f) * 0.7f * s, hy - 0.6f * s, hx2 + (f - 0.5f) * 0.85f * s + side * 0.2f * s, hy - 2.0f * s, (uint8_t)(Z_SKIN + 3), s > 1.8f ? 2 : 1);
    }
}

static void headstone(int cx, int base, int w, int h, int cross) {
    const float lx = 0.55f;
    if (cross) {
        for (int y = base - h; y <= base; y++) bset(cx, y, Z_STONE + 2), bset(cx + 1, y, Z_STONE + 1);
        for (int x = cx - 2; x <= cx + 3; x++) bset(x, base - h + 2, (uint8_t)(Z_STONE + (x > cx ? 1 : 3)));
        return;
    }
    for (int y = base - h; y <= base; y++)
        for (int x = cx - w / 2; x <= cx + w / 2; x++) {
            float nx = (x + 0.5f - cx) / (w * 0.5f + 0.5f), ny = (y + 0.5f - (base - h + w * 0.5f)) / (w * 0.5f + 0.5f);
            if (y < base - h + w / 2 && nx * nx + ny * ny > 1.0f) continue;   /* rounded top */
            float v = 0.3f + 0.45f * clampf(0.5f + nx * lx, 0.0f, 1.0f) + (hash01(x, y, 61) - 0.5f) * 0.2f;
            if (y == base - h + w / 2 + 1 && x > cx - w / 2 && x < cx + w / 2) v -= 0.25f;   /* carved line */
            bset(x, y, dpick(Z_STONE, 5, v, x, y));
        }
}

static void mound(float cx, float cy, float rx, float churn) {
    for (int y = (int)(cy - 2); y <= (int)(cy + 2); y++)
        for (int x = (int)(cx - rx - 1); x <= (int)(cx + rx + 1); x++) {
            float nx = (x + 0.5f - cx) / rx, ny = (y + 0.5f - cy) / 1.6f;
            if (nx * nx + ny * ny > 1.0f) continue;
            float v = 0.35f + 0.4f * (-ny * 0.5f + nx * 0.3f) + (hash01(x, y, 71) - 0.5f) * 0.3f;
            if (churn > 0.0f && hash01(x, y + 3, 72) < churn * 0.4f) v -= 0.3f;
            bset(x, y, dpick(Z_DIRT, 4, v, x, y));
        }
}

static void dirt_spray(float cx, float cy, float u, int seed) {      /* clods thrown up as something digs out */
    if (u < 0.0f || u > 3.5f) return;
    for (int i = 0; i < 12; i++) {
        float ph = fmodf(u * (1.1f + hash01(i, 1, seed) * 0.6f) + hash01(i, 2, seed), 1.0f);
        float vx = (hash01(i, 3, seed) - 0.5f) * 9.0f, vy = 6.0f + hash01(i, 4, seed) * 6.0f;
        float x = cx + vx * ph, y = cy - vy * ph + 14.0f * ph * ph;
        fset((int)x, (int)y, (uint8_t)(Z_DIRT + 1 + (i & 1)));
    }
}

static void crow(float x, float y, float flap, int flying) {
    int up = flap > 0.0f;
    fset((int)x, (int)y, Z_BLACK); fset((int)x + 1, (int)y, Z_BLACK); fset((int)x - 1, (int)y, Z_BLACK);
    fset((int)x + 2, (int)y - 1, Z_BLACK);                           /* head and beak */
    if (!flying) { fset((int)x - 1, (int)y - 1, Z_BLACK); fset((int)x - 2, (int)y + 1, Z_BLACK); return; }
    for (int i = 1; i <= 3; i++) { fset((int)x - i + 1, (int)y + (up ? -i : i) / 2 - (up ? 1 : 0), Z_BLACK); fset((int)x + i - 1, (int)y + (up ? -i : i) / 2 - (up ? 1 : 0), Z_BLACK); }
}

static void art_zombies(double t) {
    float c = (float)cycle_begin(t, 30.0);
    ev(2.8, ART_EV_DIRT); ev(4.5, ART_EV_DIRT); ev(7.4, ART_EV_CAW); ev(8.5, ART_EV_DIRT);
    ev(10.2, ART_EV_GROAN); ev(17.0, ART_EV_GROAN); ev(24.0, ART_EV_GROAN); ev(28.0, ART_EV_GROAN);
    cv_begin();
    g_clip = 1e9f;
    for (int y = 0; y < 25; y++)                                       /* a sickly sky */
        for (int x = 0; x < PW; x++) bset(x, y, dpick(Z_SKY, 8, powf(y / 24.0f, 1.6f), x, y));
    for (int i = 0; i < 24; i++) {
        int sx = (int)(hash01(i, 1, 81) * 80), sy = (int)(hash01(i, 2, 81) * 14);
        if (hash01(i, 3, 81) + 0.15f * sinf((float)t * (1.0f + i * 0.3f)) > 0.55f) bset(sx, sy, Z_STAR);
    }
    const float mx = 58.0f, my = 9.0f, mr = 6.5f;
    for (int y = 0; y < 22; y++)
        for (int x = 46; x < 72; x++) {
            float dx = (x + 0.5f - mx) / mr, dy = (y + 0.5f - my) / (mr * 1.1f), d = sqrtf(dx * dx + dy * dy);
            if (d <= 1.0f) {
                float v = 0.92f - 0.3f * d * d - 0.15f * (dx + dy) * 0.5f;
                if (vnoise(x * 0.5f, y * 0.5f, 83) > 0.7f) v -= 0.15f;
                bset(x, y, dpick(Z_MOON, 3, v, x, y));
            } else if (d < 1.7f) bset(x, y, dpick(Z_SKY, 8, 0.85f - (d - 1.0f) * 0.9f, x, y));
        }
    for (int y = 0; y < 20; y++)                                       /* thin cloud passing */
        for (int x = 0; x < PW; x++) {
            float n = fbm2((x + (float)t * 1.2f) / 11.0f, y / 3.0f, 85);
            if (n < 0.6f) continue;
            if (cbg[y][x] >= Z_MOON && cbg[y][x] < Z_MOON + 3 && (n - 0.6f) * 40.0f < BAYER4[y & 3][x & 3] + 0.5f) continue;   /* a veil over the moon */
            bset(x, y, dpick(Z_CLOUD, 3, (n - 0.6f) * 3.0f + 0.4f / (1.0f + ((x - mx) * (x - mx) + (y - my) * (y - my)) / 60.0f), x, y));
        }
    for (int x = 0; x < PW; x++) {                                     /* far hills, a chapel */
        int h = 21 + (int)(1.5f * sinf(x * 0.11f + 2.0f) + 1.0f * sinf(x * 0.27f));
        for (int y = h; y < 26; y++) bset(x, y, Z_HILL);
    }
    for (int y = 13; y < 22; y++) for (int x = 33; x <= 37; x++) bset(x, y, Z_HILL);
    for (int y = 8; y < 13; y++) bset(35, y, Z_HILL);
    bset(34, 9, Z_HILL); bset(36, 9, Z_HILL); bset(35, 16, Z_LAMP);
    for (int y = 24; y < PH; y++)                                      /* the ground */
        for (int x = 0; x < PW; x++) {
            float v = 0.12f + 0.55f * (y - 24) / 26.0f + (vnoise(x * 0.4f, y * 0.9f, 87) - 0.5f) * 0.35f;
            v += 0.25f / (1.0f + ((x - 69.0f) * (x - 69.0f) + (y - 30.0f) * (y - 30.0f) * 3.0f) / 30.0f);   /* lantern pool */
            bset(x, y, dpick(Z_GRASS, 5, v, x, y));
        }
    for (int x = 0; x < 64; x++) {                                     /* the old fence, broken */
        if (x > 18 && x < 23) continue;
        bset(x, 26, Z_BLACK);
        if (x % 3 == 0 && !(x > 40 && x < 44)) { for (int y = 23; y < 29; y++) bset(x, y, Z_BLACK); bset(x, 22, Z_BLACK); }
    }
    for (int y = 10; y < 31; y++)                                      /* the mausoleum */
        for (int x = 64; x < PW; x++) {
            float v = 0.35f + (hash01(x / 3, y / 2, 89) - 0.5f) * 0.2f - (x == 64 ? 0.2f : 0.0f);
            if (y < 14) { if (x - 64 < (14 - y) * 2 - 3) continue; v += 0.15f; }   /* pediment */
            if ((x == 66 || x == 67 || x == 77 || x == 78) && y >= 15) v += 0.25f;   /* columns */
            if (x >= 70 && x <= 74 && y >= 18) v = -1.0f;                         /* the doorway */
            bset(x, y, v < 0 ? (uint8_t)Z_BLACK : dpick(Z_MAUS, 4, v, x, y));
        }
    bset(69, 21, Z_LAMP); bset(69, 22, Z_LAMP); bset(69, 20, Z_BLACK);
    /* the dead tree, with crows */
    fline(5, 49, 7, 30, Z_BLACK, 3); fline(7, 30, 9, 14, Z_BLACK, 2); fline(9, 14, 11, 4, Z_BLACK, 1);
    fline(8, 24, 17, 17, Z_BLACK, 2); fline(17, 17, 24, 15, Z_BLACK, 1); fline(9, 18, 4, 10, Z_BLACK, 1);
    fline(10, 12, 18, 8, Z_BLACK, 1); fline(18, 8, 22, 9, Z_BLACK, 1); fline(7, 30, 2, 26, Z_BLACK, 1);
    for (int i = 0; i < 3; i++) {
        float bx = 14.0f + i * 4.0f, by = 16.0f - i * 0.6f;
        if (c < 7.4f + i * 0.25f) crow(bx, by - 1, 0, 0);
        else {
            float u = c - 7.4f - i * 0.25f;
            float fx = bx - u * (6.0f + i * 2.0f), fy = by - u * (4.5f - i * 0.5f) + sinf(u * 9.0f + i) * 0.8f;
            if (fy > -3.0f) crow(fx, fy, sinf(u * 18.0f + i), 1);
        }
    }
    /* the graves */
    headstone(27, 37, 5, 6, 1); headstone(44, 33, 5, 7, 0); headstone(60, 35, 5, 6, 0); headstone(52, 30, 3, 4, 0);
    mound(27, 40.0f, 4.0f, c > 2.8f ? 1.0f : 0.0f);
    mound(44, 36.0f, 4.5f, c > 4.5f ? 1.0f : 0.0f);
    mound(60, 38.0f, 4.0f, c > 8.5f ? 1.0f : 0.0f);
    /* a hand that will not stay down */
    if (c > 2.8f) {
        float r = smoothstepf(2.8f, 3.8f, c), claw = sinf(c * 6.0f) * 0.5f + 0.5f;
        g_clip = 39.0f;
        f_cap(27.5f, 42.0f, 27.5f, 39.5f - r * 5.0f, 1.1f, Z_SKIN, 5, 0.55f, -0.83f, 0.55f, 1.0f);
        float hy = 39.5f - r * 5.0f;
        f_ell(27.5f, hy, 1.4f, 1.1f, Z_SKIN, 5, 0.55f, -0.83f, 0.6f, 1.0f);
        for (int f = -1; f <= 1; f++) f_line(27.5f + f * 0.9f, hy - 0.5f, 27.5f + f * 1.4f + claw * 0.6f, hy - 2.6f + claw * 0.8f, (uint8_t)(Z_SKIN + 4), 1);
        g_clip = 1e9f;
    }
    /* zombie B digs out at the right grave, then comes a little way */
    if (c > 8.5f) {
        float rise = smoothstepf(8.5f, 12.5f, c), walk = smoothstepf(13.5f, 30.0f, c);
        float yf = lerpf(38.0f + 16.0f, 38.0f, rise) + walk * 12.0f, x = 60.0f + walk * 4.0f;
        float s = (lerpf(38.0f, 50.0f, walk) - Z_HORIZON) / 18.5f;
        if (rise < 1.0f) g_clip = 37.5f;
        zombie(x + (rise < 1.0f ? sinf(c * 13.0f) * 0.4f : 0.0f), yf, s, c * 3.2f, rise < 1.0f ? 0.3f : 0.8f, 0.4f + 0.4f * sinf(c * 1.3f), sinf(c * 1.6f) * 0.5f, 7);
        g_clip = 1e9f;
        dirt_spray(60, 37, c - 8.5f, 3);
    }
    /* zombie A, from the middle grave, all the way to you */
    if (c > 4.5f) {
        float rise = smoothstepf(4.5f, 8.8f, c), walk = smoothstepf(10.5f, 27.5f, c);
        float base = lerpf(36.0f, 72.0f, walk);
        float yf = rise < 1.0f ? lerpf(36.0f + 15.0f, 36.0f, rise) : base;
        float x = lerpf(44.0f, 36.0f, walk) + (c > 27.5f ? sinf(c * 3.0f) * 0.6f : 0.0f);
        float s = (fmaxf(base, 36.0f) - Z_HORIZON) / 18.5f;
        float mouth = c > 23.5f ? 0.6f + 0.4f * sinf(c * 5.0f) : 0.3f + 0.3f * sinf(c * 1.1f);
        if (rise < 1.0f) g_clip = 35.5f;
        zombie(x + (rise < 1.0f ? sinf(c * 15.0f) * 0.4f : 0.0f), yf, s, c * 2.6f, rise < 1.0f ? 0.2f : 0.6f + 0.4f * walk, mouth, sinf(c * 1.3f) * 0.6f, 11);
        g_clip = 1e9f;
        dirt_spray(44, 35, c - 4.5f, 5);
        g_zeyes = 0.45f + 0.55f * smoothstepf(20.0f, 27.0f, c);
    } else g_zeyes = 0.45f;
    for (int r = 11; r < 14; r++)                                      /* ground mist */
        for (int x = 0; x < TCOLS; x++) {
            float n = vnoise((x + (float)t * 1.1f) / 6.0f, r * 0.8f, 91);
            if (n > 0.6f) ovset(x, r, n > 0.72f ? B_MED : B_LITE, (uint8_t)(n > 0.67f ? Z_FOG + 1 : Z_FOG));
        }
    cv_flush();
}

static void zombie_palette(uint8_t pal[256][3]) {
    static const uint8_t P[Z_STAR + 1 - 64][3] = {
        {1,2,2},{2,3,3},{2,5,4},{3,7,5},{4,9,6},{6,12,8},{8,15,9},{11,18,11},  /* sky */
        {40,44,30},{52,56,40},{62,63,52},                                       /* moon */
        {6,9,8},{10,14,12},{16,20,17},                                          /* cloud */
        {2,4,3},                                                                /* hills */
        {1,3,1},{2,6,2},{4,9,3},{6,13,5},{9,17,7},                              /* grass */
        {7,8,8},{12,13,13},{18,19,19},{26,27,26},{36,37,35},                    /* stone */
        {5,3,1},{9,6,3},{14,9,5},{20,14,8},                                     /* dirt */
        {6,9,6},{11,16,10},{18,24,15},{27,33,22},{38,44,30},                    /* skin */
        {3,3,5},{6,6,9},{10,10,14},{15,15,20},                                  /* rags */
        {50,60,14},{24,4,4},{0,1,1},                                            /* eyes, mouth, black */
        {9,13,11},{14,19,16},                                                   /* fog */
        {10,10,9},{16,16,14},{23,23,20},{31,31,27},                             /* mausoleum */
        {63,44,14},{40,38,30},{30,34,30},                                       /* lantern, teeth, stars */
    };
    for (int i = 0; i < Z_STAR + 1 - 64; i++)
        for (int k = 0; k < 3; k++) pal[64 + i][k] = P[i][k];
    pal[Z_EYE][0] = (uint8_t)(40 * g_zeyes + 8); pal[Z_EYE][1] = (uint8_t)(55 * g_zeyes + 8); pal[Z_EYE][2] = (uint8_t)(10 * g_zeyes);
}

/* ------------------------------------------------------------------ */
/* The crypt: out of their coffins, and they dance                     */
/* ------------------------------------------------------------------ */

enum {
    K_WALL = 64, K_FLOOR = 72, K_WOOD = 78, K_SATIN = 82, K_BONE = 84, K_EYE = 88, K_IRON = 89,
    K_FLAME = 91, K_WEB = 94, K_BLACK = 95, K_SKY = 96, K_DUST = 98, K_MOON = 99
};
static float g_kflame = 1.0f;

typedef struct { float x0, y0, x1, y1; int skull; } SkBone;

/* one skeleton's bones for a pose; kick > 0 lifts the left leg, < 0 the right */
static int skel_bones(SkBone *b, float x, float yf, float s, float kick, float arms, float bob, float hx, float hy) {
    int n = 0;
    float P = yf - 11.5f * s - bob, Nx = x, Ny = yf - 20.0f * s - bob;
    #define B(a, b_, c, d) (b[n++] = (SkBone){ a, b_, c, d, 0 })
    B(Nx, Ny, x, P);                                                  /* spine */
    for (int i = 0; i < 4; i++) {                                     /* ribs */
        float ry = Ny + (1.3f + i * 1.25f) * s, w = (2.7f - i * 0.25f) * s;
        B(x, ry, x - w, ry + 0.7f * s); B(x, ry, x + w, ry + 0.7f * s);
    }
    B(x - 1.8f * s, P, x + 1.8f * s, P);                               /* pelvis */
    B(Nx - 3.0f * s, Ny + 0.8f * s, Nx + 3.0f * s, Ny + 0.8f * s);     /* collarbones */
    for (int side = -1; side <= 1; side += 2) {                       /* legs */
        float k = side < 0 ? fmaxf(kick, 0.0f) : fmaxf(-kick, 0.0f);
        float hx0 = x + side * 1.4f * s;
        float kx = hx0 + side * (0.3f + 2.2f * k) * s, ky = P + (5.6f - 6.5f * k) * s;
        float fx = hx0 + side * (0.6f + 5.0f * k) * s, fy = k > 0 ? ky + (5.0f - 2.0f * k) * s : yf;
        B(hx0, P, kx, ky); B(kx, ky, fx, fy);
        B(fx, fy, fx + side * 1.2f * s, fy);
    }
    for (int side = -1; side <= 1; side += 2) {                       /* arms: down at 0, flung up at 1 */
        float a = (0.25f + arms * 2.4f) + (side > 0 ? 0.0f : 0.0f);
        float sx = Nx + side * 3.0f * s, sy = Ny + 0.8f * s;
        float ex = sx + side * sinf(a) * 4.0f * s, ey = sy + cosf(a) * 4.0f * s;
        float a2 = a + 0.5f;
        float wx = ex + side * sinf(a2) * 3.6f * s, wy = ey + cosf(a2) * 3.6f * s;
        B(sx, sy, ex, ey); B(ex, ey, wx, wy);
    }
    #undef B
    b[n++] = (SkBone){ Nx + hx * s, Ny - 2.5f * s + hy * s, 0, 0, 1 };  /* the skull */
    return n;
}

static void draw_skull_px(float cx, float cy, float s, int eyes_lit) {
    const float lx = -0.4f, ly = -0.9f;
    f_ell(cx, cy, 2.1f * s, 2.2f * s, K_BONE, 4, lx, ly, 0.35f, 1.0f);
    f_ell(cx, cy + 1.7f * s, 1.3f * s, 0.8f * s, K_BONE, 4, lx, ly, 0.3f, 0.85f);
    int ey = (int)floorf(cy + 0.1f * s + 0.5f);
    cset((int)floorf(cx - 0.8f * s + 0.5f), ey, eyes_lit ? (uint8_t)K_EYE : (uint8_t)K_BLACK);
    cset((int)floorf(cx + 0.8f * s + 0.5f), ey, eyes_lit ? (uint8_t)K_EYE : (uint8_t)K_BLACK);
    if (s > 1.1f) cset((int)floorf(cx + 0.5f), ey + 1, K_BLACK);
}

static void draw_bones(const SkBone *b, int n, float s, int eyes_lit) {
    int w = s >= 1.3f ? 2 : 1;
    for (int i = 0; i < n; i++) {
        if (b[i].skull) { draw_skull_px(b[i].x0, b[i].y0, s, eyes_lit); continue; }
        int rib = i >= 1 && i <= 8;                                   /* ribs stay thin so the cage shows */
        f_line(b[i].x0, b[i].y0, b[i].x1, b[i].y1, (uint8_t)(K_BONE + (rib ? 2 : 3)), rib ? 1 : w);
    }
    for (int i = 0; i < n; i++)                                       /* knuckles of shade at the joints */
        if (!b[i].skull) cset((int)floorf(b[i].x1 + 0.5f), (int)floorf(b[i].y1 + 0.5f), (uint8_t)(K_BONE + 2));
}

static void coffin(float cx, float top, float bottom, float open, int lit) {
    float sh = top + 6.0f;                                            /* shoulders */
    for (int y = (int)top; y <= (int)bottom; y++) {
        float hw = y < sh ? lerpf(4.0f, 6.0f, (y - top) / 6.0f) : lerpf(6.0f, 3.5f, (y - sh) / (bottom - sh));
        for (int x = (int)floorf(cx - hw - 1); x <= (int)ceilf(cx + hw + 1); x++) {
            float d = fabsf(x + 0.5f - cx);
            if (d > hw + 1.0f) continue;
            if (d > hw) { bset(x, y, (uint8_t)(K_WOOD + 1)); continue; }   /* the rim */
            bset(x, y, (uint8_t)(lit ? K_SATIN + ((x + y) & 1) : K_SATIN));
        }
        float lw = (hw + 1.0f) * (1.0f - open);                       /* the lid, swinging aside */
        for (int x = (int)floorf(cx - hw - 1 + (hw + 1) * 2 * open); x <= (int)ceilf(cx - hw - 1 + (hw + 1) * 2 * open + lw * 2); x++) {
            float v = 0.4f + 0.25f * hash01(x, y / 3, 101) - (open > 0.0f ? 0.1f : 0.0f);
            if ((y - (int)top) % 7 == 0) v -= 0.2f;
            bset(x, y, dpick(K_WOOD, 4, v, x, y));
        }
    }
}

static void art_skeletons(double t) {
    float c = (float)cycle_begin(t, 30.0);
    ev(2.0, ART_EV_DOOR); ev(3.0, ART_EV_DOOR);
    for (int k = 0; k < 9; k++) ev(6.5 + k * 1.76, ART_EV_XYLO);
    ev(24.4, ART_EV_BONES);
    g_kflame = candle_flicker(t);
    cv_begin();
    g_clip = 1e9f;
    /* the beam of moonlight from the window: a slanted band */
    #define IN_BEAM(x, y) ((y) > 6 && ((x) - ((y) - 6) * 0.9f) > 23.5f && ((x) - ((y) - 6) * 0.9f) < 31.5f)
    for (int y = 0; y < 36; y++)                                       /* crypt wall */
        for (int x = 0; x < PW; x++) {
            int row = y / 4, bx = (x + (row & 1) * 5) / 10;
            int mortar = y % 4 == 0 || (x + (row & 1) * 5) % 10 == 0;
            float v = 0.22f + 0.12f * hash01(bx, row, 103) + (hash01(x, y, 104) - 0.5f) * 0.08f;
            v += 0.35f * g_kflame / (1.0f + ((x - 54.0f) * (x - 54.0f) + (y - 22.0f) * (y - 22.0f)) / 90.0f);   /* candelabra */
            if (IN_BEAM(x, y)) v += 0.22f;
            if (mortar) v *= 0.5f;
            bset(x, y, dpick(K_WALL, 8, v, x, y));
        }
    for (int y = 36; y < PH; y++)                                      /* flagstones in perspective */
        for (int x = 0; x < PW; x++) {
            float d = (y - 34.0f), z = 40.0f / d, wx = (x - 40.0f) / d * 3.0f;
            int line = fmodf(z, 1.1f) < 0.12f || fabsf(fmodf(wx + 100.0f, 2.0f)) < 0.14f;
            float v = 0.18f + 0.2f * hash01((int)floorf(wx / 2.0f), (int)floorf(z / 1.1f), 105) + (y - 36) * 0.012f;
            v += 0.3f * g_kflame / (1.0f + ((x - 54.0f) * (x - 54.0f) * 0.4f + (y - 38.0f) * (y - 38.0f)) / 40.0f);
            if (IN_BEAM(x, y)) v += 0.28f;
            if (line) v *= 0.55f;
            bset(x, y, dpick(K_FLOOR, 6, v, x, y));
        }
    for (int y = 2; y <= 17; y++)                                      /* the window: night sky, a moon, bars */
        for (int x = 23; x <= 31; x++) {
            float ax = (x + 0.5f - 27.0f) / 4.5f, ay = (y + 0.5f - 6.0f) / 4.5f;
            if (y < 6 && ax * ax + ay * ay > 1.0f) continue;
            uint8_t col = (uint8_t)(K_SKY + (y > 11));
            if ((x - 29) * (x - 29) + (y - 5) * (y - 5) <= 2) col = K_MOON;
            if (x == 25 || x == 27 || x == 29 || y == 10) col = K_IRON;
            if (x == 23 || x == 31 || y == 17) col = K_IRON + 1;
            bset(x, y, col);
        }
    /* cobwebs in the top corners */
    for (int i = 0; i < 6; i++) { fline(0, (float)i * 2.0f, (float)i * 2.0f + 2.0f, 0, K_WEB, 1); fline(79, (float)i * 2.0f, 77.0f - i * 2.0f, 0, K_WEB, 1); }
    /* the coffins: lids swing open, and they are not empty */
    static const float CX[3] = { 12.0f, 40.0f, 68.0f };
    for (int i = 0; i < 3; i++) {
        float open = smoothstepf(i == 1 ? 3.0f : 2.0f, (i == 1 ? 3.0f : 2.0f) + 1.3f, c);
        coffin(CX[i], 9.0f, 35.0f, open, open > 0.5f);
    }
    /* the candelabra */
    for (int y = 22; y <= 36; y++) bset(54, y, K_IRON + 1);
    for (int x = 51; x <= 57; x++) bset(x, 23, K_IRON + 1);
    bset(51, 22, K_IRON); bset(57, 22, K_IRON); bset(54, 21, K_IRON); bset(53, 36, K_IRON); bset(55, 36, K_IRON);
    for (int k = 0; k < 3; k++) {
        int fx = 51 + k * 3, fy = k == 1 ? 18 : 19;
        bset(fx, fy + 2, K_BONE + 2); bset(fx, fy + 1, K_BONE + 2);
        fset(fx, fy, (uint8_t)(K_FLAME + (g_kflame > 0.9f ? 2 : 1))); fset(fx, fy - 1, K_FLAME);
    }
    /* the dancers */
    float dance = smoothstepf(4.3f, 6.5f, c), collapse = smoothstepf(24.4f, 25.4f, c);
    float beat = (c - 6.5f) / 0.44f, ph = beat * PI_F;
    int dancing = c > 6.5f && c < 24.4f;
    for (int i = 0; i < 3; i++) {
        float open = smoothstepf(i == 1 ? 3.0f : 2.0f, (i == 1 ? 3.0f : 2.0f) + 1.3f, c);
        if (open < 0.4f) continue;
        float hop = dance < 1.0f && dance > 0.0f ? fabsf(sinf(dance * PI_F * 2.0f)) * 3.0f : 0.0f;
        float x = lerpf(CX[i], 18.0f + i * 22.0f, dance), yf = lerpf(34.0f, 45.0f, dance) - hop, s = lerpf(0.95f, 1.3f, dance);
        float kick = 0.0f, arms = 0.15f, bob = 0.0f, hx = 0.0f, hy = 0.0f;
        if (dancing) {
            float sgn = (i == 1) ? -1.0f : 1.0f;
            kick = sgn * sinf(ph) * 0.9f;
            arms = 0.55f + 0.35f * sinf(ph + PI_F * 0.5f);
            bob = fabsf(sinf(ph)) * 0.8f * s;
            x += sinf(ph * 0.5f) * 1.2f;
            hx = sinf(ph) * 0.3f;
            if (c > 21.0f) { arms = 1.0f; kick = sgn * 0.9f * fabsf(sinf(ph)); }   /* the big finish */
        }
        SkBone b[32];
        int n = skel_bones(b, x, yf, s, kick, arms, bob, hx, hy);
        if (i == 1 && c > 13.6f && c < 16.0f) {                         /* the middle one tosses its head up and catches it */
            float u = (c - 13.6f) / 2.4f, arc = sinf(u * PI_F);
            b[n - 1].x0 += sinf(u * PI_F * 2.0f) * 4.0f * s; b[n - 1].y0 -= arc * 13.0f * s;
        }
        if (collapse > 0.0f) {                                          /* the music stops */
            for (int k = 0; k < n; k++) {
                float cxm = (b[k].x0 + (b[k].skull ? b[k].x0 : b[k].x1)) * 0.5f, cym = (b[k].y0 + (b[k].skull ? b[k].y0 : b[k].y1)) * 0.5f;
                float tx = x + (hash01(k, i, 107) - 0.5f) * 12.0f, ty = yf - 0.5f - hash01(k, i, 108) * 1.5f - (b[k].skull ? 1.0f : 0.0f);
                float kk = collapse * collapse;
                float rot = kk * (hash01(k, i, 109) - 0.5f) * 3.0f + kk * 1.4f, cr = cosf(rot), sr = sinf(rot);
                float ncx = lerpf(cxm, tx, kk), ncy = lerpf(cym, ty, kk);
                if (b[k].skull) { b[k].x0 = ncx; b[k].y0 = ncy; continue; }
                float ax = b[k].x0 - cxm, ay = b[k].y0 - cym, bx2 = b[k].x1 - cxm, by2 = b[k].y1 - cym;
                b[k].x0 = ncx + ax * cr - ay * sr; b[k].y0 = ncy + ax * sr + ay * cr;
                b[k].x1 = ncx + bx2 * cr - by2 * sr; b[k].y1 = ncy + bx2 * sr + by2 * cr;
                if (b[k].y0 > yf) b[k].y0 = yf; if (b[k].y1 > yf) b[k].y1 = yf;
            }
        }
        draw_bones(b, n, s, collapse < 0.9f || c < 27.0f);
    }
    /* a skull and a candle stub in front */
    draw_skull_px(70.0f, 46.0f, 1.2f, 0);
    for (int i = 0; i < 14; i++) {                                     /* dust turning in the moonbeam */
        float bx = 28.0f + hash01(i, 1, 111) * 22.0f + sinf((float)t * 0.4f + i) * 2.0f;
        float by = fmodf(hash01(i, 2, 111) * 40.0f + (float)t * (0.4f + hash01(i, 3, 111) * 0.4f), 40.0f) + 6.0f;
        if (IN_BEAM(bx, by)) ovset((int)bx, (int)(by / 2.0f), 0xFA, K_DUST);
    }
    #undef IN_BEAM
    cv_flush();
}

static void skeleton_palette(uint8_t pal[256][3]) {
    static const uint8_t P[K_MOON + 1 - 64][3] = {
        {2,2,4},{4,4,7},{6,7,10},{9,10,14},{13,14,19},{18,19,25},{24,25,31},{32,33,39},   /* wall */
        {2,2,3},{5,5,7},{8,8,11},{12,12,16},{17,17,22},{24,24,30},                           /* floor */
        {6,3,2},{11,6,3},{16,9,5},{23,13,7},                                                 /* coffin wood */
        {6,1,2},{18,3,6},                                                                    /* satin */
        {20,19,16},{34,33,28},{48,47,40},{60,59,52},                                         /* bone */
        {16,58,30},                                                                          /* eyes */
        {10,10,12},{20,20,23},                                                               /* iron */
        {63,24,2},{63,46,10},{63,62,40},                                                     /* flame */
        {16,17,20},{0,0,1},                                                                  /* web, black */
        {3,5,14},{6,9,20},                                                                   /* window sky */
        {40,44,50},{56,58,60},                                                               /* dust, moon */
    };
    for (int i = 0; i < K_MOON + 1 - 64; i++)
        for (int k = 0; k < 3; k++) pal[64 + i][k] = P[i][k];
    for (int k = 0; k < 3; k++) pal[K_FLAME][k] = (uint8_t)clampi((int)(pal[K_FLAME][k] * g_kflame), 0, 63);
}

/* ------------------------------------------------------------------ */
/* The hallway: a ghost comes to say boo, then thinks better of it     */
/* ------------------------------------------------------------------ */

enum {
    H_PAPER = 64, H_WOOD = 72, H_RUG = 80, H_CEIL = 86, H_GOLD = 90, H_FLAME = 93, H_GHOST = 96,
    H_PORT = 104, H_EYE = 108, H_BLACK = 109, H_DOOR = 110, H_DUST = 111
};
static float g_hflash, g_hflick = 1.0f;

#define HV_X 40.0f
#define HV_Y 21.0f
#define H_F 40.0f
#define H_END 9.0f

typedef struct { float x, h, z; } Sconce;
static const Sconce SCONCE[4] = { { -1.0f, 0.35f, 2.2f }, { 1.0f, 0.35f, 3.6f }, { -1.0f, 0.35f, 5.4f }, { 1.0f, 0.35f, 7.2f } };

static float hall_light(float x, float h, float z, const float *cl, float gx, float gz, float gglow) {
    float L = 0.05f;
    for (int i = 0; i < 4; i++) {
        float dx = x - SCONCE[i].x * 0.92f, dh = h - SCONCE[i].h, dz = z - SCONCE[i].z;
        L += cl[i] / (1.0f + (dx * dx + dh * dh + dz * dz) * 2.2f);
    }
    float dgx = x - gx, dgz = z - gz;
    L += gglow / (1.0f + (dgx * dgx + dgz * dgz) * 1.5f);
    return L;
}

/* the ghost at screen (sx, sy), pixels per world unit k; boo opens it right up */
static void ghost_px(float sx, float sy, float k, float t, float alpha, float boo, float look) {
    float w = 0.48f * k, h = 0.72f * k;
    for (int y = (int)floorf(sy - h * 1.15f); y <= (int)ceilf(sy + h * 1.2f); y++)
        for (int x = (int)floorf(sx - w * 1.9f); x <= (int)ceilf(sx + w * 1.9f); x++) {
            float u = (x + 0.5f - sx) / w, v = (y + 0.5f - sy) / h;   /* v: -1 top, +1 hem */
            int inside;
            if (v < -0.25f) inside = u * u + ((v + 0.25f) / 0.75f) * ((v + 0.25f) / 0.75f) <= 1.0f;
            else inside = fabsf(u) <= 1.0f + 0.22f * (v + 0.25f) && v <= 0.85f + 0.12f * sinf(u * 6.5f + t * 5.0f);
            float au = (fabsf(u) - (1.05f + boo * 0.25f)) / 0.32f, av = (v + 0.05f + boo * 0.45f + 0.08f * sinf(t * 3.0f + (u > 0 ? 1.0f : 0.0f))) / (0.3f + boo * 0.1f);
            if (!inside && au * au + av * av <= 1.0f) inside = 1;     /* arms, raised for the boo */
            if (!inside) continue;
            float a = alpha * (1.0f - smoothstepf(0.45f, 1.0f, v));
            if (a * 16.0f <= BAYER4[y & 3][x & 3] + 0.5f) continue;
            float val = 0.55f + 0.4f * (1.0f - u * u * 0.6f) - 0.25f * fmaxf(v, 0.0f) + 0.15f * sinf(t * 2.0f + v * 3.0f);
            uint8_t col = dpick(H_GHOST, 8, val, x, y);
            for (int e = -1; e <= 1; e += 2) {                         /* hollow eyes */
                float ex = (u - e * 0.32f - look * 0.12f) / 0.15f, ey = (v + 0.5f) / 0.2f;
                if (ex * ex + ey * ey <= 1.0f) col = H_BLACK;
            }
            float mx = u / (0.11f + boo * 0.1f), my = (v + 0.08f - boo * 0.04f) / (0.08f + boo * 0.16f);   /* the mouth: o, then OOO */
            if (mx * mx + my * my <= 1.0f) col = H_BLACK;
            fset(x, y, col);
        }
}

static void art_hallway(double t) {
    float c = (float)cycle_begin(t, 30.0);
    ev(1.2, ART_EV_WAIL); ev(2.8, ART_EV_BOO); ev(24.6, ART_EV_DOOR);
    g_hflick = candle_flicker(t);
    /* where the ghost is: (gx, gz) on the floor plan, gh its height, ga how solid */
    float gx = 0.0f, gz = 8.0f, gh = 0.1f, ga = 0.0f, boo = 0.0f, look = 0.0f;
    if (c < 1.2f) { gz = 8.0f; ga = smoothstepf(0.2f, 1.2f, c); }
    else if (c < 2.8f) { float u = (c - 1.2f) / 1.6f; gz = lerpf(8.0f, 1.25f, u * u); ga = 1.0f; gx = sinf(c * 4.0f) * 0.15f; gh = lerpf(0.1f, -0.15f, u); boo = smoothstepf(2.3f, 2.8f, c); }
    else if (c < 4.6f) { gz = 1.25f + 0.04f * sinf(c * 9.0f); gh = -0.15f; ga = 1.0f; boo = 1.0f - smoothstepf(3.9f, 4.6f, c); }
    else if (c < 6.0f) { float u = smoothstepf(4.6f, 6.0f, c); gz = lerpf(1.25f, 1.9f, u); ga = 1.0f; look = 0.0f; gh = lerpf(-0.15f, 0.02f, u); }
    else if (c < 22.0f) {
        float u = smoothstepf(6.0f, 22.0f, c);
        gz = lerpf(1.9f, 8.3f, u); ga = 1.0f; gx = sinf(c * 0.9f) * 0.35f; gh = 0.02f + 0.06f * sinf(c * 1.7f);
        look = (fmodf(c, 5.5f) < 1.2f) ? -1.0f : 0.0f;                  /* glances back now and then */
    } else if (c < 24.0f) { gz = 8.3f + (c - 22.0f) * 0.3f; ga = 1.0f - smoothstepf(22.0f, 24.0f, c); }
    g_hflash = c > 2.8f && c < 3.3f ? 1.0f - (c - 2.8f) / 0.5f : 0.0f;
    float cl[4];
    for (int i = 0; i < 4; i++) {                                     /* candles gutter as it passes */
        float near = fabsf(SCONCE[i].z - gz);
        cl[i] = (0.55f + 0.1f * sinf((float)t * (7.0f + i) + i)) * g_hflick * (0.25f + 0.75f * smoothstepf(0.0f, 1.6f, near) + (ga < 0.1f ? 0.75f : 0.0f));
        cl[i] = fminf(cl[i], 0.7f);
    }
    float gglow = 0.35f * ga;
    float door = c > 24.6f ? 1.0f - smoothstepf(24.6f, 26.0f, c) : (c > 21.5f ? smoothstepf(21.5f, 22.5f, c) : 0.0f);
    cv_begin();
    for (int y = 0; y < PH; y++)
        for (int x = 0; x < PW; x++) {
            float dx = x + 0.5f - HV_X, dy = y + 0.5f - HV_Y;
            float zw = fabsf(dx) > 0.01f ? H_F * 1.0f / fabsf(dx) : 1e9f;   /* side walls at x = -1, +1 */
            float zf = dy > 0.01f ? H_F * 1.0f / dy : 1e9f;                /* floor 1.0 below the eye */
            float zc = dy < -0.01f ? H_F * 1.3f / -dy : 1e9f;              /* ceiling 1.3 above */
            float z = fminf(zw, fminf(zf, zc));
            uint8_t col;
            if (z > H_END) {                                            /* the end wall, with its door */
                float wx = dx * H_END / H_F, wh = -dy * H_END / H_F;
                float L = hall_light(wx, wh, H_END, cl, gx, gz, gglow);
                if (fabsf(wx) < 0.32f && wh < 0.75f) {
                    float open = 0.25f * door;
                    col = wx > 0.32f - 0.64f * open ? (uint8_t)H_DOOR : dpick(H_WOOD, 8, L * 0.8f, x, y);
                } else col = dpick(H_PAPER, 8, L * 0.8f, x, y);
            } else if (z == zw) {                                       /* wallpaper over wainscot */
                float wh = -dy * z / H_F, side = dx < 0 ? -1.0f : 1.0f;
                float L = hall_light(side, wh, z, cl, gx, gz, gglow);
                if (wh < -0.45f) {
                    float pz = fmodf(z, 1.0f);
                    float v = L * (pz < 0.06f || wh > -0.5f ? 0.55f : 0.9f);
                    col = dpick(H_WOOD, 8, v, x, y);
                } else {
                    float a = fmodf(z * 3.0f + wh * 3.0f + 10.0f, 1.0f), b = fmodf(z * 3.0f - wh * 3.0f + 10.0f, 1.0f);
                    float pat = (fabsf(a - 0.5f) < 0.12f || fabsf(b - 0.5f) < 0.12f) ? 1.2f : 1.0f;
                    col = dpick(H_PAPER, 8, L * pat, x, y);
                }
                /* portraits on the walls; their eyes follow the ghost */
                static const float PZ[3] = { 2.9f, 4.5f, 6.3f }, PS[3] = { -1.0f, 1.0f, -1.0f };
                for (int p = 0; p < 3; p++) {
                    if (PS[p] != side || fabsf(z - PZ[p]) > 0.38f || wh < -0.05f || wh > 0.75f) continue;
                    float fz = fabsf(z - PZ[p]), fh = fabsf(wh - 0.35f);
                    if (fz > 0.31f || fh > 0.36f) col = dpick(H_GOLD, 3, L * 1.5f, x, y);
                    else {
                        float fx = (z - PZ[p]) / 0.3f, fy = (wh - 0.42f) / 0.22f;
                        col = (fx * fx + fy * fy < 1.0f) ? dpick(H_PORT, 4, 0.4f + L, x, y) : dpick(H_PORT, 4, L * 0.4f, x, y);
                    }
                }
                /* doors in the side walls */
                if (fabsf(z - 4.0f) < 0.35f && side < 0 && wh < 0.65f) col = dpick(H_WOOD, 8, L * 0.6f, x, y);
                /* the sconces themselves */
                for (int i = 0; i < 4; i++)
                    if (SCONCE[i].x == side && fabsf(z - SCONCE[i].z) < 0.08f && fabsf(wh - SCONCE[i].h + 0.1f) < 0.08f) col = H_GOLD + 2;
            } else if (z == zf) {                                       /* floorboards, with a long runner */
                float wx = dx * z / H_F;
                float L = hall_light(wx, -1.0f, z, cl, gx, gz, gglow);
                if (fabsf(wx) < 0.48f) {
                    int border = fabsf(wx) > 0.4f;
                    col = dpick(H_RUG, 6, L * (border ? 1.25f : 0.95f) * (0.9f + 0.2f * hash01((int)(wx * 8), (int)(z * 6), 113)), x, y);
                } else {
                    float bz = fmodf(wx * 6.0f + 20.0f, 1.0f);
                    col = dpick(H_WOOD, 8, L * (bz < 0.12f ? 0.55f : 0.9f), x, y);
                }
            } else {                                                    /* ceiling and its beams */
                float L = hall_light(dx * z / H_F, 1.3f, z, cl, gx, gz, gglow) * 0.6f;
                col = dpick(H_CEIL, 4, fmodf(z, 1.5f) < 0.18f ? L * 1.5f : L, x, y);
            }
            bset(x, y, col);
        }
    for (int i = 0; i < 4; i++) {                                      /* the flames */
        float z = SCONCE[i].z, sx = HV_X + SCONCE[i].x * 0.92f * H_F / z, sy = HV_Y - (SCONCE[i].h + 0.08f) * H_F / z;
        if (cl[i] > 0.12f) fset((int)sx, (int)sy, (uint8_t)(H_FLAME + (cl[i] > 0.45f ? 2 : 1)));
        if (cl[i] > 0.3f && z < 4.0f) fset((int)sx, (int)sy - 1, H_FLAME);
    }
    /* portrait eyes (drawn after, as tiny pale points looking towards the ghost) */
    static const float PZ[3] = { 2.9f, 4.5f, 6.3f }, PS[3] = { -1.0f, 1.0f, -1.0f };
    for (int p = 0; p < 3; p++) {
        float sx = HV_X + PS[p] * H_F / PZ[p], sy = HV_Y - 0.47f * H_F / PZ[p];
        float gsx = HV_X + gx * H_F / gz;
        int lookx = ga > 0.1f ? (gsx > sx ? 1 : -1) : (PS[p] < 0 ? 1 : -1);
        float span = 0.09f * H_F / PZ[p];
        if (span < 0.6f) continue;
        fset((int)(sx - span + lookx * 0.4f), (int)sy, H_EYE);
        fset((int)(sx + span + lookx * 0.4f), (int)sy, H_EYE);
    }
    if (ga > 0.02f) {
        float k = H_F / gz;
        ghost_px(HV_X + gx * k, HV_Y - gh * k, k, (float)t, ga, boo, look);
    }
    for (int i = 0; i < 10; i++) {                                     /* dust, very faint */
        float dx = hash01(i, 1, 115) * 80.0f + sinf((float)t * 0.3f + i) * 3.0f;
        float dy = fmodf(hash01(i, 2, 115) * 50.0f + (float)t * (0.5f + hash01(i, 3, 115)), 50.0f);
        ovset((int)dx, (int)(dy / 2.0f), 0xFA, H_DUST);
    }
    cv_flush();
}

static void hallway_palette(uint8_t pal[256][3]) {
    static const uint8_t P[H_DUST + 1 - 64][3] = {
        {1,2,2},{2,4,4},{3,7,6},{5,10,9},{7,13,12},{10,17,15},{14,22,19},{19,28,24},   /* wallpaper */
        {2,1,1},{4,2,1},{7,4,2},{11,6,3},{15,9,5},{20,12,7},{26,17,10},{33,22,13},      /* wood */
        {4,0,1},{9,1,2},{15,2,4},{22,4,6},{30,7,8},{40,11,10},                          /* runner */
        {1,1,1},{3,2,2},{5,4,3},{8,6,5},                                                /* ceiling */
        {20,14,4},{34,25,8},{52,40,14},                                                 /* gilt */
        {63,26,4},{63,46,12},{63,62,40},                                                /* flame */
        {10,18,24},{16,26,32},{22,34,40},{30,42,48},{38,50,55},{46,56,60},{54,61,63},{60,63,63},   /* ghost */
        {6,4,3},{12,8,6},{26,19,14},{40,31,24},                                         /* portraits */
        {50,46,34},{0,1,3},{2,3,6},{24,30,30},                                          /* eyes, black, doorway, dust */
    };
    float f = g_hflash;
    for (int i = 0; i < H_DUST + 1 - 64; i++)
        for (int k = 0; k < 3; k++) pal[64 + i][k] = (uint8_t)clampi((int)(P[i][k] + (60 - P[i][k]) * f * 0.55f), 0, 63);
    for (int k = 0; k < 3; k++) {
        pal[H_FLAME][k] = (uint8_t)clampi((int)(pal[H_FLAME][k] * g_hflick), 0, 63);
        pal[H_FLAME + 1][k] = (uint8_t)clampi((int)(pal[H_FLAME + 1][k] * g_hflick), 0, 63);
    }
}

/* ------------------------------------------------------------------ */
/* The pumpkin patch: one of them has been carved for you              */
/* ------------------------------------------------------------------ */

enum {
    P_SKY = 64, P_MOON = 70, P_CLOUD = 74, P_STAR = 77, P_FAR = 78, P_BARNWIN = 79, P_FIELD = 80,
    P_VINE = 86, P_RIND = 90, P_GLOW = 98, P_GLOW_L = 102, P_GLOW_R = 104, P_STEM = 106,
    P_BLACK = 109, P_STRAW = 110, P_RAG = 112, P_SCEYE = 114, P_SMOKE = 115, P_LEAF = 117
};
static float g_plit[3], g_sceye;

typedef struct { float cx, cy, rx, ry; int glow, nglow, face; } Lantern;
static const Lantern LANTERN[3] = {
    { 40.0f, 31.0f, 19.0f, 12.5f, P_GLOW, 4, 0 },          /* the one carved for you */
    { 12.0f, 37.0f, 6.5f, 4.6f, P_GLOW_L, 2, 1 },
    { 68.0f, 38.0f, 7.0f, 4.8f, P_GLOW_R, 2, 2 },
};
#define P_HORIZON 28

static float bump(float c, float a, float b) { return c <= a || c >= b ? 0.0f : sinf(3.14159265f * (c - a) / (b - a)); }

static int in_tri(float px, float py, float ax, float ay, float bx, float by, float cx, float cy) {
    float d1 = (px - bx) * (ay - by) - (ax - bx) * (py - by);
    float d2 = (px - cx) * (by - cy) - (bx - cx) * (py - cy);
    float d3 = (px - ax) * (cy - ay) - (cx - ax) * (py - ay);
    return !((d1 < 0 || d2 < 0 || d3 < 0) && (d1 > 0 || d2 > 0 || d3 > 0));
}

/* is this point of a face carved out?  (nx, ny) on the unit ellipse; evil turns the smile wicked */
static int carved(int face, float nx, float ny, float evil, float mouth) {
    float ax = fabsf(nx);
    if (face == 1) {                                                  /* round eyes, a little O */
        float ex = (ax - 0.36f) / 0.21f, ey = (ny + 0.22f) / 0.27f;
        if (ex * ex + ey * ey < 1.0f) return 1;
        float mx = nx / 0.2f, my = (ny - 0.38f) / 0.22f;
        return mx * mx + my * my < 1.0f;
    }
    if (face == 2) {                                                  /* slit eyes, a jagged grin */
        if (in_tri(ax, ny, 0.12f, -0.22f, 0.66f, -0.44f, 0.46f, -0.02f)) return 1;
        if (ny > 0.16f && ny < 0.54f - 0.5f * nx * nx && ax < 0.66f)
            return ny > 0.24f + 0.12f * fabsf(fmodf(nx * 5.0f + 10.5f, 1.0f) - 0.5f) * 2.0f;
        return 0;
    }
    /* the hero: triangle eyes that tip over into a scowl */
    float ix = lerpf(0.17f, 0.15f, evil), iy = lerpf(-0.10f, -0.32f, evil);
    float ox = lerpf(0.58f, 0.64f, evil), oy = lerpf(-0.10f, -0.54f, evil);
    float px = lerpf(0.38f, 0.44f, evil), py = lerpf(-0.52f, -0.04f, evil);
    if (in_tri(ax, ny, ix, iy, ox, oy, px, py)) return 1;
    if (in_tri(nx, ny, -0.08f, 0.13f, 0.08f, 0.13f, 0.0f, -0.04f)) return 1;
    float w = lerpf(0.64f, 0.76f, evil), q = (nx / w) * (nx / w);
    if (q >= 1.0f) return 0;
    float yc = lerpf(0.08f, -0.06f, evil), y0 = 0.22f, y1 = 0.66f + 0.14f * mouth;
    float top = y0 + (yc - y0) * q, bot = y1 + (yc - y1) * q;
    if (ny <= top || ny >= bot) return 0;
    float dt = ny - top, db = bot - ny;
    if (evil < 0.5f) {                                                /* two square teeth up top, one below */
        if (fabsf(ax - 0.24f) < 0.065f && dt < 0.13f) return 0;
        if (ax < 0.07f && db < 0.13f) return 0;
    } else {                                                          /* fangs */
        for (int k = 0; k < 2; k++) {
            float tc = k ? 0.46f : 0.13f;
            if (dt < 0.17f && fabsf(ax - tc) < 0.06f * (1.0f - dt / 0.17f) + 0.015f) return 0;
        }
        if (db < 0.17f && fabsf(ax - 0.29f) < 0.065f * (1.0f - db / 0.17f) + 0.015f) return 0;
    }
    return 1;
}

static float lid_cut(float nx) { return -0.60f + 0.07f * (fabsf(fmodf(nx * 3.5f + 20.5f, 1.0f) - 0.5f) * 4.0f - 1.0f); }

/* the rind at unit-ellipse point (nx, ny), lit by the moon behind and the other lanterns */
static uint8_t rind(const Lantern *L, int me, float nx, float ny, float lit, int x, int y) {
    float d2 = nx * nx + ny * ny, z = sqrtf(fmaxf(0.0f, 1.0f - d2));
    float lam = asinf(clampf(nx / sqrtf(fmaxf(1e-3f, 1.0f - ny * ny)), -1.0f, 1.0f));
    float crease = 0.5f + 0.5f * smoothstepf(-1.0f, 0.3f, cosf(lam * (me ? 6.0f : 9.0f)));
    float moon = clampf(0.7f * nx - 0.7f * ny, 0.0f, 1.0f);
    float v = 0.17f + 0.12f * z + 0.6f * powf(moon, 1.5f) * (1.15f - z) + 0.22f * lit * z * z;
    for (int j = 0; j < 3; j++) {
        if (j == me || g_plit[j] < 0.02f) continue;
        float side = LANTERN[j].cx > L->cx ? 1.0f : -1.0f;
        float dx = LANTERN[j].cx - (L->cx + nx * L->rx), dy = (LANTERN[j].cy - (L->cy + ny * L->ry)) * 1.125f;
        v += g_plit[j] * 0.5f * clampf(0.2f + 0.8f * nx * side, 0.0f, 1.0f) / (1.0f + (dx * dx + dy * dy) / 150.0f);
    }
    return dpick(P_RIND, 8, v * crease, x, y);
}

static void lantern(int me, float evil, float mouth, float lid, float look) {
    const Lantern *L = &LANTERN[me];
    float lit = g_plit[me];
    for (int y = (int)floorf(L->cy - L->ry - lid - 2.0f); y <= (int)ceilf(L->cy + L->ry); y++)
        for (int x = (int)floorf(L->cx - L->rx - 1.0f); x <= (int)ceilf(L->cx + L->rx + 1.0f); x++) {
            float fx = (x + 0.5f - L->cx) / L->rx;
            float lift = lid * (1.0f + 0.35f * fx);                     /* the lid tips as it rises */
            for (int pass = 0; pass < 2; pass++) {
                float py = y + 0.5f + (pass == 0 ? lift : 0.0f);
                float nx = fx, ny = (py - L->cy) / L->ry;
                float sy = ny < 0.0f ? ny * 1.06f : ny;                 /* a little flatter on top */
                if (fabsf(nx) < 0.2f && ny < 0.0f) sy -= 0.06f * (1.0f - fabsf(nx) / 0.2f);   /* dimple at the stem */
                if (nx * nx + sy * sy > 1.0f) continue;
                int above = me == 0 && ny < lid_cut(nx);
                if (pass == 0) {                                        /* the lid, lifted */
                    if (!above || lid < 0.05f) continue;
                    fset(x, y, rind(L, me, nx, ny, lit, x, y));
                    break;
                }
                if (above && lid >= 0.05f) { fset(x, y, dpick(L->glow, L->nglow, 0.8f, x, y)); break; }   /* the open top */
                if (me == 0 && lid < 0.05f && fabsf(ny - lid_cut(nx)) < 0.05f) { fset(x, y, P_RIND); break; }   /* the cut */
                if (carved(L->face, nx, ny, evil, mouth)) {
                    float ex = nx - look * 0.25f, ey = ny - 0.3f;
                    float v = 0.85f - 0.5f * sqrtf(ex * ex + ey * ey);
                    if (me == 0) {                                      /* the cut flesh shows on the outer side of each hole */
                        float r = sqrtf(nx * nx + ny * ny) + 1e-3f, k = 0.6f + 1.4f * r;
                        if (!carved(0, nx + nx / r * k / L->rx, ny + ny / r * k / L->ry, evil, mouth)) v = 1.0f;
                    }
                    fset(x, y, dpick(L->glow, L->nglow, v, x, y));
                } else fset(x, y, rind(L, me, nx, ny, lit, x, y));
                break;
            }
        }
    /* the stem, on the lid */
    float sx = L->cx, sy = L->cy - L->ry * (me ? 1.0f : 1.04f) - (me == 0 ? lid * 1.0f : 0.0f);
    float h = me ? 1.6f : 4.5f;
    f_cap(sx - 0.3f, sy + 0.8f, sx + 0.6f, sy - h, me ? 0.6f : 1.1f, P_STEM, 3, 0.7f, -0.6f, 0.1f, 0.9f);
    if (!me) f_line(sx + 0.6f, sy - h, sx + 2.6f, sy - h - 0.6f, P_STEM + 1, 1);
}

/* a scarecrow; s = 1 stands about 19 pixels tall.  under: orange light from below */
static void scarecrow(float x, float yf, float s, float under, int pole) {
    float top = yf - 19.0f * s, ay = yf - 13.0f * s, hip = yf - 7.0f * s;
    float lo = 0.0f, hi = under;
    if (pole) f_line(x, yf, x, ay - 1.0f, P_RAG, 1);
    else for (int side = -1; side <= 1; side += 2)                    /* legs */
        f_cap(x + side * 1.0f * s, hip, x + side * 1.3f * s, yf - 0.5f, 0.75f * s, P_RAG, 2, 0.0f, 1.0f, lo, hi * 0.35f);
    f_cap(x - 6.6f * s, ay + 0.3f * s, x + 6.6f * s, ay - 0.3f * s, 0.85f * s, P_RAG, 2, 0.0f, 1.0f, lo, hi * 0.6f);   /* arms on the crossbar */
    for (int yy = (int)floorf(ay - 1.0f * s); yy <= (int)ceilf(hip + 1.2f * s); yy++)   /* the coat */
        for (int xx = (int)floorf(x - 3.0f * s); xx <= (int)ceilf(x + 3.0f * s); xx++) {
            float u = (yy - ay) / (hip - ay), half = (2.4f - 0.5f * u) * s;
            if (fabsf(xx + 0.5f - x) > half) continue;
            if (u > 0.85f && hash01(xx, 7, 151) < 0.5f) continue;     /* ragged hem */
            fset(xx, yy, dpick(P_RAG, 2, lo + (hi - lo) * smoothstepf(0.55f, 1.0f, u), xx, yy));
        }
    for (int i = 0; i < 6; i++) {                                     /* straw sticking out */
        float a = (hash01(i, 1, 153) - 0.5f) * 1.6f;
        for (int side = -1; side <= 1; side += 2)
            f_line(x + side * 6.6f * s, ay, x + side * (7.4f + hash01(i, 2, 153)) * s, ay + a * s, P_STRAW + (i & 1), 1);
        if (i < 3) f_line(x + (i - 1) * 1.2f * s, hip + 1.0f * s, x + (i - 1) * 1.6f * s, hip + 2.4f * s, P_STRAW, 1);
    }
    float hx = x + 0.2f * s, hy = top + 4.6f * s;                      /* sack head, stitched face */
    f_ell(hx, hy, 2.4f * s, 2.6f * s, P_STRAW, 2, 0.0f, 1.0f, 0.0f, 0.4f + 0.6f * under);
    for (int side = -1; side <= 1; side += 2) {
        int ex = (int)floorf(hx + side * 1.0f * s + 0.5f), ey = (int)floorf(hy - 0.4f * s + 0.5f);
        fset(ex, ey, g_sceye > 0.05f ? (uint8_t)P_SCEYE : (uint8_t)P_BLACK);
    }
    if (s > 1.4f) f_line(hx - 1.2f * s, hy + 1.2f * s, hx + 1.2f * s, hy + 1.0f * s, P_BLACK, 1);
    f_line(x - 3.8f * s, top + 2.2f * s, x + 3.9f * s, top + 2.0f * s, P_RAG, s > 1.4f ? 2 : 1);   /* the hat */
    f_cap(x - 1.6f * s, top + 1.8f * s, x + 1.2f * s, top, 1.1f * s, P_RAG, 2, 0.0f, 1.0f, 0.0f, under * 0.5f);
    f_line(x + 1.2f * s, top, x + 2.6f * s, top + 0.8f * s, P_RAG, 1);
}

static void bat_px(float x, float y, int up) {
    int bx = (int)floorf(x), by = (int)floorf(y);
    fset(bx, by, P_BLACK);
    for (int side = -1; side <= 1; side += 2) {
        fset(bx + side, by - (up ? 1 : 0), P_BLACK);
        fset(bx + 2 * side, by + (up ? -1 : 1), P_BLACK);
    }
}

static void art_pumpkin(double t, float talk) {
    float c = (float)cycle_begin(t, 40.0);
    ev(1.4, ART_EV_IGNITE); ev(6.6, ART_EV_IGNITE); ev(8.6, ART_EV_IGNITE); ev(15.2, ART_EV_BAT);
    ev(17.6, ART_EV_WIND); ev(20.5, ART_EV_CACKLE); ev(25.4, ART_EV_SETTLE); ev(26.6, ART_EV_DOOR);
    /* the candles: lit one after another, nearly blown out by a gust, and back */
    float gust = smoothstepf(17.7f, 18.5f, c) * (1.0f - smoothstepf(19.6f, 20.2f, c));
    float dim2 = smoothstepf(25.7f, 26.1f, c) * (1.0f - smoothstepf(26.5f, 26.9f, c));
    float lit[3] = { smoothstepf(1.4f, 2.1f, c) + 0.8f * bump(c, 1.4f, 1.8f), smoothstepf(6.6f, 7.2f, c), smoothstepf(8.6f, 9.2f, c) };
    for (int i = 0; i < 3; i++) {
        float fl = 0.88f + 0.08f * sinf((float)t * (11.0f + i * 2.3f) + i) * sinf((float)t * 4.7f + i * 1.7f) + 0.04f * sinf((float)t * 29.0f + i);
        fl -= gust * 0.25f * (0.5f + 0.5f * sinf((float)t * 23.0f + i * 2.0f));
        g_plit[i] = lit[i] * fl * (1.0f - 0.9f * gust) * (1.0f - 0.75f * dim2) * (1.0f + 0.45f * bump(c, 20.0f, 21.0f));
    }
    float evil = smoothstepf(19.3f, 19.8f, c);
    float mouth = clampf((talk - 0.3f) / 0.7f, 0.0f, 1.0f);
    if (c > 20.5f && c < 22.1f) mouth = fmaxf(mouth, (0.5f + 0.5f * sinf((c - 20.5f) * 6.2832f * 5.5f)) * bump(c, 20.4f, 22.1f));
    float lid = 2.6f * smoothstepf(22.6f, 23.2f, c) * (1.0f - smoothstepf(25.0f, 25.4f, c));
    if (c > 23.2f && c < 25.0f) lid += 0.5f * fabsf(sinf(c * 31.0f));
    float look = evil > 0.5f ? 0.35f * sinf(c * 1.3f) : 0.0f;
    g_sceye = smoothstepf(27.6f, 28.6f, c);
    cv_begin();
    g_clip = 1e9f;
    /* sky, with a harvest moon low over the field */
    const float mx = 63.0f, my = 12.0f, mr = 7.5f;
    for (int y = 0; y < P_HORIZON; y++)
        for (int x = 0; x < PW; x++) {
            float ddx = (x + 0.5f - mx) / mr, ddy = (y + 0.5f - my) / (mr * 1.125f), d = sqrtf(ddx * ddx + ddy * ddy);
            float v = 0.1f + 0.75f * powf(y / (float)P_HORIZON, 1.5f) + 0.5f / (1.0f + (d - 1.0f) * (d - 1.0f) * 3.0f) * (d > 1.0f);
            bset(x, y, dpick(P_SKY, 6, v, x, y));
            if (d <= 1.0f) {
                float m = 0.85f - 0.25f * d * d + 0.15f * (-ddx - ddy) * 0.5f;
                m -= 0.22f * smoothstepf(0.5f, 0.75f, vnoise(x * 0.3f, y * 0.33f, 161));
                bset(x, y, dpick(P_MOON, 4, m, x, y));
            }
        }
    for (int i = 0; i < 28; i++) {
        int sx = (int)(hash01(i, 1, 163) * 80), sy = (int)(hash01(i, 2, 163) * 15);
        if (fabsf(sx - mx) < 13 && fabsf(sy - my) < 13) continue;
        if (hash01(i, 3, 163) + 0.15f * sinf((float)t * (1.3f + i * 0.21f)) > 0.5f) bset(sx, sy, P_STAR);
    }
    for (int y = 2; y < 22; y++)                                       /* long thin cloud, drifting over the moon */
        for (int x = 0; x < PW; x++) {
            float n = fbm2((x + (float)t * 0.7f) / 15.0f, y / 2.0f, 165);
            if (n < 0.62f) continue;
            float near = 1.0f / (1.0f + ((x - mx) * (x - mx) + (y - my) * (y - my)) / 80.0f);
            if (cbg[y][x] >= P_MOON && cbg[y][x] < P_MOON + 4 && (n - 0.62f) * 50.0f < BAYER4[y & 3][x & 3] + 0.5f) continue;
            bset(x, y, dpick(P_CLOUD, 3, (n - 0.62f) * 3.0f + near, x, y));
        }
    for (int x = 0; x < PW; x++) {                                     /* the far treeline, and a barn */
        int h = P_HORIZON - 2 - (int)(2.2f * vnoise(x * 0.3f, 0.5f, 167) + 1.2f * vnoise(x * 0.9f, 1.5f, 167));
        for (int y = h; y < P_HORIZON; y++) bset(x, y, P_FAR);
    }
    for (int y = 16; y < P_HORIZON; y++)
        for (int x = 0; x <= 9; x++) {
            float dx = fabsf(x + 0.5f - 4.5f);
            float roof = dx < 4.0f ? 16.0f + dx * 0.5f : 18.0f + (dx - 4.0f) * 1.6f;   /* gambrel */
            if (y >= roof) bset(x, y, P_FAR);
        }
    bset(4, 22, P_BARNWIN); bset(5, 22, P_BARNWIN);
    /* the patch: furrows running away from you, vines, pumpkins, and the candlelight on the earth */
    for (int y = P_HORIZON; y < PH; y++)
        for (int x = 0; x < PW; x++) {
            float px = x + 0.5f, py = y + 0.5f, depth = (py - P_HORIZON) / (PH - P_HORIZON);
            float pool = 0.0f;
            for (int i = 0; i < 3; i++) {
                const Lantern *L = &LANTERN[i];
                float dx = (px - L->cx) / (L->rx * 1.5f), dy = (py - (L->cy + L->ry * 0.8f)) / (L->ry * 0.75f);
                pool += g_plit[i] * (i ? 0.8f : 1.1f) / (1.0f + dx * dx + dy * dy);
            }
            float furrow = sinf(atan2f(px - 40.0f, py - 16.0f) * 30.0f);
            float v = 0.06f + 0.14f * depth + 0.07f * smoothstepf(0.5f, 0.9f, furrow) + (vnoise(px * 0.3f, py * 0.6f, 169) - 0.5f) * 0.05f + pool * 0.7f;
            uint8_t col = dpick(P_FIELD, 6, v, x, y);
            float n = vnoise(px * 0.22f, py * 0.45f, 171);
            if (y > P_HORIZON + 1 && fabsf(n - 0.5f) < 0.022f + 0.012f * depth)
                col = dpick(P_VINE, 4, 0.3f + 0.25f * depth + pool * 0.8f, x, y);
            else if (y > P_HORIZON + 2 && fabsf(n - 0.5f) < 0.06f && hash01(x / 2, y, 173) > 0.93f)
                col = dpick(P_VINE, 4, 0.45f + 0.2f * depth + pool * 0.8f, x, y);   /* a leaf on the vine */
            bset(x, y, col);
        }
    static const float BGP[12][3] = { {6, 30, 1.2f}, {18, 31.5f, 1.4f}, {33, 29.5f, 1.0f}, {47, 29, 0.9f}, {57, 30.5f, 1.2f}, {75, 31, 1.5f},
                                      {3, 42, 2.4f}, {26, 46, 2.4f}, {54, 46.5f, 2.8f}, {77, 45, 2.6f}, {29, 34, 1.6f}, {52, 34, 1.7f} };
    for (int i = 0; i < 12; i++) {                                    /* the rest of the patch, uncarved */
        float cx = BGP[i][0], cy = BGP[i][1], r = BGP[i][2];
        for (int y = (int)floorf(cy - r); y <= (int)ceilf(cy + r); y++)
            for (int x = (int)floorf(cx - r * 1.3f); x <= (int)ceilf(cx + r * 1.3f); x++) {
                float nx = (x + 0.5f - cx) / (r * 1.3f), ny = (y + 0.5f - cy) / r;
                if (nx * nx + ny * ny > 1.0f) continue;
                float pool = 0.0f;
                for (int j = 0; j < 3; j++) {
                    float dx = cx - LANTERN[j].cx, dy = (cy - LANTERN[j].cy) * 1.125f;
                    pool += g_plit[j] * 0.7f / (1.0f + (dx * dx + dy * dy) / 120.0f);
                }
                float rib = 0.6f + 0.4f * fabsf(sinf(nx * 4.5f));
                float v = (0.16f + 0.5f * clampf(0.7f * nx - 0.6f * ny, 0.0f, 1.0f) + pool) * rib;
                bset(x, y, dpick(P_RIND, 8, v, x, y));
            }
        bset((int)cx, (int)floorf(cy - r - 0.2f), P_STEM);
    }
    /* the scarecrow keeps its place in the field, until it doesn't */
    if (c < 19.0f) scarecrow(16.5f, 31.0f, 1.0f, 0.0f, 1);
    else if (c > 26.2f) scarecrow(61.5f, 46.0f, 1.75f, clampf(g_plit[0] * 0.5f + g_plit[2] * 0.6f, 0.0f, 1.0f), 0);
    /* bats over the moon */
    for (int i = 0; i < 3; i++) {
        float u = c - 13.0f - i * 0.6f;
        if (u < 0.0f || u > 4.8f) continue;
        bat_px(-3.0f + u * (19.0f + i * 3.0f), 8.0f + i * 3.0f + sinf(u * 3.0f + i) * 2.0f - u * 0.6f, sinf(u * 22.0f + i * 2.0f) > 0.0f);
    }
    lantern(1, 0.0f, 0.0f, 0.0f, 0.0f);
    lantern(2, 0.0f, 0.0f, 0.0f, 0.0f);
    lantern(0, evil, mouth, lid, look);
    if (lid > 0.3f || (c > 23.0f && c < 28.0f)) {                       /* smoke out of the top */
        float gy = LANTERN[0].cy - LANTERN[0].ry * 0.85f - lid;
        for (int i = 0; i < 14; i++) {
            float ph = fmodf((c - 23.0f) * 0.55f + hash01(i, 1, 175), 1.0f);
            if (c - 23.0f < ph / 0.55f && c < 25.5f) continue;
            float sx = LANTERN[0].cx + (hash01(i, 2, 175) - 0.5f) * 8.0f + sinf(ph * 6.0f + i) * 2.0f * ph;
            float sy = gy - ph * 15.0f;
            if (hash01(i, (int)(ph * 9.0f), 177) < 1.0f - ph) fset((int)sx, (int)sy, (uint8_t)(P_SMOKE + (ph < 0.4f)));
        }
    }
    for (int i = 0; i < 16; i++) {                                     /* leaves on the gust */
        float u = c - 17.4f - hash01(i, 1, 179) * 1.2f;
        if (u < 0.0f || u > 3.2f) continue;
        float x = 82.0f - u * (26.0f + hash01(i, 2, 179) * 14.0f), y = 18.0f + hash01(i, 3, 179) * 28.0f + sinf(u * 5.0f + i) * 2.0f - u * 2.0f;
        fset((int)x, (int)y, (uint8_t)(P_LEAF + (i & 1)));
        if (i & 2) fset((int)x + 1, (int)y, P_LEAF);
    }
    cv_flush();
}

static void pumpkin_palette(uint8_t pal[256][3]) {
    static const uint8_t P[P_LEAF + 2 - 64][3] = {
        {1,0,3},{2,1,5},{4,2,8},{7,3,11},{11,5,13},{16,8,14},          /* sky */
        {40,22,6},{52,32,10},{60,42,16},{63,52,26},                    /* harvest moon */
        {5,3,8},{9,5,11},{14,8,13},                                    /* cloud */
        {30,28,36},{1,0,2},{44,26,6},                                  /* star, far, barn window */
        {1,1,2},{3,2,2},{6,4,2},{11,6,2},{20,10,2},{32,16,3},          /* the field, cold to candlelit */
        {1,3,1},{2,6,2},{4,10,3},{10,16,4},                            /* vines */
        {4,1,0},{9,3,0},{15,5,0},{22,8,0},{30,12,1},{40,17,2},{50,24,4},{60,33,8},   /* rind */
        {40,8,0},{58,22,2},{63,40,8},{63,58,26},                       /* hero glow */
        {58,22,2},{63,48,12},{58,22,2},{63,48,12},                     /* the small ones */
        {6,8,2},{11,13,4},{17,19,7},                                   /* stem */
        {0,0,0},{20,15,5},{36,29,11},{2,1,3},{22,10,3},{63,18,4},      /* black, straw, rags, its eyes */
        {9,8,8},{16,15,14},{24,10,3},{36,18,5},                        /* smoke, leaves */
    };
    for (int i = 0; i < P_LEAF + 2 - 64; i++)
        for (int k = 0; k < 3; k++) pal[64 + i][k] = P[i][k];
    static const int BASE[3] = { P_GLOW, P_GLOW_L, P_GLOW_R }, N[3] = { 4, 2, 2 };
    for (int l = 0; l < 3; l++)
        for (int i = 0; i < N[l]; i++)
            for (int k = 0; k < 3; k++) pal[BASE[l] + i][k] = (uint8_t)clampi((int)(P[BASE[l] - 64 + i][k] * g_plit[l] + 0.5f), 0, 63);
    for (int k = 0; k < 3; k++) pal[P_SCEYE][k] = (uint8_t)(P[P_SCEYE - 64][k] * g_sceye);
}

/* ------------------------------------------------------------------ */
/* The ossuary: a skull in its niche, and a wall of them listening     */
/* ------------------------------------------------------------------ */

enum {
    O_WALL = 64, O_STONE = 70, O_BONE = 75, O_BLACK = 83, O_EYE = 84, O_WEYE = 87, O_FLAME = 88,
    O_WAX = 91, O_CLOTH = 94, O_SMOKE = 98, O_WEB = 99
};
static float g_oeye, g_oweye, g_oc[2];

static const float OC_X[2] = { 19.0f, 61.0f };
#define OC_Y 30.0f                                  /* the candle flames */
#define OS_X 40.0f
#define OS_Y 19.0f                                  /* the middle of the skull's eye line */

static float o_light(float x, float y, float R) {
    float L = 0.0f;
    for (int i = 0; i < 2; i++) {
        float dx = x - OC_X[i], dy = (y - OC_Y) * 1.125f;
        L += g_oc[i] / (1.0f + (dx * dx + dy * dy) / (R * R));
    }
    return L;
}

static int in_niche(float x, float y) {
    float dx = x - OS_X;
    if (fabsf(dx) > 16.0f || y > 40.0f) return 0;
    return y >= 14.0f - sqrtf(fmaxf(0.0f, 256.0f - dx * dx)) * 0.8f;
}

/* one skull of the wall, 7x6: 1-3 bone (dark to light), 8 socket, 9 a gap; the second set has the jaw dropped */
static const char *const WSKULL[2][6] = {
    { "0233320", "2333332", "2883882", "2329232", "0323230", "0022200" },
    { "0233320", "2333332", "2883882", "2329232", "0999990", "0323230" },
};

/* the wall: rows of skulls between bands of bone ends, the way they stack them in the catacombs */
static void ossuary_wall(float c) {
    for (int y = 0; y < 40; y++)
        for (int x = 0; x < PW; x++) {
            float px = x + 0.5f, py = y + 0.5f;
            if (in_niche(px, py)) {                                    /* the niche: plain dressed stone, set back */
                float L = o_light(px, py, 22.0f) * 0.5f;
                int joint = (y % 5 == 0) || ((x + (y / 5) * 3) % 7 == 0);
                bset(x, y, dpick(O_STONE, 5, (0.06f + L) * (joint ? 0.6f : 1.0f), x, y));
                continue;
            }
            float L = o_light(px, py, 15.0f);
            if (in_niche(px - 2.0f, py) || in_niche(px + 2.0f, py) || in_niche(px, py + 2.0f)) {   /* the arch stones */
                float a = atan2f(py - 14.0f, px - OS_X);
                int seam = fabsf(fmodf(a * 8.0f / 3.14159265f + 50.0f, 1.0f) - 0.5f) > 0.42f;
                bset(x, y, dpick(O_STONE, 5, seam ? 0.05f : 0.18f + L * 0.9f, x, y));
                continue;
            }
            int row = y / 9, ry = y % 9;
            uint8_t col = dpick(O_STONE, 5, 0.03f + 0.06f * L, x, y);  /* mortar */
            if (ry < 6) {                                               /* a row of skulls */
                int off = (row & 1) * 4, sx = (x + off) % 8, id = (x + off) / 8;
                float h = hash01(id, row, 131);
                int open = c > 18.6f && c < 21.4f && (((int)(c * 11.0f + h * 7.0f)) & 1);   /* chattering */
                char k = sx < 7 ? WSKULL[open][ry][sx] : '0';
                if (h > 0.92f) k = (k == '0') ? '0' : '9';               /* one has crumbled away */
                if (k == '8') {
                    float on = 8.0f + 7.0f * hash01(id, row, 133), off_t = 31.0f + 2.0f * hash01(id, row, 135);
                    col = (hash01(id, row, 137) < 0.5f && c > on && c < off_t) ? (uint8_t)O_WEYE : (uint8_t)O_BLACK;
                } else if (k == '9') col = O_BLACK;
                else if (k != '0') col = dpick(O_WALL, 6, (0.14f + 0.19f * (k - '0')) * (0.75f + 0.35f * h) * L, x, y);
            } else if (ry < 8) {                                        /* a band of bone ends */
                int kx = (x + row * 2) % 3;
                int jaw = 0;
                if (ry == 6 && c > 18.6f && c < 21.4f) {                /* a dropped jaw hangs down over it */
                    int off = (row & 1) * 4, sx = (x + off) % 8, id = (x + off) / 8;
                    jaw = sx >= 2 && sx <= 4 && (((int)(c * 11.0f + hash01(id, row, 131) * 7.0f)) & 1) && hash01(id, row, 131) <= 0.92f;
                }
                if (jaw) col = dpick(O_WALL, 6, 0.4f * L, x, y);
                else if (kx < 2) col = dpick(O_WALL, 6, (ry == 6 ? 0.42f : 0.28f) * (0.8f + 0.4f * hash01((x + row * 2) / 3, row, 139)) * L, x, y);
            }
            bset(x, y, col);
        }
    for (int y = 40; y < PH; y++)                                      /* the ledge */
        for (int x = 0; x < PW; x++) {
            float L = o_light(x + 0.5f, y + 0.5f, 18.0f);
            float v = y < 42 ? 0.2f + L * 0.8f : (0.07f + L * 0.35f) * (1.0f - (y - 42) * 0.07f);
            if (y >= 42 && (x % 13 == 0 || y == 46)) v *= 0.5f;
            bset(x, y, dpick(O_STONE, 5, v + (hash01(x, y, 141) - 0.5f) * 0.06f, x, y));
        }
}

/* candle i: a stub of wax with its flame (level L) at (OC_X, OC_Y) */
static void o_candle(int i, double t) {
    float cx = OC_X[i], L = g_oc[i];
    int x0 = (int)cx;
    for (int y = 32; y < 41; y++)
        for (int x = x0 - 1; x <= x0 + 1; x++) {
            float v = (0.12f + 0.75f * L * (1.0f - (y - 32) / 12.0f)) * (x == x0 - 1 ? 0.7f : x == x0 + 1 ? 1.1f : 1.0f);
            fset(x, y, dpick(O_WAX, 3, v, x, y));
        }
    fset(x0 - 2, 33, O_WAX + 1); fset(x0 + 2, 34, O_WAX + 1); fset(x0 + 2, 35, O_WAX);
    for (int x = x0 - 3; x <= x0 + 3; x++) fset(x, 40, dpick(O_WAX, 3, 0.1f + 0.4f * L, x, 40));   /* the puddle */
    if (L < 0.06f) { fset(x0, 31, O_BLACK); return; }
    int sway = 0;
    float s = sinf((float)t * 9.0f + i * 2.0f) + 0.6f * sinf((float)t * 23.0f + i * 5.0f);
    if (s > 1.0f) sway = 1; else if (s < -1.0f) sway = -1;
    fset(x0, 31, L > 0.25f ? O_FLAME + 2 : O_FLAME);
    if (L > 0.35f) fset(x0, 30, O_FLAME + 1);
    if (L > 0.7f) fset(x0 + sway, 29, O_FLAME);
}

/* what the skull is made of at (u, v), its own coordinates: u columns right of the middle,
 * v rows down from the eye line.  0 nothing, 1 bone (normal and occlusion out), 2 eye socket, 3 dark */
static int skull_at(float u, float v, float drop, float *nx, float *ny, float *nz, float *ao) {
    float au = fabsf(u);
    *ao = 1.0f;
    float cu = u / 14.0f, cv = (v + 5.0f) / 15.5f;
    int part = 0;
    if (cu * cu + cv * cv <= 1.0f && v < 2.0f) {                       /* the cranium */
        float z = sqrtf(fmaxf(0.0f, 1.0f - cu * cu - cv * cv));
        *nx = cu; *ny = cv; *nz = z; part = 1;
    } else if (v >= -2.0f && v < 12.5f) {                              /* cheeks, maxilla */
        float hw = v < 3.0f ? 13.4f : (v < 5.5f ? 14.2f : lerpf(14.2f, 9.2f, smoothstepf(5.5f, 10.0f, v)));
        if (au > hw) return 0;
        *nx = u / hw * 0.9f; *ny = 0.15f; *nz = sqrtf(fmaxf(0.0f, 1.0f - *nx * *nx - 0.0225f)); part = 1;
        if (v > 3.0f && v < 5.5f && au > 9.0f) *ao *= 1.12f;          /* zygomatic arch */
        if (v > 5.5f && v < 10.0f && au > 7.0f && au < 12.0f) *ao *= 0.72f;   /* hollow under it */
    } else {
        float vj = v - drop;
        if (v >= 12.5f && vj < 12.5f) {                                /* the jaw has dropped: the mouth */
            if (au < 8.3f) return 3;
            if (au > 9.6f) return 0;
            *nx = u / 9.6f * 0.9f; *ny = 0.2f; *nz = 0.5f; *ao = 0.55f;
            return 1;
        }
        if (vj < 12.5f || vj > 18.5f) return 0;
        float hw = lerpf(9.6f, 5.4f, smoothstepf(13.0f, 18.4f, vj));
        if (au > hw || vj > 18.5f - 1.8f * (u / hw) * (u / hw)) return 0;
        *nx = u / hw * 0.9f; *ny = 0.25f + 0.3f * smoothstepf(15.0f, 18.5f, vj); *nz = sqrtf(fmaxf(0.0f, 1.0f - *nx * *nx - *ny * *ny));
        if (vj < 14.2f && au < 7.0f) {                                 /* lower teeth */
            float ph = fmodf((u + 7.0f) / 2.4f + 0.5f, 1.0f);
            *ao *= ph < 0.4f ? 0.45f : 1.1f;
        }
        if (vj > 17.6f) *ao *= 0.85f;
        part = 1;
    }
    if (v > -1.0f && v < 12.5f) {                                      /* upper teeth and the gum line over them */
        if (v > 10.0f && au < 7.6f) { float ph = fmodf((u + 7.6f) / 2.4f, 1.0f); *ao *= ph < 0.4f ? 0.45f : (v > 12.0f ? 0.9f : 1.1f); }
        else if (v > 9.2f && au < 8.0f) *ao *= 0.72f;
    }
    for (int s = -1; s <= 1; s += 2) {                                 /* eye sockets, slightly drooping at the outer corners */
        float du = (u - s * 5.8f) / 4.6f, dv = (v - 0.3f - 0.12f * (u - s * 5.8f) * s) / 4.8f;
        float q = powf(fabsf(du), 2.4f) + powf(fabsf(dv), 2.4f);
        if (q <= 1.0f) return 2;
        if (q < 1.7f) *ao *= dv < -0.6f ? 1.12f : 0.55f + 0.45f * (q - 1.0f) / 0.7f;   /* brow above, shadow below */
    }
    if (v > 3.4f && v < 8.6f) {                                        /* the nose */
        float hwn = 0.3f + 1.9f * smoothstepf(3.4f, 7.6f, v);
        if (v > 7.8f) hwn *= sqrtf(fmaxf(0.0f, 1.0f - (v - 7.8f) * (v - 7.8f) / 0.64f));
        if (au < hwn) { if (au < 0.4f && v > 5.8f) { *ao *= 0.6f; return 1; } return 3; }
        if (au < hwn + 1.2f) *ao *= 0.8f;
    }
    if (au > 11.0f && v > -7.0f && v < 3.0f) *ao *= 0.78f;             /* temples */
    static const float CRACK[5][2] = { {-9.0f, -17.0f}, {-6.0f, -12.5f}, {-7.2f, -9.5f}, {-4.0f, -6.5f}, {-4.6f, -5.0f} };
    for (int i = 0; i < 4; i++) {
        float ax = CRACK[i][0], ay = CRACK[i][1], bx = CRACK[i + 1][0], by = CRACK[i + 1][1];
        float vx = bx - ax, vy = by - ay, k = clampf(((u - ax) * vx + (v - ay) * vy) / (vx * vx + vy * vy), 0.0f, 1.0f);
        float ex = u - (ax + vx * k), ey = v - (ay + vy * k);
        if (ex * ex + ey * ey < 0.3f) *ao *= 0.3f;
    }
    *ao *= 0.88f + 0.24f * vnoise(u * 0.4f + 20.0f, v * 0.4f + 20.0f, 143);   /* old stains */
    return part;
}

#define OS_SIZE 0.8f
static void hero_skull(float cx, float cy, float roll, float look, float jaw) {
    float cr = cosf(roll), sr = sinf(roll), drop = jaw * 3.4f;
    float lu[2], lv[2];
    for (int i = 0; i < 2; i++) {                                      /* the candles, in the skull's frame */
        float X = (OC_X[i] - cx) * 1.125f, Y = OC_Y - cy;
        lu[i] = (X * cr + Y * sr) / 1.125f / OS_SIZE; lv[i] = (-X * sr + Y * cr) / OS_SIZE;
    }
    for (int y = (int)(cy - 23.0f); y <= (int)(cy + 24.0f); y++)
        for (int x = (int)(cx - 18.0f); x <= (int)(cx + 18.0f); x++) {
            float X = (x + 0.5f - cx) * 1.125f, Y = y + 0.5f - cy;
            float u = (X * cr + Y * sr) / 1.125f / OS_SIZE, v = (-X * sr + Y * cr) / OS_SIZE;
            float nx = 0, ny = 0, nz = 1, ao = 1;
            int m = skull_at(u, v, drop, &nx, &ny, &nz, &ao);
            if (!m) continue;
            if (m == 3) { fset(x, y, O_BLACK); continue; }
            if (m == 2) {                                              /* an ember deep in the socket */
                float ex = u - (u < 0 ? -5.8f : 5.8f) - look * 1.1f, ey = v - 0.9f;
                float r = sqrtf(ex * ex * 1.27f + ey * ey);
                uint8_t col = O_BLACK;
                if (r < 0.9f) col = O_EYE + 2;
                else if (r < 1.8f) col = O_EYE + 1;
                else if (r < 3.2f && (3.2f - r) * 12.0f > BAYER4[y & 3][x & 3] + 0.5f) col = O_EYE;
                fset(x, y, col);
                continue;
            }
            float L = 0.08f;
            for (int i = 0; i < 2; i++) {
                float dx = (lu[i] - u) * 1.125f, dy = lv[i] - v, dz = 22.0f - nz * 12.0f;
                float d = sqrtf(dx * dx + dy * dy + dz * dz) + 1e-3f;
                float dif = (nx * dx + ny * dy + nz * dz) / d;
                L += g_oc[i] * fmaxf(dif, 0.0f) / (1.0f + d * d / 700.0f);
            }
            L += 0.10f * g_oeye * clampf(1.0f - (fabsf(v - 0.3f) + fabsf(fabsf(u) - 5.8f)) / 9.0f, 0.0f, 1.0f);   /* the embers' own glow */
            fset(x, y, dpick(O_BONE, 8, L * ao * 1.25f, x, y));
        }
}

static void art_skull(double t, float talk) {
    float c = (float)cycle_begin(t, 40.0);
    ev(1.2, ART_EV_IGNITE); ev(18.6, ART_EV_CHATTER); ev(20.0, ART_EV_CHATTER); ev(24.6, ART_EV_WIND); ev(26.2, ART_EV_SETTLE);
    float gust = smoothstepf(24.8f, 25.6f, c) * (1.0f - smoothstepf(27.0f, 27.6f, c));
    float lit[2] = { 1.0f, smoothstepf(1.2f, 1.7f, c) + 0.6f * bump(c, 1.2f, 1.5f) };
    for (int i = 0; i < 2; i++) {
        float fl = 0.88f + 0.08f * sinf((float)t * (12.0f + i * 3.1f) + i) * sinf((float)t * 5.1f + i) + 0.04f * sinf((float)t * 31.0f + i * 2.0f);
        g_oc[i] = lit[i] * fl * (1.0f - 0.95f * gust) * (1.0f + 0.4f * bump(c, 27.2f, 28.4f));
    }
    float jaw = clampf((talk - 0.2f) / 0.6f, 0.0f, 1.0f);
    int moved = c > 26.2f;
    g_oeye = smoothstepf(2.0f, 3.0f, c) * (0.8f + 0.1f * sinf((float)t * 3.0f) + 0.3f * jaw) + 0.5f * gust + (moved ? 0.25f : 0.0f);
    g_oweye = 0.75f + 0.25f * sinf((float)t * 2.3f);
    float look = moved ? 0.0f : (c > 4.0f ? 0.6f * sinf((c - 4.0f) * 0.45f) : 0.0f);
    cv_begin();
    g_clip = 1e9f;
    ossuary_wall(c);
    for (int y = 35; y <= 41; y++)                                     /* the velvet it rests on */
        for (int x = 26; x <= 54; x++) {
            float nx = (x + 0.5f - OS_X) / 13.5f, ny = (y + 0.5f - 38.4f) / 2.8f;
            if (nx * nx + ny * ny > 1.0f) continue;
            float L = o_light(x + 0.5f, y + 0.5f, 20.0f);
            fset(x, y, dpick(O_CLOTH, 4, (0.12f + L * 0.9f) * (1.0f - 0.4f * ny), x, y));
        }
    hero_skull(OS_X + (moved ? 0.6f : 0.0f), OS_Y + (moved ? 0.4f : 0.0f), moved ? -0.15f : 0.0f, look, jaw);
    o_candle(0, t); o_candle(1, t);
    if (c > 25.0f && c < 29.0f)                                        /* smoke off the snuffed wicks */
        for (int i = 0; i < 2; i++)
            for (int k = 0; k < 6; k++) {
                float ph = fmodf((c - 25.0f) * 0.7f + k / 6.0f, 1.0f);
                if (c - 25.0f < ph / 0.7f) continue;
                float sx = OC_X[i] + sinf(ph * 7.0f + k + i) * 1.6f * ph, sy = OC_Y - 1.0f - ph * 12.0f;
                if (hash01(k, (int)(ph * 8.0f), 145 + i) < 1.0f - ph) fset((int)sx, (int)sy, O_SMOKE);
            }
    for (int i = 0; i < 8; i++) {                                      /* dust in the candlelight */
        float dx = OS_X + (hash01(i, 1, 147) - 0.5f) * 70.0f + sinf((float)t * 0.3f + i) * 2.0f;
        float dy = fmodf(hash01(i, 2, 147) * 40.0f + (float)t * (0.4f + 0.4f * hash01(i, 3, 147)), 40.0f);
        if (o_light(dx, dy, 16.0f) > 0.3f) ovset((int)dx, (int)(dy / 2.0f), 0xFA, O_WEB);
    }
    cv_flush();
}

static void skull_palette(uint8_t pal[256][3]) {
    static const uint8_t P[O_WEB + 1 - 64][3] = {
        {3,2,1},{8,6,4},{14,12,8},{22,19,13},{32,28,20},{44,39,28},     /* the wall's bones */
        {1,1,1},{4,3,3},{8,7,6},{13,12,10},{20,18,15},                  /* stone */
        {4,3,2},{9,7,5},{15,13,9},{23,20,14},{32,28,20},{41,37,27},{50,46,35},{58,55,44},   /* the skull */
        {0,0,0},{26,2,0},{52,10,2},{63,42,18},{44,4,2},                  /* black, embers, the wall's eyes */
        {50,16,2},{63,38,8},{63,60,36},                                 /* flame */
        {16,11,5},{36,28,14},{54,46,30},                                /* wax */
        {4,0,1},{10,1,3},{18,3,6},{28,6,10},                            /* velvet */
        {12,11,10},{16,16,14},                                          /* smoke, web */
    };
    for (int i = 0; i < O_WEB + 1 - 64; i++)
        for (int k = 0; k < 3; k++) pal[64 + i][k] = P[i][k];
    for (int i = 0; i < 3; i++)
        for (int k = 0; k < 3; k++) pal[O_EYE + i][k] = (uint8_t)clampi((int)(P[O_EYE - 64 + i][k] * fminf(g_oeye, 1.3f) + 0.5f), 0, 63);
    for (int k = 0; k < 3; k++) pal[O_WEYE][k] = (uint8_t)(P[O_WEYE - 64][k] * g_oweye);
}

void art_palette(int kind, uint8_t pal[256][3]) {
    switch (kind) {
    case ART_HOUSE:    manor_palette(pal); break;
    case ART_SPIDER:   spider_palette(pal); break;
    case ART_ZOMBIE:   zombie_palette(pal); break;
    case ART_SKELETON: skeleton_palette(pal); break;
    case ART_GHOST:    hallway_palette(pal); break;
    case ART_PUMPKIN:  pumpkin_palette(pal); break;
    case ART_SKULL:    skull_palette(pal); break;
    default: break;
    }
}

/* ------------------------------------------------------------------ */

void art_draw(int kind, double t, float talk) {
    memset(art, 0, sizeof art);
    switch (kind) {
    case ART_PUMPKIN: art_pumpkin(t, talk); break;
    case ART_SKULL:   art_skull(t, talk); break;
    case ART_HOUSE:   art_manor(t); break;
    case ART_SPIDER:  art_spider(t); break;
    case ART_GHOST:   art_hallway(t); break;
    case ART_ZOMBIE:  art_zombies(t); break;
    case ART_SKELETON: art_skeletons(t); break;
    default: break;
    }
}
