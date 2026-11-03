/* The carved stone (docs/PLAN-ui.md): the marble, the slabs and wells cut from it, its rules and ornaments, text cut
 * or inlaid, the sockets and the Alignment's stone. Only gfx and the palette's ramps; the game and the sheet both draw
 * with it. */
#include "crawl_ui.h"
#include "gfx.h"
#include <stdlib.h>
#include <stdio.h>

#define MW 320
#define MH 200

/* ---- the marble ---- */

static uint8_t g_mshade[MW * MH];                  /* the slab's shade at each pixel of the screen, 0..15 */
int ui_mr = R_CEIL, ui_wr = R_FLOOR;
void ui_ramps(int marble) { ui_mr = marble ? R_CEIL : R_STONE; ui_wr = marble ? R_FLOOR : R_STONE; }

/* ---- the fit checks ---- */
int ui_check;
const char *ui_where = "?";
static uint16_t g_ink[MW * MH], g_ink_id = 1;      /* which frame's glyph last inked each pixel */
static int box_x0, box_y0, box_x1, box_y1, box_on;  /* the inside of the last stone cut, its moulding left out */
void ui_check_frame(void) { if (++g_ink_id == 0) { memset(g_ink, 0, sizeof g_ink); g_ink_id = 1; } box_on = 0; }
void ui_box_none(void) { box_on = 0; }
static void box_set(int x, int y, int w, int h) { box_x0 = x + 4; box_y0 = y + 3; box_x1 = x + w - 4; box_y1 = y + h - 3; box_on = 1; }
/* a glyph just drawn at x, y, w by h with its ink in g (rows of 8 bits, sc to a pixel): stamped, and told if it
 * lands on another's ink or leaves its stone */
static void ink(int x, int y, const unsigned char *g, int rows, int sc, uint8_t ch) {
    if (!ui_check) return;
    int over = 0, out = 0;
    for (int r = 0; r < rows; r++) for (int c = 0; c < 8; c++) {
        if (!(g[r] >> (7 - c) & 1)) continue;
        for (int j = 0; j < sc; j++) for (int i = 0; i < sc; i++) {
            int px = x + c * sc + i, py = y + r * sc + j;
            if ((unsigned)px >= MW || (unsigned)py >= MH) { out = 1; continue; }
            if (box_on && (px < box_x0 || px >= box_x1 || py < box_y0 || py >= box_y1)) out = 1;
            if (g_ink[py * MW + px] == g_ink_id) over = 1;
            g_ink[py * MW + px] = g_ink_id;
        }
    }
    if (over || out) {                                              /* told once a row of text */
        char w[64];
        snprintf(w, sizeof w, "%s %s on row %d", ui_where, over ? "overlap" : "outside", y);
        char what[24];
        snprintf(what, sizeof what, "x %d '%c'", x, ch >= 32 && ch < 127 ? ch : '?');
        ui_overflow(w, what, 0);
    }
}

static void ramp3(uint8_t pal[256][3], int first, const float a[3], const float b[3], float gamma) {
    for (int i = 0; i < 16; i++) {
        float k = powf((float)i / 15.0f, gamma);
        for (int c = 0; c < 3; c++) {
            float v = a[c] + (b[c] - a[c]) * k;
            pal[first + i][c] = (uint8_t)(v < 0 ? 0 : v > 63 ? 63 : v + 0.5f);
        }
    }
}

void ui_marble_pal(uint8_t pal[256][3], const uint8_t stone_mid[3]) {
    float lean[3] = { 1, 1, 1 };                   /* a tenth towards the run's stone, as bright as ever */
    if (stone_mid) {
        float m = (stone_mid[0] + stone_mid[1] + stone_mid[2]) / 3.0f;
        if (m > 0.5f) for (int c = 0; c < 3; c++) lean[c] = 1.0f + 0.1f * ((float)stone_mid[c] / m - 1.0f);
    }
    const float sa[3] = { 4.0f * lean[0], 4.0f * lean[1], 5.0f * lean[2] }, sb[3] = { 56.0f * lean[0], 56.0f * lean[1], 57.0f * lean[2] };
    const float wa[3] = { 4.0f * lean[0], 4.0f * lean[1], 5.0f * lean[2] }, wb[3] = { 38.0f * lean[0], 38.0f * lean[1], 42.0f * lean[2] };
    ramp3(pal, R_CEIL, sa, sb, 1.15f);             /* the slab: weathered grey, groove to vein */
    ramp3(pal, R_FLOOR, wa, wb, 1.05f);            /* the wells: a cooler, darker stone, never black */
}

/* While a menu is up the floor's and ceiling's ramps are the marble's. What of the crypt is already drawn in them goes to
 * the run's stone entry nearest it in brightness first, so the dimmed crypt behind looks as it did. */
void ui_lend(Frame *f) {
    uint8_t to[32];
    for (int i = 0; i < 32; i++) {
        const uint8_t *p = f->pal[R_FLOOR + i];
        int y = p[0] * 3 + p[1] * 6 + p[2], best = 0, bd = 1 << 30;
        for (int k = 0; k < 16; k++) {
            const uint8_t *q = f->pal[R_STONE + k];
            int d = abs(q[0] * 3 + q[1] * 6 + q[2] - y);
            if (d < bd) { bd = d; best = k; }
        }
        to[i] = (uint8_t)(R_STONE + best);
    }
    int n = f->w * f->h;
    for (int i = 0; i < n; i++) { uint8_t c = f->pix[i]; if (c >= R_FLOOR && c < R_CEIL + 16) f->pix[i] = to[c - R_FLOOR]; }
    ui_marble_pal(f->pal, f->pal[R_STONE + 8]);
}

static uint32_t hash2(int x, int y, unsigned s) {
    uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u + s * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}
static float vnoise(float x, float y, unsigned s) {
    int xi = (int)floorf(x), yi = (int)floorf(y);
    float fx = x - xi, fy = y - yi;
    fx = fx * fx * (3 - 2 * fx); fy = fy * fy * (3 - 2 * fy);
    float a = (hash2(xi, yi, s) & 0xFFFF) / 65535.0f, b = (hash2(xi + 1, yi, s) & 0xFFFF) / 65535.0f;
    float c = (hash2(xi, yi + 1, s) & 0xFFFF) / 65535.0f, d = (hash2(xi + 1, yi + 1, s) & 0xFFFF) / 65535.0f;
    return a + (b - a) * fx + (c - a) * fy + (a - b - c + d) * fx * fy;
}
static float fbm(float x, float y, unsigned s, int oct) {
    float v = 0, amp = 0.5f, sum = 0;
    for (int i = 0; i < oct; i++) { v += amp * vnoise(x, y, s + (unsigned)i * 101u); sum += amp; x *= 2.03f; y *= 2.03f; amp *= 0.5f; }
    return v / sum;
}

void ui_marble_make(unsigned seed) {
    static const float BAYER[4][4] = { { 0, 8, 2, 10 }, { 12, 4, 14, 6 }, { 3, 11, 1, 9 }, { 15, 7, 13, 5 } };
    float ang = (float)(hash2(1, 2, seed) % 628) / 100.0f;
    float ca = cosf(ang) * 0.030f, sa = sinf(ang) * 0.030f;
    for (int y = 0; y < MH; y++)
        for (int x = 0; x < MW; x++) {
            float m = fbm(x * 0.045f, y * 0.045f, seed, 4);                    /* the body's clouding */
            float tu = fbm(x * 0.012f, y * 0.012f, seed + 7u, 4) * 9.0f;       /* the veins wander */
            float v1 = 1.0f - fabsf(sinf(x * ca + y * sa + tu));
            float v2 = 1.0f - fabsf(sinf(x * sa * 1.7f - y * ca * 1.3f + tu * 0.7f + 2.0f));
            v1 = powf(v1, 7.0f); v2 = powf(v2, 11.0f) * 0.45f;        /* soft, tonal veins, not a crackle */
            float sh = 4.9f + (m - 0.5f) * 2.6f + (v1 + v2) * 2.3f;
            if ((hash2(x, y, seed + 3u) % 97u) == 0) sh -= 1.3f;              /* pits */
            sh += (BAYER[y & 3][x & 3] / 16.0f) - 0.47f;
            int k = (int)floorf(sh);
            g_mshade[y * MW + x] = (uint8_t)(k < 1 ? 1 : k > 11 ? 11 : k);
        }
    for (int c = 0; c < 5; c++) {                                               /* hairline cracks */
        uint32_t h = hash2(c, 99, seed);
        float x = (float)(h % MW), y = (float)((h >> 9) % MH), a = (float)((h >> 17) % 628) / 100.0f;
        int len = 30 + (int)((h >> 3) % 70);
        for (int i = 0; i < len; i++) {
            a += ((float)(hash2(c, i, seed) % 100) / 100.0f - 0.5f) * 0.9f;
            x += cosf(a); y += sinf(a);
            int xi = (int)x, yi = (int)y;
            if (xi < 0 || yi < 0 || xi >= MW - 1 || yi >= MH - 1) break;
            g_mshade[yi * MW + xi] = 1;
            if (g_mshade[(yi + 1) * MW + xi + 1] > 1 && g_mshade[(yi + 1) * MW + xi + 1] < 9) g_mshade[(yi + 1) * MW + xi + 1] += 2;
        }
    }
}

static int mshade(int x, int y) { return (unsigned)x < MW && (unsigned)y < MH ? g_mshade[y * MW + x] : 5; }

/* ---- slabs, wells, rules ---- */

static int in_shape(int kind, int x, int y, int w, int h) {   /* a headstone's arch; a chip off each corner */
    if (kind == SLAB_HEAD) {
        int r = w / 2, ah = w / 5 > 22 ? 22 : w / 5;           /* the arch: an ellipse r across, ah high */
        if (y < ah) { float dx = (x - (w - 1) * 0.5f) / r, dy = (ah - y) / (float)ah; if (dx * dx + dy * dy > 1.0f) return 0; }
        return 1;
    }
    int c = kind == SLAB_TABLET ? 1 : 2;
    if ((x < c && y < c - x) || (x >= w - c && y < x - (w - 1 - c)) ) return 0;
    if ((x < c && h - 1 - y < c - x) || (x >= w - c && h - 1 - y < x - (w - 1 - c))) return 0;
    return 1;
}

void ui_slab(Frame *f, int x, int y, int w, int h, int kind, int orn, float t) {
    int calm = kind & SLAB_CALM;
    kind &= ~SLAB_CALM;
    box_set(x, y, w, h);
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            if (!in_shape(kind, i, j, w, h)) continue;
            int s = mshade(x + i, y + j);
            int edge = 0;                                      /* how far in from the shape's edge, up to 5 */
            int inner = i >= 5 && j >= (kind == SLAB_HEAD ? 30 : 5) && i < w - 5 && j < h - 5;   /* well inside: no edge to look for */
            for (int d = 1; d <= 5 && !edge && !inner; d++)
                if (!in_shape(kind, i - d, j, w, h) || !in_shape(kind, i + d, j, w, h) || !in_shape(kind, i, j - d, w, h) || !in_shape(kind, i, j + d, w, h)) edge = d;
            if (edge) {
                int lit = !in_shape(kind, i - edge, j, w, h) || !in_shape(kind, i, j - edge, w, h);   /* the moulding, lit from above left */
                int shad = !in_shape(kind, i + edge, j, w, h) || !in_shape(kind, i, j + edge, w, h);
                if (edge == 1) s = 1;
                else if (edge == 2 || edge == 3) s = lit && !shad ? (edge == 2 ? 12 : 10) : shad && !lit ? (edge == 2 ? 3 : 4) : 7;
                else if (edge == 4) s = lit && !shad ? 2 : shad && !lit ? 9 : 4;   /* the groove inside the moulding */
                else s = s > 8 ? 8 : s;
            } else if (calm) s = 2 + (s * 5 + 4) / 9;                /* 1..11 to 2..8: the veins soft, dim words still read */
            gfx_px(f, x + i, y + j, UI_M(s));
        }
    if (orn && kind != SLAB_TABLET) {                          /* the owner's sign in the top corners */
        ui_orn(f, x + 7, y + (kind == SLAB_HEAD ? h / 6 + 4 : 7), orn, t);
        ui_orn(f, x + w - 15, y + (kind == SLAB_HEAD ? h / 6 + 4 : 7), orn, t + 0.37f);
    }
}

void ui_well(Frame *f, int x, int y, int w, int h) {
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            int s;
            if (i == 0 || j == 0) s = -1;                          /* the cut's shadow, top and left */
            else if (i == w - 1 || j == h - 1) s = -2;             /* its lit lip, foot and right */
            else s = mshade(x + i, y + j) - (i == 1 || j == 1);
            uint8_t c = s == -1 ? UI_W(0) : s == -2 ? UI_M(10) : UI_W(s < 2 ? 2 : s > 8 ? 8 : s);
            gfx_px(f, x + i, y + j, c);
        }
}

void ui_niche(Frame *f, int x, int y, int w, int h) {
    int ah = w / 4;                                                 /* the arch: an ellipse across the top */
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            float dx = (i - (w - 1) * 0.5f) / (w * 0.5f), dy = j < ah ? (ah - j) / (float)ah : 0.0f;
            float d = dx * dx + dy * dy;
            if (d > 1.0f) continue;                                 /* outside the arch: the slab stays */
            int rim = d > 0.86f || i == 0 || i == w - 1 || j == h - 1;
            int s;
            if (rim) s = (i < w / 2 && j < h - 1) || (j < ah && dy > 0.5f) ? -1 : -2;   /* shadow up and left, the lit lip below */
            else s = mshade(x + i, y + j) - 1 + (j < h / 3 ? (h / 3 - j) * 4 / (h / 3 + 1) : 0);   /* light falls in from above */
            gfx_px(f, x + i, y + j, s == -1 ? UI_W(0) : s == -2 ? UI_M(10) : UI_W(s < 1 ? 1 : s > 9 ? 9 : s));
        }
}

void ui_rule(Frame *f, int x0, int x1, int y, int orn) {
    int mid = (x0 + x1) / 2;
    for (int x = x0; x <= x1; x++) {
        if (orn && abs(x - mid) < 6) continue;
        gfx_px(f, x, y, UI_M(1)); gfx_px(f, x, y + 1, UI_M(11));
    }
    if (orn) {                                                     /* a lozenge at its middle */
        for (int k = 0; k < 4; k++) {
            gfx_px(f, mid - k, y - 3 + k, UI_M(1)); gfx_px(f, mid + k, y - 3 + k, UI_M(1));
            gfx_px(f, mid - k, y + 4 - k, UI_M(11)); gfx_px(f, mid + k, y + 4 - k, UI_M(11));
        }
        gfx_px(f, mid, y, UI_M(13)); gfx_px(f, mid, y + 1, UI_M(13));
    }
}

/* the owners' signs, 8 by 8, cut in the stone: '#' groove, '+' lit edge, 'f' a flame */
static const char *const ORN_ART[NORN][8] = {
    { 0 },
    { "...f....", "..fff...", "...f....", "..###...", "..#+#...", "..#+#...", "..#+#...", ".#####.." },   /* the Lampstand's candle */
    { ".#..#...", ".#..#...", "........", "#.##.#..", "........", ".####...", "######..", ".####..." },   /* her paw */
    { "#....#..", ".#..#...", "..##....", "..##....", ".#..#...", "#....#..", "........", "........" },   /* bones crossed */
    { ".##.....", "#..#....", "#..#....", ".##.....", "..#.....", "..##....", "..#.....", "..##...." },   /* a key */
    { "..#.#...", ".#+#+#..", "#+#.#+#.", ".#...#..", "#+#.#+#.", ".#+#+#..", "..#.#...", "........" },   /* a rosette */
    { "........", "..####..", ".#....#.", "#..##..#", ".#....#.", "..####..", "........", "........" },   /* an eye */
};
void ui_orn(Frame *f, int x, int y, int orn, float t) {
    if (orn <= 0 || orn >= NORN) return;
    int fl = (int)(t * 9.0f) & 1;
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++) {
            char ch = ORN_ART[orn][r][c];
            if (ch == '#') { gfx_px(f, x + c, y + r, UI_M(1)); if (c < 7 && ORN_ART[orn][r][c + 1] == '.') gfx_px(f, x + c + 1, y + r, UI_M(10)); }
            else if (ch == '+') gfx_px(f, x + c, y + r, UI_M(11));
            else if (ch == 'f') gfx_px(f, x + c + (r == 0 ? fl : 0), y + r, (uint8_t)(R_FLAME + 15 - r * 3));
        }
}

/* the fit checks (section 2.4): a line that runs out of its box is told, and counted */
int ui_overflows;
void ui_overflow(const char *where, const char *what, int by) {
    ui_overflows++;
#ifdef CRAWL_DEBUG
    static uint32_t seen[512];                                      /* each told once a session */
    uint32_t h = 2166136261u;
    for (const char *p = where; *p; p++) h = (h ^ (uint8_t)*p) * 16777619u;
    for (int i = 0; i < 512; i++) { if (seen[i] == h) return; if (!seen[i]) { seen[i] = h; break; } }
    fprintf(stderr, "UI overflow %s%s%s%s\n", where, *what ? " \"" : "", what, *what ? "\"" : "");
    (void)by;
#else
    (void)where; (void)what; (void)by;
#endif
}

/* ---- text ---- */

extern unsigned char ui_face8[NFACE][128][8];              /* ui_art.c, made by ui_face_init: what a face lacks, the BIOS gives */
extern unsigned char ui_face16[128][16];
extern unsigned char ui_has8[NFACE][128], ui_has16[128];

static const unsigned char *glyph8(uint8_t ch, int face) {
    if (face > FACE_BIOS && face < NFACE && ch < 128 && ui_has8[face][ch]) return ui_face8[face][ch];
    return font_bios8[ch];
}
static void plot8(Frame *f, int x, int y, const unsigned char *g, int col, int sc) {
    for (int r = 0; r < 8; r++) for (int c = 0; c < 8; c++)
        if (g[r] >> (7 - c) & 1) { if (sc == 1) gfx_px(f, x + c, y + r, (uint8_t)col); else gfx_rect(f, x + c * sc, y + r * sc, sc, sc, (uint8_t)col); }
}
void ui_glyph8x(Frame *f, int x, int y, uint8_t ch, int face, int col, int style, int sc) {
    const unsigned char *g = glyph8(ch, face);
    if (sc < 1) sc = 1;
    if (style == TX_CUT) { plot8(f, x + sc, y + sc, g, UI_M(13), sc); plot8(f, x, y, g, UI_M(1), sc); }
    else if (style == TX_INLAID) { plot8(f, x + sc, y + sc, g, UI_M(0), sc); plot8(f, x, y, g, col, sc); }
    else plot8(f, x, y, g, col, sc);
    if (ch != ' ') ink(x, y, g, 8, sc, ch);
}
void ui_glyph8(Frame *f, int x, int y, uint8_t ch, int face, int col, int style) { ui_glyph8x(f, x, y, ch, face, col, style, 1); }
void ui_text8(Frame *f, int x, int y, const char *s, int face, int col, int style) {
    for (; *s; s++, x += 8) ui_glyph8(f, x, y, (uint8_t)*s, face, col, style);
}
static void plot16(Frame *f, int x, int y, const unsigned char *g, int col, int sc) {
    for (int r = 0; r < 16; r++) for (int c = 0; c < 8; c++) if (g[r] >> (7 - c) & 1) gfx_rect(f, x + c * sc, y + r * sc, sc, sc, (uint8_t)col);
}
void ui_glyph16x(Frame *f, int x, int y, uint8_t ch, int face, int col, int style, int sc) {
    const unsigned char *g = face == FACE_INSC && ch < 128 && ui_has16[ch] ? ui_face16[ch] : font_vga16[ch];
    if (sc < 1) sc = 1;
    if (style == TX_CUT) { plot16(f, x + sc, y + sc, g, UI_M(12), sc); plot16(f, x, y, g, UI_M(1), sc); }
    else if (style == TX_INLAID) { plot16(f, x + sc, y + sc, g, UI_M(0), sc); plot16(f, x, y, g, col, sc); }
    else plot16(f, x, y, g, col, sc);
    if (ch != ' ') ink(x, y, g, 16, sc, ch);
}
void ui_text16(Frame *f, int x, int y, const char *s, int face, int col, int style, int sc) {
    if (sc < 1) sc = 1;
    for (; *s; s++, x += 8 * sc) ui_glyph16x(f, x, y, (uint8_t)*s, face, col, style, sc);
}

/* word-wrapped lines of s, cols to a line: each line's start and length; how many */
static int wrap_lines(const char *s, int cols, const char *st[], int len[], int max) {
    int n = 0;
    while (*s == ' ') s++;
    while (*s && n < max) {
        int k = (int)strlen(s);
        if (k > cols) { k = cols; while (k > 0 && s[k] != ' ') k--; if (k == 0) k = cols; }
        st[n] = s; len[n] = k; n++;
        s += k; while (*s == ' ') s++;
    }
    return n;
}
int ui_wrap_count(const char *s, int cols) { const char *st[32]; int len[32]; return wrap_lines(s, cols, st, len, 32); }
int ui_wrap(Frame *f, int x, int y, const char *s, int cols, int lh, int maxl, int face, int col, int style) {
    const char *st[32]; int len[32];
    int n = wrap_lines(s, cols, st, len, maxl < 32 ? maxl : 32);
    for (int i = 0; i < n; i++) {
        char b[64]; int k = len[i] < 63 ? len[i] : 63;
        memcpy(b, st[i], (size_t)k); b[k] = 0;
        ui_text8(f, x, y + i * lh, b, face, col, style);
    }
    return n;
}

/* a smooth raised plate, for words cut in the stone to read on */
void ui_plate(Frame *f, int x, int y, int w, int h) {
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            int s = i == 0 || j == 0 ? 11 : i == w - 1 || j == h - 1 ? 3 : 7 + ((mshade(x + i, y + j) > 7) & ((i + j) & 1));
            gfx_px(f, x + i, y + j, UI_M(s));
        }
}

/* a hint line: in each group (two spaces apart) its first word is the key, set on a raised cap behind the 8 px cells,
 * so the line is as wide as ever; a '\1' before a letter marks a pad's face button, drawn in the button's colour */
int ui_keys_width(const char *s) { int n = 0; for (; *s; s++) n += *s != '\1'; return n * 8; }
void ui_keys(Frame *f, int x, int y, const char *s, int col) {
    char t[160]; uint8_t mark[160];                                /* the line without its marks, and which letters were marked */
    int n = 0;
    for (int m = 0; *s && n < 159; s++) { if (*s == '\1') { m = 1; continue; } t[n] = *s; mark[n++] = (uint8_t)m; m = 0; }
    t[n] = 0;
    int key = 1;
    for (int i = 0; t[i]; i++) {
        int cx = x + i * 8;
        if (t[i] == ' ') { if (t[i + 1] == ' ') key = 1; else if (key == 1 && i > 0 && t[i - 1] != ' ') key = 0; continue; }
        if (key) {
            int first = i == 0 || t[i - 1] == ' ', last = t[i + 1] == 0 || t[i + 1] == ' ';
            for (int yy = -1; yy < 9; yy++)
                for (int xx = first ? -2 : 0; xx < (last ? 9 : 8); xx++) {
                    int edge_t = yy == -1, edge_b = yy == 8, edge_l = first && xx == -2, edge_r = last && xx == 8;
                    gfx_px(f, cx + xx, y + yy, edge_t || edge_l ? UI_M(12) : edge_b || edge_r ? UI_M(2) : UI_M(8));
                }
            int c = !mark[i] ? UI_W(1) : t[i] == 'A' ? R_MOSS + 9 : t[i] == 'B' ? R_BLOOD + 9 : t[i] == 'X' ? R_GLOW + 8 : R_GOLD + 6;
            ui_glyph8(f, cx, y, (uint8_t)t[i], FACE_BODY, c, TX_PLAIN);
        } else ui_glyph8(f, cx, y, (uint8_t)t[i], FACE_BODY, col, TX_INLAID);
    }
}

/* ---- icons and sockets ---- */


void ui_icon(Frame *f, int x, int y, int icon, int ramp, int mode, int sc) {
    if (icon < 0 || icon >= NICON) return;
    static const int CUT[5] = { 1, 3, 5, 9, 12 }, DIM[5] = { 2, 3, 5, 7, 9 };
    static const int INL[5] = { 6, 9, 11, 13, 15 }, HALF[5] = { 3, 5, 7, 9, 11 };
    int top = ramp == R_GOLD ? 12 : 15;
    for (int r = 0; r < 16; r++)
        for (int c = 0, n = ui_icon_row(icon, r) ? (int)strlen(ui_icon_row(icon, r)) : 0; c < n && c < 16; c++) {
            char ch = ui_icon_row(icon, r)[c];
            if (ch < 'a' || ch > 'e') continue;
            int k = ch - 'a';
            uint8_t col;
            if (mode == IM_CUT) col = UI_M(CUT[k]);
            else if (mode == IM_DIM) col = UI_M(DIM[k]);
            else if (mode == IM_GLOW) col = k >= 3 ? (uint8_t)(ramp + (k == 4 ? 9 : 6)) : UI_M(CUT[k]);
            else if (mode == IM_HALF) col = (uint8_t)(ramp + (HALF[k] > top ? top : HALF[k]));
            else col = (uint8_t)(ramp + (INL[k] > top ? top : INL[k]));
            gfx_rect(f, x + c * sc, y + r * sc, sc, sc, col);
        }
}

void ui_mark(Frame *f, int x, int y, const char *const *mark, int ramp, int mode, int sc) {
    if (!mark || !mark[0]) return;
    static const int CUT[2] = { 9, 12 }, DIM[2] = { 6, 9 }, INL[2] = { 11, 15 }, HALF[2] = { 8, 11 };
    int top = ramp == R_GOLD ? 12 : 15, n = (int)strlen(mark[0]);
    for (int r = 0; r < n; r++) for (int c = 0; mark[r][c]; c++) {
        char ch = mark[r][c];
        if (ch != '#' && ch != 'o') continue;
        int k = ch == 'o';
        uint8_t col = mode == IM_CUT ? UI_M(CUT[k]) : mode == IM_DIM ? UI_M(DIM[k]) : mode == IM_GLOW ? (uint8_t)(ramp + (k ? 9 : 6))
                    : mode == IM_HALF ? (uint8_t)(ramp + HALF[k]) : (uint8_t)(ramp + (INL[k] > top ? top : INL[k]));
        if (mode == IM_CUT || mode == IM_DIM) gfx_rect(f, x + c * sc + sc, y + r * sc + sc, sc, sc, UI_M(1));   /* cut: its shadow */
        gfx_rect(f, x + c * sc, y + r * sc, sc, sc, col);
    }
}

static int chamfer_in(int i, int j, int s, int c) {         /* inside a square of side s with its corners cut by c */
    if (i < 0 || j < 0 || i >= s || j >= s) return 0;
    int a = i < c ? c - i : i >= s - c ? i - (s - 1 - c) : 0, b = j < c ? c - j : j >= s - c ? j - (s - 1 - c) : 0;
    return a + b <= c;
}

static void socket_ex(Frame *f, int cx, int cy, int r, int icon, const char *const *mark, int ramp, int state, int sel, int ult, float t);
void ui_socket(Frame *f, int cx, int cy, int r, int icon, int ramp, int state, int sel, int ult, float t) { socket_ex(f, cx, cy, r, icon, 0, ramp, state, sel, ult, t); }
void ui_socket_mark(Frame *f, int cx, int cy, int r, const char *const *mark, int ramp, int state, int sel, int ult, float t) {
    socket_ex(f, cx, cy, r, -1, mark, ramp, state, sel, ult, t);
}
static void socket_ex(Frame *f, int cx, int cy, int r, int icon, const char *const *mark, int ramp, int state, int sel, int ult, float t) {
    int s = r * 2, x0 = cx - r, y0 = cy - r, c = r / 3 + 1;
    float br = 0.5f + 0.5f * sinf(t * 3.2f);
    int top = ramp == R_GOLD ? 12 : 15;
    if (ult) {                                                    /* an ultimate's outer ring */
        for (int j = -3; j < s + 3; j++) for (int i = -3; i < s + 3; i++) {
            if (!chamfer_in(i + 3, j + 3, s + 6, c + 2) || chamfer_in(i + 1, j + 1, s + 2, c + 1)) continue;
            int lit = (i + j) < s;
            uint8_t col = state == SK_HELD ? (uint8_t)(R_GOLD + (lit ? 11 : 7)) : UI_M(lit ? 10 : 3);
            if (state != SK_HELD && (i == -3 || j == -3 || i == s + 2 || j == s + 2)) col = UI_M(1);
            gfx_px(f, x0 + i, y0 + j, col);
        }
    }
    for (int j = 0; j < s; j++) for (int i = 0; i < s; i++) {
        if (!chamfer_in(i, j, s, c)) continue;
        int edge = !chamfer_in(i - 1, j, s, c) || !chamfer_in(i, j - 1, s, c) ? 1 : !chamfer_in(i + 1, j, s, c) || !chamfer_in(i, j + 1, s, c) ? 2 : 0;
        int edge2 = !edge && (!chamfer_in(i - 2, j, s, c) || !chamfer_in(i, j - 2, s, c) || !chamfer_in(i + 2, j, s, c) || !chamfer_in(i, j + 2, s, c));
        uint8_t col;
        if (edge == 1) col = UI_M(1);                                  /* sunk: shadow on the upper left, */
        else if (edge == 2) col = UI_M(11);                            /* light on the lower right */
        else if (edge2) {                                              /* the rim, by its state */
            col = state == SK_HELD ? (uint8_t)(R_GOLD + 10)
                : state == SK_READY ? (uint8_t)(ramp + (int)(7 + 5 * br > top ? top : 7 + 5 * br))
                : state == SK_PART ? (uint8_t)(ramp + 8)
                : state == SK_SHORT ? UI_M(8) : UI_M(state == SK_CLOSED ? 3 : 5);
        } else col = state == SK_HELD ? UI_W(3) : UI_W(state == SK_CLOSED ? 1 : 2);
        gfx_px(f, x0 + i, y0 + j, col);
    }
    if (state == SK_HELD)                                              /* the inlay's light, under the icon */
        for (int j = 3; j < s - 3; j++) for (int i = 3; i < s - 3; i++) {
            float dx = (i - r + 0.5f) / (r - 3), dy = (j - r + 0.5f) / (r - 3);
            if (dx * dx + dy * dy < 0.8f && ((i + j) & 1)) gfx_px(f, x0 + i, y0 + j, (uint8_t)(ramp + 2));
        }
    int sc = s >= 34 ? 2 : 1, iw = 16 * sc;
    int mode = state == SK_HELD ? IM_INLAID : state == SK_PART ? IM_HALF : state == SK_READY ? IM_GLOW : state == SK_CLOSED ? IM_DIM
             : state == SK_REACH ? IM_DIM : IM_CUT;
    if (mark) {                                                    /* a mark of the game's own, doubled when it fits */
        int n = (int)strlen(mark[0]), ms = n * 2 <= s - 4 ? 2 : 1;
        ui_mark(f, cx - n * ms / 2, cy - n * ms / 2, mark, ramp, mode, ms);
    } else ui_icon(f, cx - iw / 2, cy - iw / 2, icon, ramp, mode, sc);
    if (state == SK_CLOSED) {                                          /* cracked across, a chain over the socket */
        for (int i = 1; i < s - 1; i++) {
            int yy = cy - 1 + ((i / 3) & 1);
            gfx_px(f, x0 + i, yy, (i % 3) ? UI_M(9) : UI_M(2));
            gfx_px(f, x0 + i, yy + 1, UI_M(2));
        }
        for (int k = 0; k < 4; k++) gfx_px(f, x0 + 2 + k, y0 + 2 + k * 2, UI_M(0));
    }
    if (sel) {                                                         /* gilt brackets at the four corners */
        uint8_t g = (uint8_t)(R_GOLD + (br > 0.5f ? 12 : 9));
        int o = ult ? 5 : 3, x1 = x0 - o, y1 = y0 - o, x2 = x0 + s - 1 + o, y2 = y0 + s - 1 + o;
        for (int k = 0; k < 5; k++) {
            gfx_px(f, x1 + k, y1, g); gfx_px(f, x1, y1 + k, g); gfx_px(f, x2 - k, y1, g); gfx_px(f, x2, y1 + k, g);
            gfx_px(f, x1 + k, y2, g); gfx_px(f, x1, y2 - k, g); gfx_px(f, x2 - k, y2, g); gfx_px(f, x2, y2 - k, g);
        }
    }
}

void ui_groove(Frame *f, int x0, int y0, int x1, int y1, int ramp, float fill) {
    int n = abs(x1 - x0) > abs(y1 - y0) ? abs(x1 - x0) : abs(y1 - y0);
    if (n < 1) return;
    int top = ramp == R_GOLD ? 12 : 15;
    for (int i = 0; i <= n; i++) {
        int x = x0 + (x1 - x0) * i / n, y = y0 + (y1 - y0) * i / n;
        int lit = (float)i / n <= fill;
        gfx_px(f, x, y, lit ? (uint8_t)(ramp + (11 > top ? top : 11)) : UI_W(0));
        gfx_px(f, x + 1, y, lit ? (uint8_t)(ramp + top) : UI_W(1));
        gfx_px(f, x + 1, y + 1, UI_M(11));
        gfx_px(f, x + 2, y + 1, UI_M(10));
    }
}

/* ---- the Alignment's stone ---- */

static const int TREE_X[10] = { 90, 46, 134, 90, 46, 46, 46, 134, 134, 134 };
static const int TREE_Y[10] = { 56, 78, 78, 100, 122, 143, 164, 122, 143, 164 };
static const int TREE_R[10] = { 9, 9, 9, 10, 9, 10, 11, 9, 10, 11 };
static const int TREE_UP[10] = { -1, 0, 0, 1, 3, 4, 5, 3, 7, 8 };    /* the node each hangs from (the ability from both) */

static void seal(Frame *f, int x, int y, int e, int rank, int ramp, int chosen, int cls, float t) {
    int cx = x + 7, cy = y + 7;
    for (int j = -7; j <= 7; j++) for (int i = -7; i <= 7; i++) {
        int d2 = i * i + j * j;
        if (d2 > 50) continue;
        uint8_t c;
        if (d2 > 36) c = (i + j < 0) == !chosen ? UI_M(chosen ? 12 : 1) : UI_M(chosen ? 3 : 11);
        else c = chosen ? UI_M(9 + ((i * 3 + j * 5) & 1)) : UI_W(2 + ((i + j) & 1));
        gfx_px(f, cx + i, cy + j, c);
    }
    if (cls) for (int i = -6; i <= 6; i++) { gfx_px(f, cx + i, cy + 9, R_GOLD + 11); }
    extern const char *const ELEM_GLYPH[6][7];
    for (int r = 0; r < 7; r++) for (int c = 0; c < 7; c++) {
        char ch = ELEM_GLYPH[e][r][c];
        if (ch != '#' && ch != 'o') continue;
        uint8_t col = rank ? (uint8_t)(ramp + (ch == 'o' ? (ramp == R_GOLD ? 12 : 15) : 11)) : UI_M(ch == 'o' ? 9 : 7);   /* a line not open: its mark cut pale */
        gfx_px(f, cx - 3 + c, cy - 3 + r, col);
    }
    char b[8];
    if (rank) snprintf(b, sizeof b, "%d", rank); else snprintf(b, sizeof b, "-");
    ui_text8(f, x + 17, y + 4, b, FACE_INSC, rank ? ramp + (ramp == R_GOLD ? 12 : 14) : UI_M(3), rank ? TX_INLAID : TX_CUT);
    (void)t;
}

void ui_tree(Frame *f, const UiTree *tr) {
    float t = tr->t;
    ui_slab(f, 4, 4, 312, 192, SLAB_LEDGER, ORN_CANDLE, t);
    for (int k = 0; k < 6; k++) seal(f, 12 + k * 34, 9, k, tr->ranks[k], tr->ramps[k], k == tr->sel_el, k == tr->cls, t);
    char b[48];
    snprintf(b, sizeof b, "%d POINT%s", tr->points, tr->points == 1 ? "" : "S");
    int breathe = tr->points > 0 && ((int)(t * 2.0f) & 1);
    ui_text8(f, 296 - 8 * (int)strlen(b), 13, b, FACE_INSC, breathe ? R_GOLD + 12 : R_GOLD + 9, TX_INLAID);
    ui_well(f, 9, 28, 163, 152);                                   /* the tree */
    ui_well(f, 176, 28, 135, 152);                                 /* the pane */
    if (tr->cls >= 0) {
        extern const char *const CLASS_MARK[6][11];
        for (int r = 0; r < 11; r++) for (int c = 0; c < 11; c++) {
            char ch = CLASS_MARK[tr->cls][r][c];
            if (ch == '#' || ch == 'o') gfx_px(f, 13 + c, 32 + r, (uint8_t)(tr->ramps[tr->cls] + (ch == 'o' ? 14 : 10)));
        }
    }
    int tw = 8 * (int)strlen(tr->title), tx = 98 - tw / 2;
    ui_text8(f, tx, 34, tr->title, FACE_INSC, tr->ramp + (tr->ramp == R_GOLD ? 12 : 13), TX_INLAID);
    for (int m = 1; m < 10; m++) {                                 /* the grooves first, the sockets over them */
        int u = TREE_UP[m];
        float fill = tr->n[m].grow > 0.0f ? tr->n[m].grow : tr->n[m].state == SK_HELD ? 1.0f : tr->n[m].state == SK_PART ? 0.5f : 0.0f;
        if (m == 3) {
            ui_groove(f, TREE_X[1] + 6, TREE_Y[1] + 6, TREE_X[3] - 7, TREE_Y[3] - 6, tr->ramp, fill);
            ui_groove(f, TREE_X[2] - 7, TREE_Y[2] + 6, TREE_X[3] + 6, TREE_Y[3] - 6, tr->ramp, fill);
            continue;
        }
        int dx = TREE_X[m] - TREE_X[u];
        ui_groove(f, TREE_X[u] + (dx > 0 ? 6 : dx < 0 ? -7 : 0), TREE_Y[u] + TREE_R[u] - (dx ? 2 : 0),
                  TREE_X[m] + (dx > 0 ? -7 : dx < 0 ? 6 : 0), TREE_Y[m] - TREE_R[m] + (dx ? 2 : 0), tr->ramp, fill);
    }
    for (int m = 0; m < 10; m++) {
        const UiNode *nd = &tr->n[m];
        int icon = UI_NODE_ICON[tr->elem][m];
        ui_socket(f, TREE_X[m], TREE_Y[m], TREE_R[m], icon, tr->ramp, nd->state, m == tr->sel, m == 6 || m == 9, t);
        if (nd->steps && nd->cost > 1 && nd->state != SK_HELD) {      /* its points as pips under it */
            int px = TREE_X[m] - (nd->cost * 4 - 2) / 2, py = TREE_Y[m] + TREE_R[m] + 2;
            for (int k = 0; k < nd->cost; k++) {
                uint8_t c = k < nd->got ? (uint8_t)(tr->ramp + 13) : UI_W(1);
                gfx_rect(f, px + k * 4, py, 2, 2, c);
                gfx_px(f, px + k * 4 + 2, py + 1, UI_M(10));
            }
        }
    }
    /* the pane: the chosen node */
    const UiNode *nd = &tr->n[tr->sel < 0 || tr->sel > 9 ? 0 : tr->sel];
    int m = tr->sel;
    if (nd->state == SK_HELD) snprintf(b, sizeof b, "YOURS");
    else if (nd->cost > 1 && nd->got > 0) snprintf(b, sizeof b, "%d OF %d POINTS", nd->got, nd->cost);
    else snprintf(b, sizeof b, "%d POINT%s", nd->cost, nd->cost == 1 ? "" : "S");
    UiPane pn = { UI_NODE_ICON[tr->elem][m], 0, tr->ramp, nd->state, 0, nd->name, nd->kind, b, nd->state == SK_HELD ? R_GOLD + 12 : R_GOLD + 10,
                  nd->does, 0, 0, nd->note, tr->warn, 0 };
    ui_pane(f, 176, 28, 135, 152, &pn, t);
    if (tr->keys) ui_keys(f, 160 - ui_keys_width(tr->keys) / 2, 183, tr->keys, UI_M(13));
}

/* ---- a screen and its pane ---- */

void ui_screen(Frame *f, const char *title, int title_col, const char *right, int right_col, int split, float t) {
    ui_slab(f, 4, 4, 312, 192, SLAB_LEDGER, ORN_NONE, t);
    if (title) ui_text16(f, 12, 8, title, FACE_INSC, title_col, TX_INLAID, 1);
    if (right) ui_text8(f, 308 - 8 * (int)strlen(right), 12, right, FACE_INSC, right_col, TX_INLAID);
    if (split > 0) { ui_well(f, 9, 28, split - 13, 152); ui_well(f, split, 28, 311 - split, 152); }
    else ui_well(f, 9, 28, 302, 152);
}

static int longest_word(const char *s) {
    int best = 0, run = 0;
    for (; *s; s++) { run = *s == ' ' ? 0 : run + 1; if (run > best) best = run; }
    return best;
}

void ui_pane(Frame *f, int x, int y, int w, int h, const UiPane *p, float t) {
    int cols = (w - 7) / 8, ncols = (w - 55) / 8, lx = x + 6;
    int col = p->ramp + (p->ramp == R_GOLD ? 12 : 14);
    socket_ex(f, x + 23, y + 25, 19, p->icon, p->mark, p->ramp, p->state, 0, p->ult, t);
    int under = longest_word(p->name) > ncols, dy = 0;               /* a long word: the name goes under the socket, full width */
    if (!under) {
        int nl = ui_wrap_count(p->name, ncols);
        ui_wrap(f, x + 46, nl > 1 ? y + 8 : y + 12, p->name, ncols, 9, 2, FACE_INSC, col, TX_INLAID);
        if (nl > 2) ui_overflow("pane name", p->name, nl - 2);
        if (p->kind) ui_text8(f, x + 46, y + 28, p->kind, FACE_BODY, UI_M(10), TX_PLAIN);
    } else {
        if (p->kind) ui_text8(f, x + 46, y + 20, p->kind, FACE_BODY, UI_M(10), TX_PLAIN);
        int nl = ui_wrap(f, lx, y + 47, p->name, cols, 9, 2, FACE_INSC, col, TX_INLAID);
        dy = nl * 9 + 1;
    }
    if (p->cost) ui_text8(f, lx, y + 46 + dy, p->cost, FACE_INSC, p->cost_col ? p->cost_col : R_GOLD + 10, TX_INLAID);
    ui_rule(f, lx, x + w - 7, y + 57 + dy, 0);
    int top = y + 62 + dy, bottom = y + h - 4;
    if (p->warn) {                                                    /* a warning stands in the description's place */
        int lines = (bottom - top) / 9, need = ui_wrap_count(p->warn, cols);
        ui_wrap(f, lx, top, p->warn, cols, 9, lines, FACE_BODY, p->warn_col ? p->warn_col : R_GOLD + 12, TX_INLAID);
        if (need > lines) ui_overflow("pane warning", p->warn, need - lines);
        return;
    }
    int fl = p->note ? ui_wrap_count(p->note, cols) : 0, fy = bottom - fl * 9;
    int el = p->extra ? ui_wrap_count(p->extra, cols) : 0, ey = el ? fy - (fl ? 3 : 0) - el * 9 : fy;
    int dl = (ey - 3 - top) / 9, need = p->does ? ui_wrap_count(p->does, cols) : 0;
    if (p->does) ui_wrap(f, lx, top, p->does, cols, 9, dl, FACE_BODY, UI_M(14), TX_INLAID);
    if (need > dl) ui_overflow("pane", p->name, need - dl);
    if (p->extra) ui_wrap(f, lx, ey, p->extra, cols, 9, el, FACE_BODY, p->extra_col ? p->extra_col : UI_M(12), TX_INLAID);
    if (p->note) ui_wrap(f, lx, fy, p->note, cols, 9, fl, FACE_BODY, UI_M(11), TX_INLAID);
}

void ui_pane_wide(Frame *f, int x, int y, int w, int h, const UiPane *p) {
    int cols = (w - 12) / 8, lx = x + 6;
    int col = p->ramp + (p->ramp == R_GOLD ? 12 : 14);
    if (p->mark) ui_mark(f, lx, y + 5, p->mark, p->ramp, p->state == SK_HELD || p->state == SK_PART ? IM_INLAID : IM_CUT, 1);
    int nx = p->mark ? lx + 16 : lx;
    if (p->name) ui_text8(f, nx, y + 5, p->name, FACE_INSC, col, TX_INLAID);
    if (p->cost) ui_text8(f, x + w - 6 - 8 * (int)strlen(p->cost), y + 5, p->cost, FACE_INSC, p->cost_col ? p->cost_col : R_GOLD + 10, TX_INLAID);
    int nlen = p->name ? (int)strlen(p->name) : 0, klen = p->kind ? (int)strlen(p->kind) : 0, clen = p->cost ? (int)strlen(p->cost) : 0;
    int one = !p->kind || nx + 8 * (nlen + 1 + klen + 1 + clen) <= x + w - 6, hy = one ? 0 : 10;   /* the kind on the name's line if it fits */
    if (p->kind) ui_text8(f, one ? nx + 8 * (nlen + 1) : nx, y + 5 + hy, p->kind, FACE_BODY, UI_M(10), TX_PLAIN);
    ui_rule(f, lx, x + w - 7, y + 15 + hy, 0);
    int top = y + 20 + hy, bottom = y + h - 4;
    if (p->warn) {
        int lines = (bottom - top) / 9, need = ui_wrap_count(p->warn, cols);
        ui_wrap(f, lx, top, p->warn, cols, 9, lines, FACE_BODY, p->warn_col ? p->warn_col : R_GOLD + 12, TX_INLAID);
        if (need > lines) ui_overflow("pane warning", p->warn, need - lines);
        return;
    }
    int fl = p->note ? ui_wrap_count(p->note, cols) : 0, fy = bottom - fl * 9;
    int dl = (fy - (fl ? 3 : 0) - top) / 9, need = p->does ? ui_wrap_count(p->does, cols) : 0;
    if (p->does) ui_wrap(f, lx, top, p->does, cols, 9, dl, FACE_BODY, UI_M(14), TX_INLAID);
    if (need > dl) ui_overflow("pane", p->name ? p->name : "?", need - dl);
    if (p->note) ui_wrap(f, lx, fy, p->note, cols, 9, fl, FACE_BODY, UI_M(11), TX_INLAID);
}

