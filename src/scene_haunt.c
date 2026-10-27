/* HAUNT.EXE - a possessed PC in VGA 80x25 text mode (720x400, 9x16 cells).
 *
 * A tiny script runner drives a fake DOS session: BIOS POST, a prompt, and a
 * ghost that types back.  Between the commands, full-screen character-art set
 * pieces materialise and dissolve.  Everything is CP437 text; the only
 * non-standard thing is that the DAC palette is reprogrammed for effects
 * (ghost fade-ins, candle flicker, lightning, red alarm). */
#include "core.h"
#include "gfx.h"
#include "snd.h"
#include "haunt_art.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define COLS TCOLS
#define ROWS TROWS
#define CELL_W 9
#define CELL_H 16
#define QMAX 3072

/* inline colour escapes: "\x01" + hex digit = foreground, "\x02" = background, "\x03" resets;
 * "\x04" + any byte prints that byte as it is (for the CP437 faces at 1 and 2) */
#define FG(c) "\x01" #c
#define BG(c) "\x02" #c
#define RESET "\x03"

typedef struct { uint8_t ch, fg, bg, ghost; float age; } Cell;

enum {
    OP_WAIT, OP_PRINT, OP_TYPE, OP_GTYPE, OP_ENTER, OP_SFX, OP_CLS, OP_MOOD, OP_CURSOR,
    OP_LOCATE, OP_TOP, OP_ART, OP_ARTEND, OP_GLITCH, OP_BLACK, OP_CRT, OP_GFADE,
    OP_ALARM, OP_GARBAGE, OP_TALK, OP_PUTS, OP_SCROLL
};

typedef struct { uint8_t op; int a, b, c; float f; char s[192]; } Op;

enum { A_BOOT, A_INTRO, A_CHKDSK, A_FORMAT, A_MEM, A_DEL, A_IDLE, A_ABORT, A_SOULS, A_README,
       A_ART0, A_ART1, A_ART2, A_ART3, A_ART4, A_ART5, A_ART6, A_CRASH,
       A_EDIT, A_DEFRAG, A_BBS, A_VIRUS, A_MSD, A_SETUP, A_QBASIC, A_NC };

static struct {
    uint32_t rs;
    double t, blink;
    Cell cell[ROWS][COLS];
    int cx, cy, top;
    uint8_t afg, abg;
    int cursor_vis;
    Op q[QMAX];
    int qh, qt;
    double vt;
    int plan[40], plan_n, plan_i;
    int haunt_count;
    /* effects */
    float glitch, alarm, talk, gfade, gfade_len;
    int gfading;
    int black;
    int crt_mode; double crt_t;
    int art_kind, art_state;        /* state: 0 none, 1 revealing, 2 dissolving */
    double art_t; float art_reveal;
    double boom_t;
    uint32_t grs;                   /* glitch rng, independent of the script */
} S;

static int rnd(int n) { return (int)(rng_next(&S.rs) % (uint32_t)n); }
static float rndf(void) { return rng_f(&S.rs); }

/* ------------------------------------------------------------------ */
/* Text screen                                                         */
/* ------------------------------------------------------------------ */

static void cell_blank(Cell *c) { c->ch = ' '; c->fg = 7; c->bg = 0; c->ghost = 0; c->age = 0; }

static void cls(void) {
    for (int y = 0; y < ROWS; y++) for (int x = 0; x < COLS; x++) cell_blank(&S.cell[y][x]);
    S.cx = S.cy = 0; S.afg = 7; S.abg = 0; S.top = 0;
}

static void scroll_up(void) {
    for (int y = S.top; y < ROWS - 1; y++) memcpy(S.cell[y], S.cell[y + 1], sizeof S.cell[y]);
    for (int x = 0; x < COLS; x++) cell_blank(&S.cell[ROWS - 1][x]);
}

static void tprint_char(uint8_t ch, int ghost) {
    switch (ch) {
    case '\n': S.cx = 0; S.cy++; break;
    case '\r': S.cx = 0; break;
    case '\b':
        if (S.cx > 0) { S.cx--; cell_blank(&S.cell[S.cy][S.cx]); S.cell[S.cy][S.cx].fg = S.afg; S.cell[S.cy][S.cx].bg = S.abg; }
        return;
    default: {
        Cell *c = &S.cell[S.cy][S.cx];
        c->ch = ch; c->fg = S.afg; c->bg = S.abg; c->ghost = (uint8_t)ghost; c->age = 0;
        if (++S.cx >= COLS) { S.cx = 0; S.cy++; }
    }
    }
    if (S.cy >= ROWS) { scroll_up(); S.cy = ROWS - 1; }
}

static int hexval(char c) { return c >= '0' && c <= '9' ? c - '0' : (c >= 'A' && c <= 'F' ? c - 'A' + 10 : 7); }

/* prints one logical unit (an escape or a character); returns bytes consumed */
static int tprint_unit(const char *s, int ghost) {
    if (s[0] == 1) { S.afg = (uint8_t)hexval(s[1]); return 2; }
    if (s[0] == 2) { S.abg = (uint8_t)hexval(s[1]); return 2; }
    if (s[0] == 3) { S.afg = 7; S.abg = 0; return 1; }
    if (s[0] == 4 && s[1]) { tprint_char((uint8_t)s[1], ghost); return 2; }
    tprint_char((uint8_t)s[0], ghost);
    return 1;
}

/* full-screen programs write straight into the cells: no cursor, no wrapping, no scrolling */
static void puts_at(int x, int y, const char *s) {
    uint8_t fg = 7, bg = 0;
    if ((unsigned)y >= ROWS) return;
    while (*s && x < COLS) {
        uint8_t ch = (uint8_t)*s;
        if (ch == 1 && s[1]) { fg = (uint8_t)hexval(s[1]); s += 2; continue; }
        if (ch == 2 && s[1]) { bg = (uint8_t)hexval(s[1]); s += 2; continue; }
        if (ch == 3) { fg = 7; bg = 0; s++; continue; }
        if (ch == 4 && s[1]) { ch = (uint8_t)s[1]; s++; }
        s++;
        if (x >= 0) { Cell *c = &S.cell[y][x]; c->ch = ch; c->fg = fg; c->bg = bg; c->ghost = 0; c->age = 0; }
        x++;
    }
}

static void scroll_rows(int y0, int y1, int bg) {
    for (int y = y0; y < y1; y++) memcpy(S.cell[y], S.cell[y + 1], sizeof S.cell[y]);
    for (int x = 0; x < COLS; x++) { cell_blank(&S.cell[y1][x]); S.cell[y1][x].bg = (uint8_t)bg; }
}

static void tprint(const char *s, int ghost) { while (*s) s += tprint_unit(s, ghost); }

/* ------------------------------------------------------------------ */
/* Script queue                                                        */
/* ------------------------------------------------------------------ */

static int qempty(void) { return S.qh == S.qt; }
static void qx(uint8_t op, int a, int b, int c, float f, const char *s) {
    int nt = (S.qt + 1) % QMAX;
    if (nt == S.qh) return;
    Op *o = &S.q[S.qt];
    o->op = op; o->a = a; o->b = b; o->c = c; o->f = f;
    strncpy(o->s, s ? s : "", sizeof o->s - 1); o->s[sizeof o->s - 1] = 0;
    S.qt = nt;
}
static void q(uint8_t op, int a, int b, float f, const char *s) { qx(op, a, b, 0, f, s); }

static void q_wait(float t) { q(OP_WAIT, 0, 0, t, ""); }
static void q_print(const char *s) { q(OP_PRINT, 0, 0, 0, s); }
static void q_type(const char *s, float cps) { q(OP_TYPE, 0, 0, cps, s); }
static void q_gtype(const char *s, float cps) { q(OP_GTYPE, 0, 0, cps, s); }
static void q_gplain(const char *s, float cps) { qx(OP_GTYPE, 0, 0, 1, cps, s); }   /* the ghost at the keys of a program: ordinary text */
static void q_sfx(int id) { q(OP_SFX, id, 0, 0, ""); }
static void q_mood(float m) { q(OP_MOOD, 0, 0, m, ""); }
static void q_cls(void) { q(OP_CLS, 0, 0, 0, ""); }
static void q_enter(void) { q(OP_ENTER, 0, 0, 0, ""); }
static void q_at(int x, int y) { q(OP_LOCATE, x, y, 0, ""); }
/* a line of output: adds the newline unless the text already ends with one
 * (colour escapes at the end don't count) */
static void q_line(const char *s, float pause) {
    int last = 0;
    for (const char *p = s; *p; ) {
        if (*p == 1 || *p == 2) { p += 2; continue; }
        if (*p == 3) { p++; continue; }
        last = *p++;
    }
    q_print(s);
    if (last != '\n') q_print("\n");
    q_wait(pause);
}

static void q_prompt(void) { q_print(RESET "C:\\>"); }
static void user_cmd(const char *cmd) {
    q_prompt();
    q_wait(0.5f + 0.9f * rndf());
    q_type(cmd, 9.0f + 3.0f * rndf());
    q_wait(0.25f);
    q_enter();
}
static void ghost_at_prompt(const char *msg, float cps) {
    q_prompt();
    q_wait(1.1f + rndf());
    q_gtype(msg, cps);
    q_wait(0.9f);
}

/* ------------------------------------------------------------------ */
/* Acts                                                                */
/* ------------------------------------------------------------------ */

static void act_boot(void) {
    char buf[104];
    q_mood(0.0f);
    q(OP_BLACK, 1, 0, 0, ""); q_cls(); q_wait(0.7f);
    q(OP_BLACK, 0, 0, 0, ""); q(OP_CRT, 2, 0, 0, ""); q_wait(0.45f);
    q(OP_CURSOR, 0, 0, 0, "");
    q_line(FG(F) "CRYPTEK 486 BIOS " FG(7) "v13.0.31   (C) 1994 Phantom Systems Inc.\n", 0.15f);
    q_line("BIOS date: 10/31/94 00:00:00\n\n", 0.25f);
    q_line("CPU      : 80486DX2-66  (66 MHz)\n", 0.12f);
    q_line("Cache    : 256K, haunted\n", 0.12f);
    q_line("FPU      : Present (and watching)\n\n", 0.35f);
    for (int k = 0; k <= 40; k++) {
        snprintf(buf, sizeof buf, "\rMemory Test : %5dK", 16384 * k / 40);
        q_print(buf);
        q_wait(0.028f);
    }
    q_print("\rMemory Test : " FG(F) "16384K OK" FG(7) "\n\n");
    q_sfx(SFX_BEEP_POST);
    q_wait(0.5f);
    q_print("Detecting IDE Primary Master  ... ");
    q_sfx(SFX_HDD); q_wait(0.95f);
    q_line(FG(F) "SEANCE-660  666MB\n" FG(7), 0.2f);
    q_print("Detecting IDE Primary Slave   ... ");
    q_wait(0.55f);
    q_line("None (it left)\n", 0.2f);
    q_print("Detecting Floppy Drive A:     ... ");
    q_sfx(SFX_FLOPPY); q_wait(1.3f);
    q_line(FG(F) "1.44M, 3.5in.\n\n" FG(7), 0.3f);
    q_line("Press " FG(F) "DEL" FG(7) " to enter SETUP, or don't. We'll know.\n", 1.8f);
    q_cls();
    q(OP_CURSOR, 1, 0, 0, "");
    q_wait(0.5f);
    q_line("Starting DOS...\n\n", 1.4f);
}

static void act_intro(void) {
    q_mood(0.08f);
    user_cmd("ver");
    q_line("\nDOS Version 6.66\n\n", 0.5f);
    user_cmd("dir");
    q_sfx(SFX_HDD);
    q_wait(0.5f);
    q_line("\n Volume in drive C is HAUNTED", 0.07f);
    q_line(" Volume Serial Number is 0666-1031", 0.07f);
    q_line(" Directory of C:\\\n", 0.07f);
    q_line("COMMAND  COM        54,619 10-31-94  12:00a", 0.07f);
    q_line("CONFIG   SYS           666 10-31-94  12:00a", 0.07f);
    q_line("AUTOEXEC BAT           313 10-31-94  12:00a", 0.07f);
    q_line("SEANCE   BAT         1,013 10-31-94  12:00a", 0.07f);
    q_line("BOOGEY   COM        13,013 10-31-94  12:00a", 0.07f);
    q_line("WITCH    DOC        31,031 10-31-94  12:00a", 0.07f);
    q_line("GHOST    EXE        66,613 10-31-94  12:00a", 0.07f);
    q_line("SOULS    DLL     1,313,013 10-31-94  12:00a", 0.07f);
    q_line("README   NOW             0 10-31-94  12:00a", 0.07f);
    q_line("         9 file(s)  1,480,281 bytes", 0.07f);
    q_line("         0 dir(s)          13 bytes free\n", 0.9f);
}

static void act_chkdsk(void) {
    q_mood(0.30f);
    user_cmd("chkdsk");
    q_sfx(SFX_HDD); q_wait(0.9f);
    q_line("\nVolume HAUNTED created 10-31-1666 12:00a", 0.1f);
    q_line("Volume Serial Number is 0666-1031\n", 0.35f);
    q_line("Errors found.  F parameter not specified.", 0.1f);
    q_line("Corrections will not be written to disk.\n", 0.4f);
    q_line("   3 lost allocation units found in 1 chains.", 0.1f);
    q_print("   Convert lost chains to files (Y/N)? ");
    q_wait(1.8f);
    q_gtype("n", 4.0f);
    q_wait(0.9f);
    q_print("\n\n");
    q_gtype("they were never lost.\nthey live here now.", 7.0f);
    q_wait(2.2f);
    q(OP_GFADE, 0, 0, 1.5f, "");
    q_wait(1.8f);
    q_print("\n");
}

static void act_format(void) {
    q_mood(0.35f);
    user_cmd("format c:");
    q_wait(0.6f);
    q_line("\n" FG(C) "WARNING, ALL DATA ON NON-REMOVABLE DISK", 0.05f);
    q_line("DRIVE C: WILL BE LOST!" FG(7), 0.1f);
    q_print("Proceed with Format (Y/N)?");
    q(OP_CURSOR, 1, 0, 0, "");
    q_wait(2.4f);
    q_gtype("n", 3.0f);
    q_wait(0.7f);
    q_print("\n\n");
    q_gtype("are you crazy?\ni live here.", 6.5f);
    q_wait(2.0f);
    q(OP_GFADE, 0, 0, 1.5f, "");
    q_wait(1.7f);
    q_print("\n");
}

static void act_mem(void) {
    q_mood(0.40f);
    user_cmd("mem");
    q_wait(0.4f);
    q_line("\nMemory Type        Total    Used    Free", 0.06f);
    q_line("----------------  ------- ------- -------", 0.06f);
    q_line("Conventional         640K    627K     13K", 0.06f);
    q_line("Upper                  0K      0K      0K", 0.06f);
    q_line("Extended (XMS)     3,072K  3,072K      0K", 0.06f);
    q_line("----------------  ------- ------- -------", 0.06f);
    q_line("Total memory       3,712K  3,699K     13K\n", 0.06f);
    q_line("Largest executable program size       13K\n", 0.9f);
    ghost_at_prompt("i am using the rest", 7.0f);
    q_wait(1.6f);
    q(OP_GFADE, 0, 0, 1.4f, "");
    q_wait(1.6f);
    q_print("\n");
}

static void act_del(void) {
    q_mood(0.45f);
    user_cmd("del *.*");
    q_line("\nAll files in directory will be deleted!", 0.1f);
    q_print("Are you sure (Y/N)?");
    q_wait(1.6f);
    q_gtype("n", 3.0f);
    q_wait(0.7f);
    q_print("\n");
    q_sfx(SFX_BEEP_ERR);
    q_line(FG(C) "Access denied" FG(7) " (by the dead)\n", 1.2f);
}

static const char *const IDLE_LINES[] = {
    "hello?", "is anyone there", "i can see you through the screen",
    "you left the light on", "do not turn off the monitor", "it is so dark in here",
    "your fans are loud. i love it.", "turn around", "we have been waiting for you",
    "i like it here. it hums.", "let me out. just kidding.", "have you tried turning me off and on?",
};

static void act_idle(void) {
    q_mood(0.50f);
    q_prompt();
    q_wait(2.2f);
    q_sfx(SFX_WHISPER);
    q_wait(0.5f);
    q_gtype(IDLE_LINES[rnd((int)(sizeof IDLE_LINES / sizeof IDLE_LINES[0]))], 6.0f);
    q_wait(2.6f);
    q(OP_GFADE, 0, 0, 1.6f, "");
    q_wait(1.9f);
    q_print("\n");
    q_prompt();
    q_wait(1.2f);
    q_gtype("you can hear me, can't you", 6.5f);
    q_wait(2.4f);
    q(OP_GFADE, 0, 0, 1.6f, "");
    q_wait(1.9f);
    q_print("\n");
}

static void act_abort(void) {
    q_mood(0.55f);
    user_cmd("copy a:*.* c:\\");
    q_sfx(SFX_FLOPPY); q_wait(1.5f);
    q_line("\nNot ready reading drive A", 0.2f);
    q_print("Abort, Retry, Fail?");
    q_sfx(SFX_BEEP_ERR);
    q_wait(1.8f);
    q_print("\b\b\b\b\b");
    q_wait(0.3f);
    q_gtype("Haunt?", 5.0f);
    q_wait(2.4f);
    q_print("\n\n");
}

static void act_souls(void) {
    char buf[104];
    q_mood(0.60f);
    user_cmd("boogey");
    q_line("\nBOOGEY 1.3  Copyright (C) Phantom Systems", 0.2f);
    q_line("Loading SOULS.DLL\n", 0.4f);
    for (int p = 0; p <= 13; p++) {
        int filled = p * 30 / 100;
        char bar[40]; int n = 0;
        for (int i = 0; i < 30; i++) bar[n++] = (char)(i < filled ? 0xDB : 0xB0);
        bar[n] = 0;
        snprintf(buf, sizeof buf, "\r" FG(A) "[%s]" FG(7) " %3d%%", bar, p);
        q_print(buf);
        q_sfx(SFX_TICK);
        q_wait(0.09f);
    }
    q_wait(2.0f);
    q_print("\n\n");
    q_sfx(SFX_STATIC);
    q_line(FG(C) "Error: soul #13 not found." FG(7), 0.2f);
    q_print("Retry? (Y/N) ");
    q_wait(1.6f);
    q_gtype("y", 3.0f);
    q_wait(0.5f);
    q_print("\n");
    q_sfx(SFX_CREAK);
    q_wait(1.8f);
}

static void act_readme(void) {
    q_mood(0.65f);
    user_cmd("type readme.now");
    q_wait(0.6f);
    q_print("\n\n");
    q_gtype("THEY ARE ALL STILL HERE.", 5.5f);
    q_sfx(SFX_WHISPER);
    q_wait(2.2f);
    q_gtype("\nTHEY ARE ALL STILL HERE.", 5.5f);
    q_wait(1.6f);
    q_gtype("\nTHEY ARE ALL STILL HERE.", 5.5f);
    q_wait(2.4f);
    q(OP_GFADE, 0, 0, 1.8f, "");
    q_wait(2.0f);
    q_print("\n\n");
}

/* ------------------------------------------------------------------ */
/* Full-screen programs.  A screen is composed in B and sent a row at  */
/* a time; later changes are made in B and only what changed is sent.  */
/* ------------------------------------------------------------------ */

typedef struct { uint8_t ch, fg, bg; } BCell;
static BCell B[ROWS][COLS];

static void b_fill(int x, int y, int w, int h, int ch, int fg, int bg) {
    for (int r = y; r < y + h; r++)
        for (int c = x; c < x + w; c++)
            if ((unsigned)r < ROWS && (unsigned)c < COLS) { B[r][c].ch = (uint8_t)ch; B[r][c].fg = (uint8_t)fg; B[r][c].bg = (uint8_t)bg; }
}
/* recolour, keeping the characters (highlight bars, shadows); -1 leaves that colour alone */
static void b_attr(int x, int y, int w, int h, int fg, int bg) {
    for (int r = y; r < y + h; r++)
        for (int c = x; c < x + w; c++)
            if ((unsigned)r < ROWS && (unsigned)c < COLS) { if (fg >= 0) B[r][c].fg = (uint8_t)fg; if (bg >= 0) B[r][c].bg = (uint8_t)bg; }
}
static void b_text(int x, int y, int fg, int bg, const char *s) {
    for (; *s && x < COLS; s++, x++)
        if (x >= 0 && (unsigned)y < ROWS) { B[y][x].ch = (uint8_t)*s; B[y][x].fg = (uint8_t)fg; B[y][x].bg = (uint8_t)bg; }
}
static void b_center(int y, int fg, int bg, const char *s) { b_text((COLS - (int)strlen(s)) / 2, y, fg, bg, s); }
static void b_box(int x, int y, int w, int h, int fg, int bg, int dbl) {
    static const uint8_t G[2][6] = { { 0xDA, 0xBF, 0xC0, 0xD9, 0xC4, 0xB3 }, { 0xC9, 0xBB, 0xC8, 0xBC, 0xCD, 0xBA } };
    const uint8_t *g = G[dbl];
    b_fill(x, y, w, h, ' ', fg, bg);
    b_fill(x + 1, y, w - 2, 1, g[4], fg, bg); b_fill(x + 1, y + h - 1, w - 2, 1, g[4], fg, bg);
    b_fill(x, y + 1, 1, h - 2, g[5], fg, bg); b_fill(x + w - 1, y + 1, 1, h - 2, g[5], fg, bg);
    b_fill(x, y, 1, 1, g[0], fg, bg); b_fill(x + w - 1, y, 1, 1, g[1], fg, bg);
    b_fill(x, y + h - 1, 1, 1, g[2], fg, bg); b_fill(x + w - 1, y + h - 1, 1, 1, g[3], fg, bg);
}
static void b_title(int x, int y, int w, int fg, int bg, const char *t) {
    char buf[64];
    snprintf(buf, sizeof buf, " %s ", t);
    b_text(x + (w - (int)strlen(buf)) / 2, y, fg, bg, buf);
}
static void b_shadow(int x, int y, int w, int h) { b_attr(x + w, y + 1, 2, h, 8, 0); b_attr(x + 2, y + h, w, 1, 8, 0); }

/* send part of a row of B to the screen */
static void q_span(int x0, int y, int w) {
    static const char HEXD[] = "0123456789ABCDEF";
    char buf[192];
    int n = 0, fg = -1, bg = -1, start = x0;
    for (int x = x0; x < x0 + w && x < COLS; x++) {
        const BCell *c = &B[y][x];
        if (n > 170) { buf[n] = 0; q(OP_PUTS, start, y, 0, buf); start = x; n = 0; fg = bg = -1; }
        if (c->fg != fg) { buf[n++] = 1; buf[n++] = HEXD[c->fg & 15]; fg = c->fg; }
        if (c->bg != bg) { buf[n++] = 2; buf[n++] = HEXD[c->bg & 15]; bg = c->bg; }
        uint8_t ch = c->ch ? c->ch : ' ';
        if (ch <= 4) buf[n++] = 4;
        buf[n++] = (char)ch;
    }
    buf[n] = 0;
    q(OP_PUTS, start, y, 0, buf);
}
static void q_rows(int y0, int y1) { for (int y = y0; y <= y1; y++) q_span(0, y, COLS); }

/* a string the ghost types inside a program, in that program's colours */
static void q_gplain_c(int fg, int bg, const char *s, float cps) {
    char buf[192];
    snprintf(buf, sizeof buf, "\x01%c\x02%c%s", "0123456789ABCDEF"[fg & 15], "0123456789ABCDEF"[bg & 15], s);
    q_gplain(buf, cps);
}
static void q_type_c(int fg, int bg, const char *s, float cps) {
    char buf[192];
    snprintf(buf, sizeof buf, "\x01%c\x02%c%s", "0123456789ABCDEF"[fg & 15], "0123456789ABCDEF"[bg & 15], s);
    q_type(buf, cps);
}

/* the logged-in Windows user, as DOS would have spelled it: eight characters, upper case */
static void user_name(char *out, int n) {
    const char *u = getenv("USERNAME");
    if (!u || !*u) u = getenv("USER");
    int k = 0;
    for (; u && *u && k < n - 1 && k < 8; u++) {
        char ch = *u;
        if (ch >= 'a' && ch <= 'z') ch = (char)(ch - 32);
        if ((ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_' || ch == '-') out[k++] = ch;
    }
    out[k] = 0;
    if (!k) snprintf(out, (size_t)n, "YOU");
}
static void today(char *out, int n) {
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    if (tm) snprintf(out, (size_t)n, "%02d-%02d-%02d", (tm->tm_mon + 1) % 100, tm->tm_mday % 100, tm->tm_year % 100);
    else snprintf(out, (size_t)n, "10-31-94");
}

/* ------------------------------------------------------------------ */
/* EDIT: the ghost rewrites AUTOEXEC.BAT for a seance                  */
/* ------------------------------------------------------------------ */

static const char *const EDIT_HINT = "DOS Editor  <F1=Help> Press ALT to activate menus";

static void edit_status(int line, int col, const char *msg) {
    char buf[32];
    b_fill(0, 24, COLS, 1, ' ', 0, 3);
    b_text(1, 24, 15, 3, msg);
    snprintf(buf, sizeof buf, "%05d:%03d", line, col);
    b_text(70, 24, 0, 3, buf);
    q_rows(24, 24);
}

static void edit_screen(const char *title) {
    static const char *const MENU[4] = { "File", "Edit", "Search", "Options" };
    b_fill(0, 0, COLS, ROWS, ' ', 7, 1);
    b_fill(0, 0, COLS, 1, ' ', 0, 7);
    for (int i = 0, x = 2; i < 4; x += (int)strlen(MENU[i]) + 2, i++) { b_text(x, 0, 0, 7, MENU[i]); B[0][x].fg = 15; }
    b_text(74, 0, 0, 7, "Help"); B[0][74].fg = 15;
    b_box(0, 1, COLS, 23, 7, 1, 0);
    b_title(0, 1, COLS, 1, 7, title);
    b_fill(79, 2, 1, 20, 0xB0, 0, 7); b_fill(79, 2, 1, 1, 0x18, 0, 7); b_fill(79, 3, 1, 1, 0xDB, 0, 7); b_fill(79, 21, 1, 1, 0x19, 0, 7);
    b_fill(1, 22, 78, 1, 0xB0, 0, 7); b_fill(1, 22, 1, 1, 0x1B, 0, 7); b_fill(2, 22, 1, 1, 0xDB, 0, 7); b_fill(78, 22, 1, 1, 0x1A, 0, 7);
}

static void edit_menu(int sel) {
    static const char *const ITEM[8] = { "New", "Open...", "Save", "Save As...", "-", "Print...", "-", "Exit" };
    b_attr(1, 0, 6, 1, 15, 0);
    b_box(1, 1, 18, 10, 0, 7, 0);
    for (int i = 0; i < 8; i++) {
        int y = 2 + i;
        if (ITEM[i][0] == '-') { b_fill(2, y, 16, 1, 0xC4, 0, 7); B[y][1].ch = 0xC3; B[y][18].ch = 0xB4; continue; }
        b_text(3, y, 0, 7, ITEM[i]);
        B[y][3].fg = 15;
        if (i == sel) b_attr(2, y, 16, 1, 15, 0);
    }
    b_shadow(1, 1, 18, 10);
}

static void act_edit(void) {
    static const char *const FILE0[6] = { "@ECHO OFF", "PROMPT $P$G", "PATH C:\\DOS;C:\\WINDOWS", "SET TEMP=C:\\TEMP",
                                           "LH C:\\DOS\\MOUSE.COM", "LH C:\\DOS\\SMARTDRV.EXE 2048" };
    static const char *const SEANCE[7] = { "REM --- the circle ---", "SET CANDLES=13", "SET CIRCLE=UNBROKEN", "SET MEDIUM=C:\\OUIJA.SYS",
                                           "SET SPIRITS=WILLING", "SET LIGHTS=OFF", "LH C:\\BEYOND\\TABLE.COM /KNOCK:3" };
    static BCell keep[ROWS][COLS];
    q_mood(0.5f);
    user_cmd("edit autoexec.bat");
    q_sfx(SFX_HDD); q_wait(0.8f);
    edit_screen("AUTOEXEC.BAT");
    for (int i = 0; i < 6; i++) b_text(1, 2 + i, 7, 1, FILE0[i]);
    q_rows(0, 23);
    edit_status(1, 1, EDIT_HINT);
    q_at(1, 2);
    q_wait(1.3f);
    for (int i = 1; i <= 6; i++) { q_at(1, 2 + i); edit_status(i + 1, 1, EDIT_HINT); q_wait(0.16f); }   /* the cursor creeps down on its own */
    q_sfx(SFX_WHISPER);
    q_wait(1.0f);
    for (int i = 0; i < 7; i++) {
        q_at(1, 8 + i);
        q_gplain_c(7, 1, SEANCE[i], 18.0f);
        b_text(1, 8 + i, 7, 1, SEANCE[i]);
        edit_status(9 + i, (int)strlen(SEANCE[i]) + 1, EDIT_HINT);
        q_wait(0.25f);
        if (i < 6) { q_at(1, 9 + i); q_wait(0.12f); }
    }
    q_wait(0.7f);
    for (int k = 0; k < 3; k++) { q_sfx(SFX_KNOCK); q_wait(0.6f); }
    q_wait(1.0f);
    q_at(12, 3); edit_status(2, 12, EDIT_HINT);                      /* back up to the prompt, which it changes */
    q_wait(0.9f);
    q_gplain_c(7, 1, "\b\b\b\b", 5.0f);
    q_gplain_c(7, 1, "WE ARE HERE$G", 9.0f);
    b_text(1, 3, 7, 1, "PROMPT WE ARE HERE$G");
    edit_status(2, 21, EDIT_HINT);
    q_wait(1.4f);
    memcpy(keep, B, sizeof B);                                        /* File > Save, then File > Exit */
    q(OP_CURSOR, 0, 0, 0, "");
    for (int pass = 0; pass < 2; pass++) {
        int last = pass ? 7 : 2;
        for (int sel = 0; sel <= last; sel++) {
            if (sel == 4 || sel == 6) continue;
            memcpy(B, keep, sizeof B); edit_menu(sel); q_rows(0, 11);
            q_sfx(SFX_KEY); q_wait(sel ? 0.22f : 0.6f);
        }
        q_wait(0.4f);
        q_sfx(SFX_ENTER);
        memcpy(B, keep, sizeof B); q_rows(0, 11);
        if (!pass) { q_sfx(SFX_HDD); edit_status(2, 21, "Saving..."); q_wait(1.1f); edit_status(2, 21, EDIT_HINT); q_wait(0.9f); }
    }
    q_cls();
    q(OP_CURSOR, 1, 0, 0, "");
    q_wait(0.8f);
    q_prompt();
    q_wait(1.4f);
    q_gtype("autoexec", 5.5f);
    q_wait(0.5f);
    q_print("\n");
    q_line("TABLE.COM 1.3 - the table is ready", 0.6f);
    for (int k = 0; k < 3; k++) { q_sfx(SFX_KNOCK); q_wait(0.6f); }
    q_print("\nWE ARE HERE>");
    q_wait(1.6f);
    q_gtype("knock once for yes.", 6.0f);
    q_wait(1.2f);
    q_sfx(SFX_KNOCK);
    q_wait(2.2f);
    q(OP_GFADE, 0, 0, 1.6f, "");
    q_wait(1.8f);
    q_print("\n\n");
}

/* ------------------------------------------------------------------ */
/* DEFRAG: souls consumed, remains moved to the front of the disk      */
/* ------------------------------------------------------------------ */

#define DF_X 1
#define DF_Y 2
#define DF_W 78
#define DF_H 13
enum { DF_FREE, DF_BONE, DF_SOUL, DF_FIXED, DF_BAD };

static void df_put(int i, int ch, int fg) {
    int x = DF_X + i % DF_W, y = DF_Y + i / DF_W;
    B[y][x].ch = (uint8_t)ch; B[y][x].fg = (uint8_t)fg; B[y][x].bg = 1;
    q_span(x, y, 1);
}
static void df_show(int i, int kind) {
    static const uint8_t CH[5] = { 0xB0, 0xFE, 0x02, 'X', 'B' }, FG[5] = { 3, 15, 14, 7, 12 };
    df_put(i, CH[kind], FG[kind]);
}

static void df_status(int pct, float secs, int souls, const char *line) {
    char buf[64];
    b_fill(2, 18, 36, 4, ' ', 0, 7);
    snprintf(buf, sizeof buf, "Cluster %d,%03d", 1 + pct / 10, (pct * 37) % 1000);
    b_text(3, 18, 0, 7, buf);
    snprintf(buf, sizeof buf, "%3d%%", pct);
    b_text(33, 18, 0, 7, buf);
    int fill = pct * 34 / 100;
    b_fill(3, 19, 34, 1, 0xB0, 8, 7); b_fill(3, 19, fill, 1, 0xDB, 1, 7);
    int s = (int)secs;
    snprintf(buf, sizeof buf, "Elapsed Time: %02d:%02d:%02d", s / 3600, s / 60 % 60, s % 60);
    b_text(3, 20, 0, 7, buf);
    snprintf(buf, sizeof buf, "Souls consumed: %d", souls);
    b_text(3, 21, souls ? 4 : 0, 7, buf);
    q_rows(18, 21);
    if (line) { b_fill(0, 24, COLS, 1, ' ', 0, 3); b_text(1, 24, 0, 3, line); q_rows(24, 24); }
}

static void act_defrag(void) {
    static uint8_t map[DF_W * DF_H];
    const int n = DF_W * DF_H;
    for (int i = 0; i < n; i++) { float r = rndf(); map[i] = r < 0.47f ? DF_BONE : (r < 0.50f ? DF_SOUL : DF_FREE); }
    for (int k = 0; k < 4; k++) map[30 + rnd(n / 2)] = DF_FIXED;
    for (int k = 0; k < 2; k++) map[rnd(n)] = DF_BAD;
    map[2] = DF_SOUL; map[1] = DF_FREE;
    int bones = 0, todo = 0;
    for (int i = 0; i < n; i++) bones += map[i] == DF_BONE;
    for (int i = 0; i < bones; i++) todo += map[i] != DF_BONE;
    q_mood(0.55f);
    user_cmd("defrag c: /f");
    q_wait(0.6f);
    b_fill(0, 0, COLS, ROWS, ' ', 15, 1);
    b_fill(0, 0, COLS, 1, ' ', 0, 7);
    b_text(1, 0, 0, 7, "Optimize  Configure  Help"); B[0][1].fg = 15; B[0][11].fg = 15; B[0][22].fg = 15;
    b_text(66, 0, 0, 7, "DEFRAG 6.66");
    for (int i = 0; i < n; i++) {
        static const uint8_t CH[5] = { 0xB0, 0xFE, 0x02, 'X', 'B' }, FG[5] = { 3, 15, 14, 7, 12 };
        BCell *c = &B[DF_Y + i / DF_W][DF_X + i % DF_W];
        c->ch = CH[map[i]]; c->fg = FG[map[i]]; c->bg = 1;
    }
    b_box(1, 17, 38, 6, 0, 7, 0); b_title(1, 17, 38, 0, 7, "Status"); b_shadow(1, 17, 38, 6);
    b_box(41, 17, 37, 6, 0, 7, 0); b_title(41, 17, 37, 0, 7, "Legend"); b_shadow(41, 17, 37, 6);
    b_text(43, 18, 0, 7, "\xFE - Remains      r - Reading");
    b_text(43, 19, 0, 7, "  - Soul         W - Writing"); B[19][43].ch = 0x02;
    b_text(43, 20, 0, 7, "\xB0 - Unused       B - Bad (cursed)");
    b_text(43, 21, 0, 7, "X - Unmovable (buried)");
    q(OP_CURSOR, 0, 0, 0, "");
    q_rows(0, 24);
    float clock = 0.0f;
    df_status(0, clock, 0, "Optimizing drive C:   Full Optimization");
    q_sfx(SFX_HDD);
    q_wait(2.2f); clock += 2.2f;
    int w = 0, r = n - 1, moves = 0, souls = 0;
    while (w < r) {
        if (map[w] == DF_BONE || map[w] == DF_FIXED || map[w] == DF_BAD) { w++; continue; }
        int at = -1;
        if (map[w] == DF_SOUL) at = w;
        else {
            while (r > w && map[r] != DF_BONE && map[r] != DF_SOUL) r--;
            if (r <= w) break;
            if (map[r] == DF_SOUL) at = r;
        }
        if (at >= 0) {                                                /* a soul under the head: it goes into the machine */
            int slow = souls < 6;
            static const uint8_t F[4] = { 0x02, 0x01, '*', 0xF9 }, FC[4] = { 15, 14, 12, 8 };
            if (slow) q_sfx(SFX_ZAP); else q_sfx(SFX_GHOST_TICK);
            for (int f = 0; f < 4; f++) { df_put(at, F[f], FC[f]); q_wait(slow ? 0.14f : 0.05f); clock += slow ? 0.14f : 0.05f; }
            map[at] = DF_FREE; df_show(at, DF_FREE);
            souls++;
            df_status(todo ? moves * 100 / todo : 100, clock, souls, souls == 1 ? "Consuming souls..." : NULL);
            continue;
        }
        int slow = moves < 36;
        if (slow) { df_put(r, 'r', 10); q_wait(0.08f); df_put(w, 'W', 12); q_wait(0.08f); clock += 0.16f; }
        map[w] = DF_BONE; map[r] = DF_FREE;
        df_show(w, DF_BONE); df_show(r, DF_FREE);
        q_wait(slow ? 0.06f : 0.022f); clock += slow ? 0.06f : 0.022f;
        moves++;
        if (moves % 8 == 0) df_status(todo ? moves * 100 / todo : 100, clock, souls, moves == 8 ? "Moving remains..." : NULL);
        if (moves % 24 == 0) q_sfx(SFX_HDD);
        w++; r--;
    }
    df_status(100, clock, souls, "Finished condensing.");
    q_wait(1.0f);
    char buf[64];
    b_box(20, 8, 40, 8, 0, 7, 1); b_title(20, 8, 40, 0, 7, "Finished"); b_shadow(20, 8, 40, 8);
    b_center(10, 0, 7, "Finished condensing drive C:");
    snprintf(buf, sizeof buf, "%d souls consumed.", souls);
    b_center(11, 4, 7, buf);
    b_center(12, 0, 7, "All remains moved to the front.");
    b_center(14, 15, 0, "   OK   ");
    q_rows(8, 17);
    q_sfx(SFX_BEEP_POST);
    q_wait(3.2f);
    q_sfx(SFX_ENTER);
    q_cls();
    q(OP_CURSOR, 1, 0, 0, "");
    q_wait(0.6f);
    ghost_at_prompt("thank you. we were so scattered.", 6.5f);
    q_wait(1.8f);
    q(OP_GFADE, 0, 0, 1.6f, "");
    q_wait(1.8f);
    q_print("\n");
}

/* ------------------------------------------------------------------ */
/* A modem call to a board that should not answer                      */
/* ------------------------------------------------------------------ */

static void bbs_status(const char *state) {
    b_fill(0, 0, COLS, 1, ' ', 0, 3);
    b_text(1, 0, 0, 3, "TERMINUS 3.1 \xB3 Alt-Z=Help \xB3 ANSI-BBS \xB3 14400 N81 \xB3 FDX \xB3");
    b_text(64, 0, 15, 3, state);
    q_rows(0, 0);
}

/* letters three pixels wide, five tall, drawn twice as wide in half blocks */
static void b_logo(int x, int y, const char *word, const uint8_t *rowfg) {
    static const char *const L[8][5] = {
        { "###", " # ", " # ", " # ", " # " }, { "# #", "# #", "###", "# #", "# #" }, { "###", "#  ", "## ", "#  ", "###" },
        { "###", "# #", "# #", "# #", "###" }, { "## ", "# #", "## ", "# #", "# #" }, { "###", "#  ", "###", "  #", "###" },
        { "###", " # ", " # ", " # ", "###" }, { "## ", "# #", "# #", "# #", "## " } };
    static const char KEY[] = "THEORSID";
    for (; *word; word++) {
        if (*word == ' ') { x += 4; continue; }
        const char *k = strchr(KEY, *word);
        if (!k) continue;
        const char *const *g = L[k - KEY];
        for (int ty = 0; ty < 3; ty++)
            for (int px = 0; px < 3; px++) {
                int top = g[2 * ty][px] == '#', bot = ty < 2 && g[2 * ty + 1][px] == '#';
                uint8_t ch = top && bot ? 0xDB : top ? 0xDF : bot ? 0xDC : ' ';
                b_fill(x + px * 2, y + ty, 2, 1, ch, rowfg[ty], 0);
            }
        x += 8;
    }
}

static void act_bbs(void) {
    q_mood(0.5f);
    user_cmd("term");
    q_wait(0.6f);
    q_cls();
    b_fill(0, 0, COLS, ROWS, ' ', 7, 0);
    bbs_status("Offline  00:00");
    q(OP_TOP, 1, 0, 0, "");
    q_at(0, 2);
    q_wait(1.0f);
    q_type("ATDT 555-0666", 9.0f);
    q_wait(0.3f);
    q_enter();
    q_sfx(SFX_DIAL);
    q_wait(1.9f);
    q_sfx(SFX_MODEM);
    q_wait(3.7f);
    q_line(FG(F) "CONNECT 14400/ARQ/V32/LAPM" RESET, 0.4f);
    bbs_status("Online   00:01");
    q_wait(1.0f);
    q(OP_CURSOR, 0, 0, 0, "");
    b_fill(0, 1, COLS, 24, ' ', 7, 0);
    static const uint8_t REDS[3] = { 12, 12, 4 };
    b_logo(6, 3, "THE OTHER", REDS);
    b_logo(24, 7, "SIDE", REDS);
    b_fill(4, 11, 72, 1, 0xC4, 3, 0);
    b_center(12, 7, 0, "Node 13 of 13  \xFA  SpecterBBS 6.66  \xFA  SysOp: The Departed");
    b_center(13, 8, 0, "Callers today: 0   Callers ever: 1,031,666");
    b_fill(4, 14, 72, 1, 0xC4, 3, 0);
    for (int y = 1; y < 15; y++) { q_rows(y, y); q_wait(0.06f); }    /* ANSI art arriving at 14400 */
    q(OP_CURSOR, 1, 0, 0, "");
    q_at(2, 16); q_print(FG(E) "Handle: " FG(F));
    q_wait(0.8f); q_type_c(15, 0, "GUEST", 8.0f); q_enter();
    q_at(2, 17); q_print(FG(E) "Password: " FG(F));
    q_wait(0.5f); q_type_c(15, 0, "******", 8.0f); q_enter();
    q_wait(0.9f);
    q_at(2, 19); q_print(FG(7) "Welcome back, " FG(F) "GUEST" FG(7) ". Your last call was " FG(F) "10-31-1994" FG(7) ".");
    q_wait(0.5f);
    q_at(2, 20); q_print(FG(7) "You have " FG(C) "1" FG(7) " new message. Read it now? [Y/n] " FG(F));
    q_wait(1.0f); q_type_c(15, 0, "y", 4.0f); q_enter();
    q_wait(0.7f);
    q(OP_CURSOR, 0, 0, 0, "");
    b_fill(0, 1, COLS, 24, ' ', 7, 0);
    b_box(2, 2, 76, 6, 3, 0, 0);
    b_text(4, 3, 3, 0, "Msg #1 of 1          Area: PRIVATE");
    b_text(4, 4, 3, 0, "From:"); b_text(10, 4, 15, 0, "Grandma");
    b_text(4, 5, 3, 0, "To  :"); b_text(10, 5, 15, 0, "GUEST");
    b_text(4, 6, 3, 0, "Subj:"); b_text(10, 6, 15, 0, "the porch light");
    q_rows(1, 24);
    q_wait(0.8f);
    q_at(4, 9);  q_print(FG(7) "we left the porch light on for you, dear."); q_wait(0.7f);
    q_at(4, 10); q_print(FG(7) "it is so cold here. why don't you ever visit?"); q_wait(0.7f);
    q_at(4, 11); q_print(FG(7) "everyone is here now. everyone but you."); q_wait(1.3f);
    q_at(4, 13); q_print(FG(B) "[R]eply  [N]ext  [Q]uit: ");
    q_wait(1.2f);
    q_at(4, 15); q_print(FG(E) "*** The SysOp is paging you ***");
    for (int k = 0; k < 3; k++) { q_sfx(SFX_BEEP_POST); q_wait(0.45f); }
    q_at(4, 16); q_print(FG(8) "Chat mode. Press ESC to leave.");
    q(OP_CURSOR, 1, 0, 0, "");
    q_wait(1.0f);
    q_at(4, 18); q_print(FG(E) "SysOp: ");
    q_gplain_c(14, 0, "you should not have called this number.", 11.0f); q_wait(0.9f);
    q_at(4, 19); q_print(FG(F) "GUEST: ");
    q_type_c(15, 0, "who is this?", 7.0f); q_wait(0.9f);
    q_at(4, 20); q_print(FG(E) "SysOp: ");
    q_gplain_c(14, 0, "you know who. stay on the line.", 10.0f); q_wait(1.4f);
    q_sfx(SFX_STATIC);
    q_at(4, 22); q_print(FG(F) "NO CARRIER");
    bbs_status("Offline  01:06");
    q_wait(1.4f);
    for (int k = 0; k < 3; k++) {
        q_print("\n" FG(F) "    RING");
        q_sfx(SFX_BEEP_ERR); q_wait(1.3f);
    }
    q_print("\n\n    ");
    q_gtype("pick up.", 5.0f);
    q_wait(2.0f);
    q(OP_GFADE, 0, 0, 1.6f, "");
    q_wait(1.8f);
    q_cls();
    q(OP_CURSOR, 1, 0, 0, "");
}

/* ------------------------------------------------------------------ */
/* A virus scan that finds something it cannot clean                   */
/* ------------------------------------------------------------------ */

static void av_buttons(int sel) {
    static const char *const BTN[3] = { " Clean ", " Delete ", " Continue " };
    int x = 21;
    for (int i = 0; i < 3; i++) {
        int w = (int)strlen(BTN[i]);
        b_text(x, 14, i == sel ? 0 : 15, i == sel ? 7 : 4, BTN[i]);
        B[14][x - 1].ch = '<'; B[14][x + w].ch = '>';
        b_attr(x - 1, 14, 1, 1, 15, 4); b_attr(x + w, 14, 1, 1, 15, 4);
        x += w + 5;
    }
    q_rows(14, 14);
}

static void act_virus(void) {
    static const char *const FILES[] = {
        "C:\\COMMAND.COM", "C:\\DOS\\EDIT.COM", "C:\\DOS\\DEFRAG.EXE", "C:\\DOS\\MOUSE.COM", "C:\\DOS\\SMARTDRV.EXE",
        "C:\\WINDOWS\\WIN.COM", "C:\\WINDOWS\\SOL.EXE", "C:\\AUTOEXEC.BAT", "C:\\SEANCE.BAT", "C:\\BOOGEY.COM",
        "C:\\WITCH.DOC", "C:\\GHOST.EXE", "C:\\SOULS.DLL" };
    const int nf = (int)(sizeof FILES / sizeof FILES[0]);
    char buf[96];
    q_mood(0.55f);
    user_cmd("pav c:");
    q_wait(0.5f);
    b_fill(0, 0, COLS, ROWS, 0xB0, 9, 1);
    b_fill(0, 0, COLS, 1, ' ', 15, 4);
    b_text(1, 0, 15, 4, "Phantom Anti-Virus 2.0");
    b_text(54, 0, 7, 4, "(C) 1994 Exorcist Software");
    b_fill(0, 24, COLS, 1, ' ', 0, 3); b_text(1, 24, 0, 3, "F1=Help  Esc=Cancel");
    b_box(8, 4, 64, 12, 0, 7, 1); b_title(8, 4, 64, 0, 7, "Scanning drive C:"); b_shadow(8, 4, 64, 12);
    b_text(11, 6, 0, 7, "Memory ......................");
    b_text(11, 7, 0, 7, "Boot sector .................");
    b_text(11, 8, 0, 7, "Partition table .............");
    b_text(11, 10, 0, 7, "Scanning:");
    q(OP_CURSOR, 0, 0, 0, "");
    q_rows(0, 24);
    q_sfx(SFX_HDD);
    for (int k = 0; k < 3; k++) { q_wait(0.7f); b_text(42, 6 + k, 2, 7, "clean"); q_rows(6 + k, 6 + k); }
    for (int i = 0; i < nf; i++) {
        b_fill(21, 10, 48, 1, ' ', 1, 7); b_text(21, 10, 1, 7, FILES[i]);
        int fill = (i + 1) * 50 / nf;
        b_fill(11, 12, 50, 1, 0xB0, 8, 7); b_fill(11, 12, fill, 1, 0xDB, 1, 7);
        snprintf(buf, sizeof buf, "Files scanned: %4d      Viruses found: %d", 37 + i * 61, 0);
        b_text(11, 14, 0, 7, buf);
        q_rows(10, 14);
        if (i % 4 == 0) q_sfx(SFX_HDD);
        q_wait(i == nf - 1 ? 1.4f : 0.3f);
    }
    q_sfx(SFX_STATIC); q_sfx(SFX_BEEP_ERR);
    b_box(14, 7, 52, 10, 15, 4, 1); b_shadow(14, 7, 52, 10);
    b_center(8, 14, 4, "VIRUS FOUND");
    b_center(10, 15, 4, "POSSESSION has infected C:\\SOULS.DLL");
    b_center(11, 15, 4, "Type: resident, stealth, incorporeal");
    q_rows(7, 18);
    av_buttons(-1);
    q_wait(1.6f);
    for (int sel = 2; sel >= 0; sel--) { av_buttons(sel); q_sfx(SFX_KEY); q_wait(0.4f); }
    q_sfx(SFX_ENTER);
    b_fill(15, 10, 50, 2, ' ', 15, 4); b_center(10, 15, 4, "Cleaning C:\\SOULS.DLL ...");
    q_rows(10, 11);
    for (int k = 0; k <= 22; k++) { b_fill(19, 12, 42, 1, 0xB0, 12, 4); b_fill(19, 12, k, 1, 0xDB, 15, 4); q_rows(12, 12); q_wait(k < 18 ? 0.08f : 0.5f); }
    q_sfx(SFX_BEEP_ERR);
    b_fill(15, 10, 50, 3, ' ', 15, 4); b_center(10, 14, 4, "Cannot clean: the host is already dead.");
    q_rows(10, 12);
    q_wait(2.2f);
    av_buttons(1); q_sfx(SFX_KEY); q_wait(0.8f); q_sfx(SFX_ENTER);
    b_fill(15, 10, 50, 3, ' ', 15, 4); b_center(10, 15, 4, "Deleting C:\\SOULS.DLL ...");
    q_rows(10, 12);
    q_wait(1.3f);
    q(OP_CURSOR, 1, 0, 0, "");
    q_at(17, 12);
    q_gplain_c(15, 4, "you cannot delete me. i was here first.", 7.0f);
    q_wait(2.2f);
    q(OP_CURSOR, 0, 0, 0, "");
    b_fill(0, 1, COLS, 23, 0xB0, 9, 1);
    b_box(8, 4, 64, 12, 0, 7, 1); b_title(8, 4, 64, 0, 7, "Scan complete"); b_shadow(8, 4, 64, 12);
    b_text(13, 7, 0, 7, "Files scanned:        830");
    b_text(13, 8, 0, 7, "Viruses found:          1");
    b_text(13, 9, 0, 7, "Viruses removed:        0");
    b_text(13, 10, 4, 7, "Files possessed:        1");
    b_text(13, 12, 0, 7, "Your computer is not alone.");
    q_rows(1, 23);
    q_wait(3.5f);
    q_cls();
    q(OP_CURSOR, 1, 0, 0, "");
    q_wait(0.6f);
}

/* ------------------------------------------------------------------ */
/* MSD: the diagnostics find who handles IRQ 13                        */
/* ------------------------------------------------------------------ */

static const char *const MSD_BTN[13] = {
    "Computer...", "Memory...", "Video...", "Network...", "OS Version...", "Mouse...", "Other Adapters...",
    "Disk Drives...", "LPT Ports...", "COM Ports...", "IRQ Status...", "TSR Programs...", "Device Drivers..." };
static const char *const MSD_VAL[13] = {
    "Phantom 486DX2", "640K, 15360K Ext", "VGA, Possessed", "The Other Side", "DOS 6.66", "Moves on its own", "Ouija Board",
    "A: C: and one more", "1 (printing alone)", "2 (one is ringing)", "", "13, all undead", "GHOST.SYS" };

static void msd_button(int i, int sel) {
    int x = i < 7 ? 2 : 41, y = 3 + (i % 7) * 3;
    b_fill(x, y, 19, 1, ' ', 0, sel ? 3 : 7);
    b_text(x + 1, y, 0, sel ? 3 : 7, MSD_BTN[i]);
    B[y][x + 1].fg = sel ? 15 : 4;
    b_fill(x + 19, y, 1, 1, 0xDC, 0, 1); b_fill(x + 1, y + 1, 19, 1, 0xDF, 0, 1);   /* the button's drop shadow */
    q_rows(y, y + 1);
}

static void act_msd(void) {
    static const char *const IRQ[16][4] = {
        { "0E62:0000", "Timer Click", "Yes", "STOPPED.EXE" }, { "0E62:0040", "Keyboard", "Yes", "(cold hands)" },
        { "F000:EF6F", "Second 8259A", "Yes", "Default Handlers" }, { "0DA1:003D", "COM2: COM4:", "COM2:", "RINGING" },
        { "0DA1:0057", "COM1: COM3:", "COM1: Mouse", "MOUSE.COM" }, { "F000:EF6F", "LPT2:", "No", "Default Handlers" },
        { "F000:EF57", "Floppy Disk", "Yes", "BIOS" }, { "0070:06F4", "LPT1:", "Yes", "Printing alone" },
        { "0E62:0058", "Real-Time Clock", "Yes", "Stopped at 12:00" }, { "F000:ECF3", "Redirected IRQ2", "Yes", "BIOS" },
        { "F000:EF6F", "(Reserved)", "", "Default Handlers" }, { "F000:EF6F", "(Reserved)", "", "Default Handlers" },
        { "F000:EF6F", "(Reserved)", "", "Default Handlers" }, { "0666:0013", "Math Coprocessor", "Yes", "GHOST.SYS" },
        { "F000:EF6F", "Fixed Disk", "Yes", "BIOS" }, { "F000:EF6F", "(Reserved)", "", "Default Handlers" } };
    static BCell keep[ROWS][COLS];
    char buf[96];
    q_mood(0.5f);
    user_cmd("msd");
    q_wait(0.5f);
    q_line("\nMortuary System Diagnostics is examining your system...", 1.8f);
    b_fill(0, 0, COLS, ROWS, ' ', 15, 1);
    b_fill(0, 0, COLS, 1, ' ', 0, 7); b_text(1, 0, 0, 7, "File  Utilities  Help"); B[0][1].fg = 4; B[0][7].fg = 4; B[0][18].fg = 4;
    b_center(1, 15, 1, "Mortuary System Diagnostics 6.66");
    for (int i = 0; i < 13; i++) {
        int x = i < 7 ? 2 : 41, y = 3 + (i % 7) * 3;
        b_text(x + 21, y, 15, 1, MSD_VAL[i]);
    }
    b_fill(0, 24, COLS, 1, ' ', 0, 7); b_text(1, 24, 0, 7, "Press ALT for menu, or press highlighted letter, or F3 to quit MSD.");
    q(OP_CURSOR, 0, 0, 0, "");
    q_rows(0, 24);
    for (int i = 0; i < 13; i++) msd_button(i, 0);
    q_sfx(SFX_HDD);
    q_wait(2.6f);
    for (int i = 0; i <= 10; i++) {                                    /* it tabs along on its own */
        if (i) msd_button(i - 1, 0);
        msd_button(i, 1);
        q_sfx(SFX_KEY);
        q_wait(i == 10 ? 0.9f : 0.28f);
    }
    q_sfx(SFX_ENTER);
    memcpy(keep, B, sizeof B);
    b_box(2, 2, 76, 21, 0, 7, 1); b_title(2, 2, 76, 0, 7, "IRQ Status"); b_shadow(2, 2, 76, 21);
    b_text(4, 4, 1, 7, "IRQ  Address    Description         Detected         Handled By");
    for (int i = 0; i < 16; i++) {
        snprintf(buf, sizeof buf, "%3d  %-9s  %-18s  %-15s  %s", i, IRQ[i][0], IRQ[i][1], IRQ[i][2], IRQ[i][3]);
        b_text(4, 5 + i, 0, 7, buf);
    }
    b_center(21, 15, 0, "   OK   ");
    for (int y = 2; y <= 23; y++) { q_rows(y, y); q_wait(0.03f); }
    q_wait(1.8f);
    for (int k = 0; k < 6; k++) {                                     /* IRQ 13 will not keep still */
        b_attr(3, 18, 74, 1, k & 1 ? 0 : 15, k & 1 ? 7 : 4);
        q_rows(18, 18);
        if (!k) q_sfx(SFX_STATIC);
        q_wait(0.3f);
    }
    b_attr(3, 18, 74, 1, 15, 4); q_rows(18, 18);
    q_wait(1.0f);
    b_fill(0, 24, COLS, 1, ' ', 7, 0); q_rows(24, 24);
    q(OP_CURSOR, 1, 0, 0, "");
    q_at(2, 24);
    q_gtype("that one is me. i handle your interrupts now.", 6.5f);
    q_wait(2.2f);
    q(OP_GFADE, 0, 0, 1.6f, "");
    q_wait(1.8f);
    q(OP_CURSOR, 0, 0, 0, "");
    q_sfx(SFX_ENTER);
    memcpy(B, keep, sizeof B); q_rows(0, 24);
    q_wait(1.2f);
    q_cls();
    q(OP_CURSOR, 1, 0, 0, "");
    q_wait(0.5f);
}

/* ------------------------------------------------------------------ */
/* SETUP: thirteen disks, and the last one is not yours                */
/* ------------------------------------------------------------------ */

static void setup_page(void) {
    b_fill(0, 0, COLS, ROWS, ' ', 7, 1);
    b_text(1, 1, 15, 1, "ECTOPLASM 2.0 Setup");
    b_fill(1, 2, 19, 1, 0xCD, 15, 1);
    b_fill(0, 24, COLS, 1, ' ', 0, 7);
}

static void setup_copy(int pct, const char *file) {
    char buf[48];
    b_fill(16, 12, 48, 1, ' ', 0, 7);
    snprintf(buf, sizeof buf, "Copying: %s", file);
    b_text(18, 12, 0, 7, buf);
    b_fill(18, 14, 44, 1, 0xB0, 8, 7); b_fill(18, 14, pct * 44 / 100, 1, 0xDB, 4, 7);
    snprintf(buf, sizeof buf, "%3d%%", pct);
    b_text(38, 15, 0, 7, buf);
    q_rows(12, 15);
}

static void setup_disk(const char *l1, const char *l2) {
    b_box(18, 8, 44, 7, 15, 4, 1); b_shadow(18, 8, 44, 7);
    b_center(10, 15, 4, l1);
    b_center(11, 14, 4, l2);
    b_center(13, 7, 4, "Press ENTER when ready.");
    q_rows(8, 15);
}

static void act_setup(void) {
    static const char *const FILES[] = { "SOULS.DAT", "CHAINS.DLL", "WAIL.DRV", "COLD.SPT", "SHROUD.FON", "MOAN.WAV", "LANTERN.EXE", "HAUNT.INI" };
    static BCell keep[ROWS][COLS];
    char buf[48];
    q_mood(0.5f);
    user_cmd("a:setup");
    q_sfx(SFX_FLOPPY); q_wait(1.6f);
    setup_page();
    b_text(3, 5, 15, 1, "Welcome to Setup.");
    b_text(3, 7, 7, 1, "The Setup program prepares Ectoplasm 2.0 to run on your");
    b_text(3, 8, 7, 1, "computer, and your computer to run on Ectoplasm.");
    b_text(5, 10, 7, 1, "\xFA To set up Ectoplasm now, press ENTER.");
    b_text(5, 12, 7, 1, "\xFA To learn more about Setup before continuing, press F1.");
    b_text(5, 14, 7, 1, "\xFA To leave this world without installing, press F3.");
    b_text(1, 24, 0, 7, "ENTER=Continue  F1=Help  F3=Exit");
    q(OP_CURSOR, 0, 0, 0, "");
    q_rows(0, 24);
    q_wait(4.2f);
    q_sfx(SFX_ENTER);
    setup_page();
    b_text(3, 5, 7, 1, "Setup will place your Ectoplasm files in the following directory:");
    b_text(9, 8, 15, 1, "C:\\BEYOND");
    b_text(3, 11, 7, 1, "If this is where you want them, press ENTER.");
    b_text(3, 13, 7, 1, "You cannot choose a different directory. Nobody can.");
    b_text(1, 24, 0, 7, "ENTER=Continue  F1=Help  F3=Exit");
    q_rows(0, 24);
    q_wait(3.6f);
    q_sfx(SFX_ENTER);
    setup_page();
    b_box(14, 9, 52, 9, 0, 7, 1); b_title(14, 9, 52, 0, 7, "Setup is copying files"); b_shadow(14, 9, 52, 9);
    b_text(1, 24, 0, 7, "Please wait...");
    q_rows(0, 24);
    for (int i = 0; i < 8; i++) {
        setup_copy(i * 6, FILES[i]);
        if (i % 2 == 0) q_sfx(SFX_FLOPPY);
        q_wait(0.55f);
    }
    memcpy(keep, B, sizeof B);
    setup_disk("Please insert Disk 2 of 13", "ECTOPLASM 2.0  Disk 2");
    q_wait(1.8f); q_sfx(SFX_ENTER); q_sfx(SFX_FLOPPY);
    for (int d = 3; d <= 12; d++) {                                   /* the disks go by */
        memcpy(B, keep, sizeof B);
        snprintf(buf, sizeof buf, "Please insert Disk %d of 13", d);
        char l2[32]; snprintf(l2, sizeof l2, "ECTOPLASM 2.0  Disk %d", d);
        setup_disk(buf, l2);
        q_wait(d < 6 ? 0.45f : 0.22f);
        if (d % 3 == 0) q_sfx(SFX_FLOPPY);
    }
    memcpy(B, keep, sizeof B);
    setup_disk("Please insert Disk 13 of 13", "labelled: DO NOT OPEN");
    q_wait(3.2f);
    q_sfx(SFX_FLOPPY);                                                /* nobody touched it */
    q_wait(1.5f);
    memcpy(B, keep, sizeof B); q_rows(8, 15);
    q(OP_CURSOR, 1, 0, 0, "");
    q_at(18, 16);
    q_gplain_c(4, 7, "thank you.", 5.0f);
    q(OP_CURSOR, 0, 0, 0, "");
    for (int p = 48; p <= 100; p += 4) { setup_copy(p, p < 80 ? "YOU.DAT" : "YOU.DAT (all of it)"); q_wait(0.14f); }
    q_wait(1.0f);
    setup_page();
    b_text(3, 5, 15, 1, "Ectoplasm 2.0 is now installed.");
    b_text(3, 7, 7, 1, "It will start by itself, every night, at midnight.");
    b_text(3, 8, 7, 1, "It cannot be removed.");
    b_text(5, 11, 7, 1, "\xFA To restart your computer and begin, press ENTER.");
    b_text(1, 24, 0, 7, "ENTER=Restart");
    q_rows(0, 24);
    q_wait(4.0f);
    q_sfx(SFX_ENTER);
    q_cls();
    q(OP_CURSOR, 1, 0, 0, "");
    q_wait(0.8f);
}

/* ------------------------------------------------------------------ */
/* QBasic: a program that writes itself, and runs                     */
/* ------------------------------------------------------------------ */

static void qb_status(int line, int col) {
    char buf[32];
    b_fill(0, 24, COLS, 1, ' ', 15, 3);
    b_text(1, 24, 15, 3, "<Shift+F1=Help> <F6=Window> <F2=Subs> <F5=Run> <F8=Step>");
    snprintf(buf, sizeof buf, "%05d:%03d", line, col);
    b_text(70, 24, 0, 3, buf);
    q_rows(24, 24);
}

static void act_qbasic(void) {
    static const char *const PROG[8] = {
        "' GHOST.BAS - nobody wrote this", "SCREEN 0: CLS", "FOR KNOCK = 1 TO 3", "    SOUND 37, 9: SLEEP 1", "NEXT KNOCK",
        "COLOR 15: LOCATE 12, 33", "PRINT \"I AM IN HERE\"", "DO: LOOP WHILE INKEY$ = \"\"" };
    static const char *const MENU[7] = { "File", "Edit", "View", "Search", "Run", "Debug", "Options" };
    static BCell keep[ROWS][COLS];
    q_mood(0.5f);
    user_cmd("qbasic ghost.bas");
    q_wait(0.9f);
    b_fill(0, 0, COLS, ROWS, ' ', 7, 1);
    b_fill(0, 0, COLS, 1, ' ', 0, 7);
    int mx[7];
    for (int i = 0, x = 2; i < 7; x += (int)strlen(MENU[i]) + 2, i++) { mx[i] = x; b_text(x, 0, 0, 7, MENU[i]); B[0][x].fg = 15; }
    b_text(74, 0, 0, 7, "Help"); B[0][74].fg = 15;
    b_box(0, 1, COLS, 19, 7, 1, 0); b_title(0, 1, COLS, 1, 7, "GHOST.BAS");
    b_fill(79, 2, 1, 17, 0xB0, 0, 7); b_fill(79, 2, 1, 1, 0x18, 0, 7); b_fill(79, 18, 1, 1, 0x19, 0, 7);
    b_box(0, 20, COLS, 4, 7, 1, 0); b_title(0, 20, COLS, 7, 1, "Immediate");
    B[20][0].ch = 0xC3; B[20][79].ch = 0xB4;
    q(OP_CURSOR, 1, 0, 0, "");
    q_rows(0, 23);
    qb_status(1, 1);
    q_at(1, 2);
    q_wait(1.4f);
    for (int i = 0; i < 8; i++) {
        q_at(1, 2 + i);
        q_gplain_c(7, 1, PROG[i], 18.0f);
        b_text(1, 2 + i, 7, 1, PROG[i]);
        qb_status(i + 1, (int)strlen(PROG[i]) + 1);
        q_wait(0.3f);
    }
    q_wait(1.2f);
    memcpy(keep, B, sizeof B);                                        /* Run > Start */
    q(OP_CURSOR, 0, 0, 0, "");
    b_attr(mx[4] - 1, 0, 5, 1, 15, 0);
    b_box(mx[4] - 1, 1, 24, 5, 0, 7, 0);
    b_text(mx[4] + 1, 2, 15, 0, "Start      Shift+F5 "); b_attr(mx[4], 2, 22, 1, 15, 0);
    b_text(mx[4] + 1, 3, 0, 7, "Restart");
    b_text(mx[4] + 1, 4, 0, 7, "Continue         F5");
    b_shadow(mx[4] - 1, 1, 24, 5);
    q_rows(0, 7);
    q_sfx(SFX_KEY);
    q_wait(1.2f);
    q_sfx(SFX_ENTER);
    q_cls();
    q_wait(1.0f);
    for (int k = 0; k < 3; k++) { q_sfx(SFX_KNOCK); q_wait(1.0f); }
    q(OP_PUTS, 33, 11, 0, FG(F) "I AM IN HERE");
    q_wait(3.0f);
    q_at(33, 13);
    q(OP_CURSOR, 1, 0, 0, "");
    q_gtype("let me out", 4.5f);
    q_wait(2.2f);
    q(OP_GFADE, 0, 0, 1.6f, "");
    q_wait(1.8f);
    q(OP_PUTS, 0, 24, 0, FG(7) "Press any key to continue");
    q(OP_CURSOR, 0, 0, 0, "");
    q_wait(3.0f);
    q_cls();
    q(OP_CURSOR, 1, 0, 0, "");
    q_wait(0.5f);
}

/* ------------------------------------------------------------------ */
/* Necro Commander: moving the dead from one directory to another      */
/* ------------------------------------------------------------------ */

typedef struct { char name[9], ext[4], size[8], date[12]; } NcFile;

static void nc_panel(int x, const char *title, const NcFile *f, int n, int cur, unsigned marked, int active) {
    b_box(x, 0, 40, 23, 11, 1, 1);
    b_fill(x + 13, 1, 1, 19, 0xB3, 11, 1); b_fill(x + 23, 1, 1, 19, 0xB3, 11, 1); b_fill(x + 32, 1, 1, 19, 0xB3, 11, 1);
    B[0][x + 13].ch = 0xD1; B[0][x + 23].ch = 0xD1; B[0][x + 32].ch = 0xD1;
    b_fill(x + 1, 20, 38, 1, 0xC4, 11, 1); B[20][x].ch = 0xC7; B[20][x + 39].ch = 0xB6;
    B[20][x + 13].ch = 0xC1; B[20][x + 23].ch = 0xC1; B[20][x + 32].ch = 0xC1;
    b_text(x + 5, 1, 14, 1, "Name"); b_text(x + 17, 1, 14, 1, "Size"); b_text(x + 26, 1, 14, 1, "Date"); b_text(x + 34, 1, 14, 1, "Time");
    char t[24]; snprintf(t, sizeof t, " %s ", title);
    b_text(x + (40 - (int)strlen(t)) / 2, 0, active ? 0 : 11, active ? 3 : 1, t);
    b_text(x + 1, 2, 11, 1, "..");
    b_text(x + 14, 2, 11, 1, "\x10UP--DIR\x11");
    for (int i = 0; i < n; i++) {
        int y = 3 + i, fg = (marked >> i & 1) ? 14 : 11;
        char buf[16];
        b_text(x + 1, y, fg, 1, f[i].name); b_text(x + 10, y, fg, 1, f[i].ext);
        snprintf(buf, sizeof buf, "%9s", f[i].size); b_text(x + 14, y, fg, 1, buf);
        b_text(x + 24, y, fg, 1, f[i].date); b_text(x + 33, y, fg, 1, "12:00a");
    }
    if (active && cur >= -1) {
        int y = 3 + cur, fg = cur >= 0 && (marked >> cur & 1) ? 14 : 0;
        b_attr(x + 1, y, 12, 1, fg, 3); b_attr(x + 14, y, 9, 1, fg, 3); b_attr(x + 24, y, 8, 1, fg, 3); b_attr(x + 33, y, 6, 1, fg, 3);
    }
    if (cur >= 0 && cur < n) {
        char buf[48];
        snprintf(buf, sizeof buf, "%-8s %-3s %9s %s", f[cur].name, f[cur].ext, f[cur].size, f[cur].date);
        b_text(x + 1, 21, 11, 1, buf);
    }
}

static void nc_keys(void) {
    static const char *const K[10] = { "Help", "Menu", "View", "Edit", "Copy", "RenMov", "Mkdir", "Delete", "PullDn", "Quit" };
    b_fill(0, 24, COLS, 1, ' ', 7, 0);
    for (int i = 0, x = 0; i < 10; i++) {
        char num[12]; snprintf(num, sizeof num, "%d", i + 1);
        b_text(x, 24, 7, 0, num); x += (int)strlen(num);
        b_fill(x, 24, 6, 1, ' ', 0, 3); b_text(x, 24, 0, 3, K[i]); x += 6 + (i < 9 ? 1 : 0);
    }
}

static void act_nc(void) {
    static const char *const DEAD[10][2] = { { "ABIGAIL", "1,702" }, { "ELIJAH", "1,771" }, { "EZEKIEL", "1,804" }, { "HANNAH", "1,811" },
        { "JOSIAH", "1,838" }, { "MARY", "1,850" }, { "MERCY", "1,867" }, { "PRUDENCE", "1,883" }, { "SILAS", "1,902" }, { "TEMPERAN", "1,919" } };
    static NcFile graves[11], souls[10];
    char me[16], date[24];
    user_name(me, sizeof me);
    today(date, sizeof date);
    int ng = 10, ns = 0;
    for (int i = 0; i < 10; i++) {
        snprintf(graves[i].name, sizeof graves[i].name, "%s", DEAD[i][0]); snprintf(graves[i].ext, sizeof graves[i].ext, "BON");
        snprintf(graves[i].size, sizeof graves[i].size, "%s", DEAD[i][1]);
        snprintf(graves[i].date, sizeof graves[i].date, "10-31-%02u", (3u + (unsigned)i * 17u) % 100u);
    }
    q_mood(0.55f);
    user_cmd("nc");
    q_wait(0.7f);
    b_fill(0, 0, COLS, ROWS, ' ', 7, 0);
    nc_panel(0, "C:\\GRAVES", graves, ng, -1, 0, 1);
    nc_panel(40, "C:\\SOULS", souls, ns, -2, 0, 0);
    b_text(0, 23, 7, 0, "C:\\GRAVES>");
    nc_keys();
    q(OP_CURSOR, 0, 0, 0, "");
    q_rows(0, 24);
    q_wait(2.0f);
    unsigned marked = 0;
    for (int i = 0; i < ng; i++) {                                    /* down the list, marking every one */
        marked |= 1u << i;
        nc_panel(0, "C:\\GRAVES", graves, ng, i + 1 < ng ? i + 1 : i, marked, 1);
        q_rows(0, 22);
        q_sfx(SFX_KEY);
        q_wait(0.24f);
    }
    q_wait(0.8f);
    static BCell keep[ROWS][COLS];
    memcpy(keep, B, sizeof B);
    b_box(14, 8, 52, 8, 0, 7, 1); b_title(14, 8, 52, 0, 7, "Rename/Move"); b_shadow(14, 8, 52, 8);
    b_text(17, 10, 0, 7, "Rename or move 10 files to");
    b_fill(17, 11, 46, 1, ' ', 0, 3); b_text(18, 11, 0, 3, "C:\\SOULS\\*.SOL");
    b_text(26, 13, 15, 0, " Rename/Move "); b_text(44, 13, 0, 7, " Cancel ");
    q_rows(8, 17);
    q_wait(1.8f);
    q_sfx(SFX_ENTER);
    q_sfx(SFX_WHISPER);
    for (int i = 0; i < 10; i++) {
        memcpy(B, keep, sizeof B);
        souls[ns] = graves[0]; snprintf(souls[ns].ext, sizeof souls[ns].ext, "SOL"); ns++;
        memmove(graves, graves + 1, sizeof graves[0] * (size_t)(ng - 1)); ng--;
        marked >>= 1;
        b_fill(0, 0, COLS, 23, ' ', 7, 0);
        nc_panel(0, "C:\\GRAVES", graves, ng, ng ? 0 : -1, marked, 1);
        nc_panel(40, "C:\\SOULS", souls, ns, -2, 0, 0);
        memcpy(keep, B, sizeof B);
        char buf[48];
        b_box(20, 9, 40, 5, 0, 7, 1); b_title(20, 9, 40, 0, 7, "Moving"); b_shadow(20, 9, 40, 5);
        snprintf(buf, sizeof buf, "%s.BON", DEAD[i][0]); b_center(10, 0, 7, buf);
        b_fill(23, 11, 34, 1, 0xB0, 8, 7); b_fill(23, 11, (i + 1) * 34 / 10, 1, 0xDB, 1, 7);
        q_rows(0, 22);
        if (i % 3 == 0) q_sfx(SFX_HDD);
        q_wait(0.42f);
    }
    memcpy(B, keep, sizeof B); q_rows(0, 22);
    q_wait(2.0f);
    snprintf(graves[0].name, sizeof graves[0].name, "%.8s", me); snprintf(graves[0].ext, sizeof graves[0].ext, "BON");
    snprintf(graves[0].size, sizeof graves[0].size, "0"); snprintf(graves[0].date, sizeof graves[0].date, "%.8s", date);
    ng = 1;                                                            /* something new in GRAVES */
    nc_panel(0, "C:\\GRAVES", graves, ng, -1, 0, 1);
    q_rows(0, 22);
    q_sfx(SFX_CREAK);
    q_wait(1.6f);
    nc_panel(0, "C:\\GRAVES", graves, ng, 0, 0, 1);
    q_rows(0, 22);
    q_sfx(SFX_KEY);
    q_wait(1.6f);
    memcpy(keep, B, sizeof B);
    char buf[48];
    b_box(20, 8, 40, 7, 15, 4, 1); b_title(20, 8, 40, 15, 4, "Delete"); b_shadow(20, 8, 40, 7);
    b_center(9, 15, 4, "Do you wish to delete");
    snprintf(buf, sizeof buf, "%s.BON", me); b_center(10, 14, 4, buf);
    b_text(26, 12, 0, 7, " Delete "); b_text(46, 12, 15, 4, " Cancel ");
    q_rows(8, 15);
    q_sfx(SFX_BEEP_ERR);
    q_wait(1.6f);
    b_text(26, 12, 15, 4, " Delete "); b_text(46, 12, 0, 7, " Cancel ");   /* it moves to Cancel by itself */
    q_rows(12, 12);
    q_sfx(SFX_KEY);
    q_wait(0.8f);
    q_sfx(SFX_ENTER);
    memcpy(B, keep, sizeof B); q_rows(0, 22);
    q(OP_CURSOR, 1, 0, 0, "");
    q_at(10, 23);
    q_gtype("not yet.", 4.5f);
    q_wait(2.4f);
    q(OP_GFADE, 0, 0, 1.6f, "");
    q_wait(1.8f);
    q_cls();
    q_wait(0.6f);
}

typedef struct { const char *l1, *l2, *l3; } Caption;
static const Caption CAPTION[ART_COUNT] = {
    { "happy halloween.", "i have been carved for you.", "" },
    { "welcome back.", "you have been watching for a long time.", "do you hear it? the fans are singing." },
    { "home sweet home.", "someone left the light on.", "" },
    { "she has been in the corner since 1994.", "do not worry. she only bites when you blink.", "" },
    { "boo.", "i had something scarier planned,", "but i forgot what it was." },
    { "do not worry. they are very slow.", "but they never, ever stop.", "" },
    { "they only dance while the music plays.", "please do not turn it off.", "" },
};
/* how long each piece stays up after its captions, so its whole 30 s story plays out */
static const float ART_HOLD[ART_COUNT] = { 16.0f, 9.0f, 12.0f, 14.0f, 10.0f, 13.0f, 12.0f };

static void act_art(int kind) {
    q_mood(0.80f);
    q_cls();
    q(OP_CURSOR, 0, 0, 0, "");
    q(OP_ART, kind, 0, 0, "");
    q_sfx(SFX_SHIMMER);
    q_wait(3.0f);
    if (kind == ART_HOUSE) q_sfx(SFX_CREAK);
    if (kind == ART_SKULL) q(OP_TALK, 1, 0, 0, "");
    q_gtype(CAPTION[kind].l1, 6.5f);
    q_wait(1.2f);
    if (CAPTION[kind].l2[0]) { q_gtype("\n", 99.0f); q_gtype(CAPTION[kind].l2, 6.5f); q_wait(1.2f); }
    if (CAPTION[kind].l3[0]) { q_gtype("\n", 99.0f); q_gtype(CAPTION[kind].l3, 6.5f); q_wait(1.2f); }
    q_wait(ART_HOLD[kind]);
    q(OP_TALK, 0, 0, 0, "");
    q(OP_ARTEND, 0, 0, 0, "");
    q(OP_GFADE, 0, 0, 1.6f, "");
    q_wait(2.4f);
    q_cls();
    q(OP_CURSOR, 1, 0, 0, "");
    q_wait(0.6f);
}

static void box_line(char *dst, size_t n, char l, char m, char r, int w) {
    size_t k = 0;
    dst[k++] = l; for (int i = 0; i < w && k + 2 < n; i++) dst[k++] = m; dst[k++] = r; dst[k] = 0;
}

static void act_crash(void) {
    char a[104], b[104];
    q_mood(1.0f);
    ghost_at_prompt("i'm coming out", 6.0f);
    q_wait(1.2f);
    q_sfx(SFX_STINGER);
    q(OP_GLITCH, 0, 0, 2.5f, "");
    q(OP_ALARM, 0, 0, 3.0f, "");
    q_wait(0.6f);
    for (int i = 0; i < 26; i++) { q(OP_GARBAGE, 30 + i * 4, 0, 0, ""); q_wait(0.05f); }
    q_sfx(SFX_STATIC); q_sfx(SFX_BOOM);
    q_wait(0.4f);
    q_cls();
    q(OP_CURSOR, 0, 0, 0, "");
    q_wait(0.6f);
    const int bx = 16, by = 7, bw = 46;
    box_line(a, sizeof a, '\311', '\315', '\273', bw);
    q_at(bx, by); q_print(FG(C)); q_print(a);
    for (int r = 1; r <= 7; r++) {
        snprintf(b, sizeof b, "\272%*s\272", bw, "");
        q_at(bx, by + r); q_print(b);
    }
    box_line(a, sizeof a, '\310', '\315', '\274', bw);
    q_at(bx, by + 8); q_print(a);
    q_sfx(SFX_BEEP_ERR);
    q_at(bx + 2, by + 1); q_print(FG(E) "FATAL ERROR 0x666 AT 0028:C0013666");
    q_wait(0.6f);
    q_at(bx + 2, by + 3); q_print(FG(F) "The dead have risen. The system has been");
    q_wait(0.4f);
    q_at(bx + 2, by + 4); q_print("possessed and will now be haunted.");
    q_wait(1.0f);
    q_at(bx + 2, by + 6); q_print(FG(7) "Press any key to continue _");
    q_wait(2.6f);
    q_at(bx + 2, by + 6); q_print("Press any key to continue ");
    q_gtype("y", 3.0f);
    q_wait(1.4f);
    q_sfx(SFX_BOOM);
    q(OP_CRT, 1, 0, 0, "");
    q_wait(0.8f);
    q(OP_BLACK, 1, 0, 0, "");
    q_mood(0.0f);
    q_wait(1.6f);
}

/* ------------------------------------------------------------------ */
/* Planning: one cycle per "possession"                                */
/* ------------------------------------------------------------------ */

static void shuffle(int *v, int n) {
    for (int i = n - 1; i > 0; i--) { int j = rnd(i + 1); int t = v[i]; v[i] = v[j]; v[j] = t; }
}

/* the next card of a deck, reshuffled when it runs out; never one already picked this cycle */
static int draw_card(int *deck, int *n, const int *all, int count, const int *picked, int npicked) {
    if (*n == 0) { memcpy(deck, all, sizeof(int) * (size_t)count); shuffle(deck, count); *n = count; }
    for (int tries = 0; tries < 2 * count; tries++) {
        int dup = 0;
        for (int i = 0; i < npicked; i++) dup |= picked[i] == deck[*n - 1];
        if (!dup) break;
        int j = rnd(*n), t = deck[j]; deck[j] = deck[*n - 1]; deck[*n - 1] = t;
    }
    return deck[--*n];
}

static void plan_cycle(void) {
#ifdef HAUNT_DEBUG
    const char *forced = getenv("HAUNT_PLAN");
    if (forced) {                                   /* e.g. HAUNT_PLAN=10,11,15 */
        int n = 0;
        for (const char *p = forced; *p && n < 38; ) { S.plan[n++] = atoi(p); while (*p && *p != ',') p++; if (*p == ',') p++; }
        S.plan_n = n; S.plan_i = 0;
        return;
    }
#endif
    /* hauntings and art pieces come from decks, so every one is seen before any repeats (across visits too) */
    static const int TEXT[] = { A_CHKDSK, A_FORMAT, A_MEM, A_DEL, A_IDLE, A_ABORT, A_SOULS, A_README,
                                A_EDIT, A_DEFRAG, A_BBS, A_VIRUS, A_MSD, A_SETUP, A_QBASIC, A_NC };
    static const int ART[] = { A_ART0, A_ART1, A_ART2, A_ART3, A_ART4, A_ART5, A_ART6 };
    enum { NTEXT = (int)(sizeof TEXT / sizeof TEXT[0]), NART = (int)(sizeof ART / sizeof ART[0]) };
    static int deck[NART], deck_n, tdeck[NTEXT], tdeck_n;
    int as[4], hs[6];
    for (int k = 0; k < 4; k++) as[k] = draw_card(deck, &deck_n, ART, NART, as, k);
    for (int k = 0; k < 6; k++) hs[k] = draw_card(tdeck, &tdeck_n, TEXT, NTEXT, hs, k);
    int n = 0;
    S.plan[n++] = A_BOOT; S.plan[n++] = A_INTRO;
    S.plan[n++] = hs[0]; S.plan[n++] = hs[1];
    S.plan[n++] = as[0];
    S.plan[n++] = hs[2]; S.plan[n++] = hs[3];
    S.plan[n++] = as[1];
    S.plan[n++] = hs[4];
    S.plan[n++] = as[2];
    S.plan[n++] = hs[5];
    S.plan[n++] = as[3];
    S.plan[n++] = A_CRASH;
    S.plan_n = n; S.plan_i = 0;
}

static void schedule_next(void) {
    if (S.plan_i >= S.plan_n) plan_cycle();
    int act = S.plan[S.plan_i++];
#ifdef HAUNT_DEBUG
    fprintf(stderr, "[%7.1f] act %d\n", S.t, act);
#endif
    switch (act) {
    case A_BOOT: act_boot(); break;
    case A_INTRO: act_intro(); break;
    case A_CHKDSK: act_chkdsk(); break;
    case A_FORMAT: act_format(); break;
    case A_MEM: act_mem(); break;
    case A_DEL: act_del(); break;
    case A_IDLE: act_idle(); break;
    case A_ABORT: act_abort(); break;
    case A_SOULS: act_souls(); break;
    case A_README: act_readme(); break;
    case A_CRASH: act_crash(); break;
    case A_EDIT: act_edit(); break;
    case A_DEFRAG: act_defrag(); break;
    case A_BBS: act_bbs(); break;
    case A_VIRUS: act_virus(); break;
    case A_MSD: act_msd(); break;
    case A_SETUP: act_setup(); break;
    case A_QBASIC: act_qbasic(); break;
    case A_NC: act_nc(); break;
    default: act_art(act - A_ART0); break;
    }
}

/* ------------------------------------------------------------------ */
/* Script execution                                                    */
/* ------------------------------------------------------------------ */

static void sfx_type(int ghost, char c) {
    if (c == ' ') return;
    snd_sfx(ghost ? SFX_GHOST_TICK : SFX_KEY);
}

static int exec_type(Op *o, int ghost) {
    const char *s = o->s;
    while (s[o->b] == 1 || s[o->b] == 2 || s[o->b] == 3) o->b += tprint_unit(&s[o->b], ghost);
    char c = s[o->b];
    if (!c) return 1;
    int cells = ghost && !o->c;                     /* plain: the ghost typing into a program */
    if (ghost && o->a == 1) {                       /* undo a glitched character */
        tprint_char('\b', cells); o->a = 0; S.vt -= 0.07;
        return 0;
    }
    if (ghost && c != ' ' && c != '\n' && c != '\b' && rndf() < 0.045f) {
        static const char JUNK[] = "#$%&@?*";
        tprint_char((uint8_t)JUNK[rnd((int)sizeof JUNK - 1)], cells);
        snd_sfx(SFX_GHOST_TICK);
        o->a = 1; S.vt -= 0.09;
        return 0;
    }
    o->b++;
    tprint_char((uint8_t)c, cells);
    if (c == '\n') { if (!ghost) snd_sfx(SFX_ENTER); }
    else sfx_type(ghost, c);
    if (ghost) S.talk = 1.0f;
    float d = 1.0f / (o->f > 0 ? o->f : 8.0f);
    d *= 0.55f + 0.9f * rndf();
    if (c == ' ') d *= 1.2f;
    if (c == '.' || c == ',' || c == '?') d *= 2.2f;
    if (ghost && rndf() < 0.07f) d += 0.25f + 0.35f * rndf();
    if (!ghost && rndf() < 0.05f) d += 0.15f + 0.2f * rndf();
    S.vt -= d;
    return s[o->b] == 0;
}

static void garbage(int n) {
    for (int i = 0; i < n; i++) {
        Cell *c = &S.cell[rnd(ROWS)][rnd(COLS)];
        static const uint8_t DIM[] = { 7, 8, 4, 2, 5, 3, 6 };          /* mostly dim colours, black backgrounds */
        c->ch = (uint8_t)(1 + rnd(254)); c->fg = DIM[rnd((int)sizeof DIM)]; c->bg = (uint8_t)(rnd(14) == 0 ? 4 : 0);
        c->ghost = 0;
    }
}

static int exec(Op *o) {
    switch (o->op) {
    case OP_WAIT: S.vt -= o->f; return 1;
    case OP_PRINT: tprint(o->s, 0); return 1;
    case OP_TYPE: return exec_type(o, 0);
    case OP_GTYPE: return exec_type(o, 1);
    case OP_ENTER: tprint_char('\n', 0); snd_sfx(SFX_ENTER); return 1;
    case OP_SFX: snd_sfx(o->a); return 1;
    case OP_CLS: cls(); S.gfading = 0; S.gfade = 0; return 1;
    case OP_MOOD: snd_set_mood(o->f); return 1;
    case OP_CURSOR: S.cursor_vis = o->a; return 1;
    case OP_LOCATE: S.cx = o->a; S.cy = o->b; return 1;
    case OP_TOP: S.top = o->a; return 1;
    case OP_ART:
        S.art_kind = o->a; S.art_state = 1; S.art_t = 0; S.art_reveal = 0;
        S.top = ART_ROWS; S.cx = 0; S.cy = ART_ROWS;
        return 1;
    case OP_ARTEND: if (S.art_state) S.art_state = 2; return 1;
    case OP_GLITCH: S.glitch = o->f; return 1;
    case OP_BLACK: S.black = o->a; return 1;
    case OP_CRT: S.crt_mode = o->a; S.crt_t = 0; if (o->a == 2) S.black = 0; return 1;
    case OP_GFADE: S.gfading = 1; S.gfade = 0; S.gfade_len = o->f; return 1;
    case OP_ALARM: S.alarm = o->f; return 1;
    case OP_GARBAGE: garbage(o->a); return 1;
    case OP_TALK: if (!o->a) S.talk = 0; return 1;
    case OP_PUTS: puts_at(o->a, o->b, o->s); return 1;
    case OP_SCROLL: scroll_rows(o->a, o->b, o->c); return 1;
    default: return 1;
    }
}

static void haunt_enter(uint32_t seed) {
    memset(&S, 0, sizeof S);
    S.rs = seed | 1u; S.grs = (seed * 2654435761u) | 1u;
    S.cursor_vis = 1;
    cls();
    plan_cycle();
    S.vt = 0;
}

static void haunt_update(double dt) {
    S.t += dt; S.blink += dt;
    S.vt += dt;
    int guard = 0;
    while (S.vt >= 0 && guard++ < 600) {
        if (qempty()) { schedule_next(); if (qempty()) break; }
        Op *o = &S.q[S.qh];
        if (exec(o)) S.qh = (S.qh + 1) % QMAX;
    }
    if (S.vt > 0.5) S.vt = 0.5;

    for (int y = 0; y < ROWS; y++)
        for (int x = 0; x < COLS; x++) if (S.cell[y][x].ghost) S.cell[y][x].age += (float)dt;

    if (S.gfading) {
        S.gfade += (float)(dt / (S.gfade_len > 0.05f ? S.gfade_len : 0.05f));
        if (S.gfade >= 1.0f) {
            S.gfading = 0; S.gfade = 0;
            for (int y = 0; y < ROWS; y++)
                for (int x = 0; x < COLS; x++) if (S.cell[y][x].ghost) cell_blank(&S.cell[y][x]);
        }
    }
    S.talk -= (float)(dt * 5.0); if (S.talk < 0) S.talk = 0;
    if (S.glitch > 0) S.glitch -= (float)dt;
    if (S.alarm > 0) S.alarm -= (float)dt;

    if (S.crt_mode) {
        S.crt_t += dt;
        float len = S.crt_mode == 1 ? 0.35f : 0.32f;
        if (S.crt_t >= len) {
            if (S.crt_mode == 1) S.black = 1;
            S.crt_mode = 0;
        }
    }

    if (S.art_state) {
        S.art_t += dt;
        if (S.art_state == 1) { S.art_reveal += (float)(dt / 2.4); if (S.art_reveal > 1.0f) S.art_reveal = 1.0f; }
        else {
            S.art_reveal -= (float)(dt / 1.8);
            if (S.art_reveal <= 0.0f) { S.art_reveal = 0; S.art_state = 0; S.art_kind = -1; }
        }
        if (S.art_state) {
            art_draw(S.art_kind, S.art_t, S.talk);
            int ev = art_events();
            if (ev & ART_EV_LIGHTNING) { snd_sfx(SFX_STATIC); S.boom_t = 0.55 + 0.6 * rndf(); }
            if (ev & ART_EV_DOOR) snd_sfx(SFX_CREAK);
            if (ev & ART_EV_SPIDER) { snd_sfx(SFX_WHISPER); snd_set_mood(0.95f); }
            if (ev & ART_EV_DIRT) snd_sfx(SFX_DIRT);
            if (ev & ART_EV_GROAN) { snd_sfx(SFX_GROAN); snd_set_mood(0.85f); }
            if (ev & ART_EV_CAW) snd_sfx(SFX_CAW);
            if (ev & ART_EV_XYLO) snd_sfx(SFX_XYLO);
            if (ev & ART_EV_BONES) { snd_sfx_ex(SFX_BONES, 1.6f, 0.5f); snd_sfx_ex(SFX_BONES, 1.2f, 0.2f); snd_sfx_ex(SFX_BONES, 1.2f, 0.8f); }
            if (ev & ART_EV_WAIL) snd_sfx(SFX_WAIL);
            if (ev & ART_EV_BOO) { snd_sfx(SFX_STINGER); snd_set_mood(1.0f); }
            if (ev & ART_EV_IGNITE) snd_sfx(SFX_IGNITE);
            if (ev & ART_EV_WIND) { snd_sfx(SFX_WIND); snd_set_mood(0.9f); }
            if (ev & ART_EV_CACKLE) { snd_sfx(SFX_CACKLE); snd_set_mood(1.0f); }
            if (ev & ART_EV_BAT) snd_sfx(SFX_BAT);
            if (ev & ART_EV_CHATTER) snd_sfx(SFX_CHATTER);
            if (ev & ART_EV_SETTLE) snd_sfx_ex(SFX_BONES, 0.55f, 0.5f);
        }
    }
    if (S.boom_t > 0) { S.boom_t -= dt; if (S.boom_t <= 0) { snd_sfx(SFX_BOOM); S.boom_t = 0; } }
}

/* ------------------------------------------------------------------ */
/* Rendering                                                           */
/* ------------------------------------------------------------------ */

static const uint8_t STD16[16][3] = {
    {0,0,0},{0,0,42},{0,42,0},{0,42,42},{42,0,0},{42,0,42},{42,21,0},{42,42,42},
    {21,21,21},{21,21,63},{21,63,21},{21,63,63},{63,21,21},{63,21,63},{63,63,21},{63,63,63} };
static const uint8_t GHOST[8][3] = {
    {2,5,6},{4,10,12},{7,16,19},{10,24,27},{14,34,38},{19,44,48},{26,54,58},{34,63,63} };

static void setpal(Frame *f, int i, float r, float g, float b) {
    f->pal[i][0] = (uint8_t)clampi((int)(r + 0.5f), 0, 63);
    f->pal[i][1] = (uint8_t)clampi((int)(g + 0.5f), 0, 63);
    f->pal[i][2] = (uint8_t)clampi((int)(b + 0.5f), 0, 63);
}

static void set_palette(Frame *f) {
    float t = (float)S.t;
    for (int i = 0; i < 16; i++) setpal(f, i, STD16[i][0], STD16[i][1], STD16[i][2]);
    float breathe = 0.85f + 0.15f * sinf(t * 1.7f);
    for (int i = 0; i < 8; i++) setpal(f, PAL_GHOST0 + i, GHOST[i][0] * breathe, GHOST[i][1] * breathe, GHOST[i][2] * breathe);
    float fl = 0.90f + 0.10f * sinf(t * 11.0f) * sinf(t * 3.1f + 1.0f) + 0.04f * sinf(t * 37.0f);
    setpal(f, PAL_OR_HI, 63, 32, 0);
    setpal(f, PAL_OR_MID, 44, 18, 0);
    setpal(f, PAL_OR_LO, 24, 8, 0);
    setpal(f, PAL_CANDLE, 63 * fl, 57 * fl, 18 * fl);
    setpal(f, PAL_BONE_HI, 60, 58, 52);
    setpal(f, PAL_BONE_MID, 36, 34, 30);
    setpal(f, PAL_BONE_LO, 16, 15, 13);
    float eg = 0.65f + 0.35f * sinf(t * 4.0f);
    setpal(f, PAL_EYEGLOW, 63 * eg, 10 * eg, 6 * eg);
    if (S.alarm > 0) {                                /* red alert */
        float k = clampf(S.alarm, 0.0f, 1.0f);
        setpal(f, 7, lerpf(42, 57, k), lerpf(42, 8, k), lerpf(42, 8, k));
        setpal(f, 15, 63, lerpf(63, 24, k), lerpf(63, 24, k));
        setpal(f, 8, lerpf(21, 30, k), lerpf(21, 3, k), lerpf(21, 3, k));
    }
    if (S.art_state) art_palette(S.art_kind, f->pal);
}

static void draw_cells(Frame *f) {
    for (int y = 0; y < ROWS; y++)
        for (int x = 0; x < COLS; x++) {
            const Cell *c = &S.cell[y][x];
            int ch = c->ch, fg = c->fg, bg = c->bg;
            if (S.art_state && !c->ghost && ch == ' ' && bg == 0) {           /* art shows through blank text */
                const ArtCell *a = &art[y][x];
                float thr = (float)((x * 7919 + y * 104729 + 13) % 251) / 251.0f;
                if (a->ch && thr < S.art_reveal) { ch = a->ch; fg = a->fg; bg = a->bg; }
            }
            if (c->ghost) {
                float lvl = clampf(c->age * 1.6f, 0.0f, 1.0f) * 7.0f;
                if (S.gfading) lvl *= 1.0f - S.gfade;
                fg = PAL_GHOST0 + clampi((int)(lvl + 0.5f), 0, 7);
                if (lvl < 0.3f && S.gfading) fg = PAL_GHOST0;
                bg = 0;
            }
            const unsigned char *gl = font_vga16[ch];
            int ninth = ch >= 0xC0 && ch <= 0xDF;
            uint8_t *dst = &f->pix[(y * CELL_H) * f->w + x * CELL_W];
            for (int r = 0; r < CELL_H; r++, dst += f->w) {
                unsigned bits = gl[r];
                for (int cx = 0; cx < 8; cx++) dst[cx] = (uint8_t)((bits >> (7 - cx)) & 1 ? fg : bg);
                dst[8] = (uint8_t)(ninth && (bits & 1) ? fg : bg);
            }
        }
    if (S.cursor_vis && !S.art_state && (((int)(S.blink * 2.2)) & 1)) {
        int cx = S.cx, cy = S.cy;
        if (cx < COLS && cy < ROWS) {
            uint8_t col = 7;
            for (int r = 14; r < 16; r++)
                memset(&f->pix[(cy * CELL_H + r) * f->w + cx * CELL_W], col, CELL_W);
        }
    }
}

static void glitch_rows(Frame *f) {
    int bands = 3 + (int)(rng_next(&S.grs) % 5);
    uint8_t tmp[FB_MAX_W];
    for (int b = 0; b < bands; b++) {
        int y0 = (int)(rng_next(&S.grs) % 400), hgt = 3 + (int)(rng_next(&S.grs) % 36);
        int dx = (int)(rng_next(&S.grs) % 61) - 30;
        for (int y = y0; y < y0 + hgt && y < f->h; y++) {
            uint8_t *row = &f->pix[y * f->w];
            for (int x = 0; x < f->w; x++) tmp[x] = row[((x - dx) % f->w + f->w) % f->w];
            memcpy(row, tmp, (size_t)f->w);
        }
    }
}

static void crt_squash(Frame *f, float squash) {
    static uint8_t copy[FB_MAX_W * FB_MAX_H];
    memcpy(copy, f->pix, (size_t)f->w * f->h);
    int cy = f->h / 2;
    for (int y = 0; y < f->h; y++) {
        int sy = cy + (int)((y - cy) * squash);
        if (sy < 0 || sy >= f->h) memset(&f->pix[y * f->w], 0, (size_t)f->w);
        else memcpy(&f->pix[y * f->w], &copy[sy * f->w], (size_t)f->w);
    }
}

static void haunt_render(Frame *f) {
    f->w = 720; f->h = 400;
    set_palette(f);
    f->fade = 1.0f;
    if (S.black) { memset(f->pix, 0, (size_t)f->w * f->h); f->fade = 0.0f; return; }
    draw_cells(f);
    if (S.glitch > 0 && ((int)(S.t * 30) & 1)) glitch_rows(f);
    if (S.crt_mode) {
        float u = (float)(S.crt_t / (S.crt_mode == 1 ? 0.35 : 0.32));
        if (S.crt_mode == 2) u = 1.0f - u;
        float squash = 1.0f + u * u * u * 90.0f;
        crt_squash(f, squash);
        f->fade = clampf(1.0f - (u - 0.85f) * 6.0f, 0.0f, 1.0f);
    }
}

const Scene scene_haunt = { "HAUNT.EXE", 720, 400, SND_PROG_HAUNT, haunt_enter, haunt_update, haunt_render, NULL, NULL, NULL };   /* not playable */
