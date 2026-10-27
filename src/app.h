/* Platform-independent saver logic: scene rotation, fades, OLED care,
 * palette -> RGB conversion.  The Win32 shell and the test harness both sit
 * on top of this. */
#ifndef APP_H
#define APP_H

#include "core.h"

#define SCENE_COUNT 2               /* 0 = HAUNT.EXE, 1 = CATACOMB */

typedef struct AppConfig {
    int sound;                      /* 1 = play audio */
    int volume;                     /* 0..100 */
    int brightness;                 /* 30..100 % - global cap, easier on OLED */
    int scene_enabled[SCENE_COUNT];
    int stay_seconds;               /* how long each scene runs before the next */
    int pixel_shift;                /* slow drift of the whole picture */
    int first_scene;                /* scene to start with */
    int start_fade;                 /* fade in from black at start */
    int pad;                        /* a game controller may take over and play */
} AppConfig;

void app_default_config(AppConfig *c);

/* audio_driven_externally: the caller pumps snd_render(); otherwise the app
 * advances the sound clock itself so the visuals stay on the beat. */
void app_init(const AppConfig *cfg, int audio_driven_externally);
void app_update(double dt);

int  app_mode(int *w, int *h);                      /* current video mode */
void app_present(uint32_t *dst, int bgr);           /* w*h pixels; bgr=0: RGBA bytes, 1: 0x00RRGGBB */
void app_present_rgb(uint8_t *rgb, int *w, int *h); /* tightly packed, for tests */
void app_pixel_shift(int *dx, int *dy);             /* drift, in source pixels */
const char *app_scene_name(void);

/* Taking over: the platform keeps an Input up to date (app_update clears its edges after each
 * step). Holding H, or any button of a controller, for three seconds hands CATACOMB to the player. */
#define HOLD_SECONDS 3.0
enum { APP_AUTO, APP_HOLDING, APP_PLAYER };
void app_set_input(Input *in);
int  app_control_state(void);       /* APP_AUTO: input ends the saver; APP_HOLDING: only H matters; APP_PLAYER: keep it all */
void app_focus(int on);             /* the window lost (0) or regained (1) focus */
int  app_quit_requested(void);      /* the player chose to leave: write what changed, then exit */
void app_slot_load(int i, const uint8_t *buf, int len);   /* a stored slot (SLOT_*), before app_init */
int  app_slot_take(int i, uint8_t *buf, int cap);         /* a slot written since last asked: its length, else 0 */
void app_pad_motors(float *low, float *high);             /* the rumble asked for, now, 0..1 each */

#endif
