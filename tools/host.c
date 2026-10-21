/* Host-side test harness (Linux/anywhere): renders sound to WAV and scene
 * frames to PNG so the saver can be checked without a Windows machine.
 *
 *   host wav   <haunt|crawl> <seconds> out.wav [sfx_id@time ...]
 *   host shot  <haunt|crawl> <t1,t2,...> prefix [scale]   (PNG frames, 1x scale by default)
 *   host dump  <haunt|crawl> <seconds> prefix <every_n_s> (frames + audio together)
 *   host apl   <haunt|crawl> <seconds>                    (average picture level)
 *
 * HOST_INPUT plays a person at the keys, e.g. "1:+H;4.2:-H;5:m300,0;6:+W;8:-W;9:+MOUSE1;9.2:-MOUSE1;
 * 10:w1" (+ press, - release, m mouse counts, w wheel notches). HOST_SAVE=file writes the progress
 * and stops when the player chooses to leave; HOST_LOAD=file starts from saved progress.
 */
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include "../src/core.h"
#include "../src/snd.h"
#include "../src/app.h"

static void put32(FILE *f, uint32_t v) { fputc(v >> 24, f); fputc(v >> 16, f); fputc(v >> 8, f); fputc(v, f); }
static void chunk(FILE *f, const char *type, const uint8_t *data, uint32_t len) {
    put32(f, len);
    fwrite(type, 1, 4, f);
    if (len) fwrite(data, 1, len, f);
    uint32_t crc = crc32(0, (const Bytef *)type, 4);
    if (len) crc = crc32(crc, data, len);
    put32(f, crc);
}

static void write_png(const char *path, const uint8_t *rgb, int w, int h, int scale) {
    int W = w * scale, H = h * scale;
    size_t rowlen = 1 + (size_t)W * 3;
    uint8_t *raw = malloc(rowlen * H);
    for (int y = 0; y < H; y++) {
        uint8_t *row = raw + rowlen * y;
        row[0] = 0;
        for (int x = 0; x < W; x++) memcpy(row + 1 + x * 3, rgb + ((y / scale) * w + x / scale) * 3, 3);
    }
    uLongf clen = compressBound((uLong)(rowlen * H));
    uint8_t *comp = malloc(clen);
    compress2(comp, &clen, raw, (uLong)(rowlen * H), 6);
    FILE *f = fopen(path, "wb");
    static const uint8_t sig[8] = { 0x89, 'P', 'N', 'G', 13, 10, 26, 10 };
    fwrite(sig, 1, 8, f);
    uint8_t ihdr[13] = { W >> 24, W >> 16, W >> 8, W, H >> 24, H >> 16, H >> 8, H, 8, 2, 0, 0, 0 };
    chunk(f, "IHDR", ihdr, 13);
    chunk(f, "IDAT", comp, (uint32_t)clen);
    chunk(f, "IEND", NULL, 0);
    fclose(f);
    free(raw); free(comp);
}

static void write_wav(const char *path, const int16_t *pcm, int frames) {
    FILE *f = fopen(path, "wb");
    uint32_t dlen = (uint32_t)frames * 4, rate = SND_RATE;
    fwrite("RIFF", 1, 4, f); uint32_t v = 36 + dlen; fwrite(&v, 4, 1, f);
    fwrite("WAVEfmt ", 1, 8, f); v = 16; fwrite(&v, 4, 1, f);
    uint16_t s = 1; fwrite(&s, 2, 1, f); s = 2; fwrite(&s, 2, 1, f);
    fwrite(&rate, 4, 1, f); v = rate * 4; fwrite(&v, 4, 1, f);
    s = 4; fwrite(&s, 2, 1, f); s = 16; fwrite(&s, 2, 1, f);
    fwrite("data", 1, 4, f); fwrite(&dlen, 4, 1, f);
    fwrite(pcm, 4, (size_t)frames, f);
    fclose(f);
}

static int scene_by_name(const char *n) { return (!strcmp(n, "crawl") || !strcmp(n, "demo")) ? 1 : 0; }

/* ---- a scripted player ---- */
static Input g_in;
static struct { double t; int key, down, dx, dy, wheel; } g_ev[4096];
static int g_nev, g_iev;
static const char *const KEYNAME[KEY_COUNT] = { "UP", "DOWN", "LEFT", "RIGHT", "W", "A", "S", "D", "E", "Q", "SPACE", "CTRL",
    "SHIFT", "TAB", "ESC", "ENTER", "H", "Y", "N", "1", "2", "3", "4", "5", "6", "7", "8", "9", "MOUSE1", "MOUSE2" };
static void input_parse(const char *s) {
    static char buf[65536];
    snprintf(buf, sizeof buf, "%s", s);
    for (char *tok = strtok(buf, ";"); tok && g_nev < 4096; tok = strtok(NULL, ";")) {
        char *c = strchr(tok, ':');
        if (!c) continue;
        *c = 0;
        double t = atof(tok);
        char *a = c + 1;
        g_ev[g_nev].t = t; g_ev[g_nev].key = -1;
        if (*a == 'm') sscanf(a + 1, "%d,%d", &g_ev[g_nev].dx, &g_ev[g_nev].dy);
        else if (*a == 'w') g_ev[g_nev].wheel = atoi(a + 1);
        else if (*a == '+' || *a == '-') {
            for (int k = 0; k < KEY_COUNT; k++) if (!strcmp(a + 1, KEYNAME[k])) g_ev[g_nev].key = k;
            if (g_ev[g_nev].key < 0) { fprintf(stderr, "unknown key %s\n", a + 1); continue; }
            g_ev[g_nev].down = *a == '+';
        } else continue;
        g_nev++;
    }
}
static void input_step(double now) {
    while (g_iev < g_nev && g_ev[g_iev].t <= now) {
        int k = g_ev[g_iev].key;
        if (k >= 0) { if (g_ev[g_iev].down && !g_in.down[k]) g_in.hit[k] = 1; g_in.down[k] = (uint8_t)g_ev[g_iev].down; }
        g_in.mdx += (float)g_ev[g_iev].dx; g_in.mdy += (float)g_ev[g_iev].dy; g_in.wheel += g_ev[g_iev].wheel;
        g_iev++;
    }
}
/* the player chose to leave: keep the progress, as the saver would in the registry */
static int input_quit(void) {
    if (!app_quit_requested()) return 0;
    const char *path = getenv("HOST_SAVE");
    uint8_t blob[1024];
    int n = app_save(blob, sizeof blob);
    if (path) { FILE *f = fopen(path, "wb"); if (f) { fwrite(blob, 1, (size_t)n, f); fclose(f); } }
    fprintf(stderr, "QUIT: saved %d bytes%s%s\n", n, path ? " to " : "", path ? path : "");
    return 1;
}
static void input_init(void) {
    if (getenv("HOST_INPUT")) input_parse(getenv("HOST_INPUT"));
    app_set_input(&g_in);
    const char *path = getenv("HOST_LOAD");
    if (path) {
        uint8_t blob[1024];
        FILE *f = fopen(path, "rb");
        if (f) { size_t n = fread(blob, 1, sizeof blob, f); fclose(f); app_load(blob, (int)n); }
    }
}

int main(int argc, char **argv) {
    if (argc < 4) { fprintf(stderr, "usage: see source header\n"); return 1; }
    const char *cmd = argv[1];
    AppConfig cfg;
    app_default_config(&cfg);
    cfg.first_scene = scene_by_name(argv[2]);
    cfg.scene_enabled[0] = cfg.scene_enabled[1] = 0;
    cfg.scene_enabled[cfg.first_scene] = 1;
    cfg.pixel_shift = 0;
    cfg.start_fade = 0;
    if (getenv("BOTH")) cfg.scene_enabled[0] = cfg.scene_enabled[1] = 1;   /* exercise scene rotation */
    if (getenv("STAY")) cfg.stay_seconds = atoi(getenv("STAY"));

    if (!strcmp(cmd, "wav") || !strcmp(cmd, "dump")) {
        double secs = atof(argv[3]);
        const char *out = argv[4];
        double every = (!strcmp(cmd, "dump") && argc > 5) ? atof(argv[5]) : 0;
        input_init();
        app_init(&cfg, 1);
        int total = (int)(secs * SND_RATE);
        int16_t *pcm = malloc((size_t)total * 4);
        int pos = 0, tick = 0;
        const double dt = 1.0 / 70.0;
        int per = (int)(SND_RATE * dt);
        double next_shot = 0;
        uint8_t *rgb = malloc(FB_MAX_W * FB_MAX_H * 3);
        while (pos + per <= total) {
            input_step(tick * dt);
            app_update(dt);
            if (input_quit()) break;
            snd_render(pcm + pos * 2, per);
            pos += per; tick++;
            double now = tick * dt;
            if (every > 0 && now >= next_shot) {
                int w, h;
                app_present_rgb(rgb, &w, &h);
                char name[512];
                snprintf(name, sizeof name, "%s_%05.1f.png", out, now);
                write_png(name, rgb, w, h, w <= 320 ? 2 : 1);
                next_shot += every;
            }
        }
        if (!strcmp(cmd, "wav")) write_wav(out, pcm, pos);
        else { char name[512]; snprintf(name, sizeof name, "%s.wav", out); write_wav(name, pcm, pos); }
        free(pcm); free(rgb);
        return 0;
    }
    if (!strcmp(cmd, "bench")) {
        /* host bench <scene> <seconds>: CPU per frame for update + render, no audio */
        double secs = atof(argv[3]);
        input_init();
        app_init(&cfg, 1);
        uint8_t *rgb = malloc(FB_MAX_W * FB_MAX_H * 3);
        const double dt = 1.0 / 70.0;
        int frames = (int)(secs / dt), w, h;
        struct timespec a, b;
        clock_gettime(CLOCK_MONOTONIC, &a);
        for (int i = 0; i < frames; i++) { input_step(i * dt); app_update(dt); app_present_rgb(rgb, &w, &h); if (input_quit()) { frames = i + 1; break; } }
        clock_gettime(CLOCK_MONOTONIC, &b);
        double ms = ((b.tv_sec - a.tv_sec) * 1e3 + (b.tv_nsec - a.tv_nsec) / 1e6) / frames;
        printf("%s: %d frames, %.3f ms per frame\n", argv[2], frames, ms);
        free(rgb);
        return 0;
    }
    if (!strcmp(cmd, "apl")) {
        /* host apl <scene> <seconds>: average picture level (mean luma, % of white), sampled twice a second */
        double secs = atof(argv[3]), sum = 0.0, peak = 0.0, peak_t = 0.0;
        app_init(&cfg, 0);
        uint8_t *rgb = malloc(FB_MAX_W * FB_MAX_H * 3);
        const double dt = 1.0 / 70.0;
        int frames = (int)(secs / dt), n = 0, w, h;
        static double hist[101];
        for (int i = 1; i <= frames; i++) {
            app_update(dt);
            if (i % 35) continue;
            app_present_rgb(rgb, &w, &h);
            double acc = 0.0;
            for (int p = 0; p < w * h; p++) acc += 0.2126 * rgb[p * 3] + 0.7152 * rgb[p * 3 + 1] + 0.0722 * rgb[p * 3 + 2];
            double apl = acc / (w * h) / 255.0 * 100.0;
            sum += apl; n++; if (apl > peak) { peak = apl; peak_t = i * dt; }
            hist[(int)apl]++;
        }
        double c = 0.0; int p95 = 0;
        for (int i = 0; i <= 100; i++) { c += hist[i]; if (c >= 0.95 * n) { p95 = i; break; } }
        printf("%s: APL mean %.1f%%, 95th percentile %d%%, peak %.1f%% at %.0f s, over %d samples\n", argv[2], sum / n, p95, peak, peak_t, n);
        free(rgb);
        return 0;
    }
    if (!strcmp(cmd, "shot")) {
        /* host shot <scene> <t1,t2,t3,...> <prefix> [scale] */
        const char *prefix = argv[4];
        int scale = argc > 5 ? atoi(argv[5]) : 0;
        input_init();
        app_init(&cfg, 0);
        uint8_t *rgb = malloc(FB_MAX_W * FB_MAX_H * 3);
        const double dt = 1.0 / 70.0;
        double t = 0;
        char list[1024];
        snprintf(list, sizeof list, "%s", argv[3]);
        for (char *tok = strtok(list, ","); tok; tok = strtok(NULL, ",")) {
            double target = atof(tok);
            int quit = 0;
            while (t < target && !quit) { input_step(t); app_update(dt); t += dt; quit = input_quit(); }
            int w, h;
            app_present_rgb(rgb, &w, &h);
            char name[512];
            snprintf(name, sizeof name, "%s_%06.2f.png", prefix, target);
            write_png(name, rgb, w, h, scale ? scale : (w <= 320 ? 3 : 1));
        }
        free(rgb);
        return 0;
    }
    return 1;
}
