#include <am.h>
#include <klib-macros.h>
#include <NDL.h>

typedef void (*handler_t)(void *buf);

static void __am_timer_config(AM_TIMER_CONFIG_T *cfg) {
  cfg->present = true;
  cfg->has_rtc = true;
}

static void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime) {
  uptime->us = NDL_GetTicks() * 1000;
}

static void __am_input_config(AM_INPUT_CONFIG_T *cfg) { cfg->present = true; }

#define keyname(k) #k,
static const char *keyname[] = {
    "NONE",
    AM_KEYS(keyname)};

static void __am_input_keybrd(AM_INPUT_KEYBRD_T *kbd) {
  static char event_str[64];
  static char key_name[20];
  static char key_down;
  if (NDL_PollEvent(event_str, sizeof(event_str)) > 0) {
    sscanf(event_str, "k%c %s\n", &key_down, key_name);
    if (key_down == 'd') {
      kbd->keydown = true;
      for (int i = 0; i < LENGTH(keyname); i++) {
        if (key_name[0] == keyname[i][0] && strcmp(key_name, keyname[i]) == 0) {
          kbd->keycode = i;
          break;
        }
      }
    } else {
      kbd->keydown = false;
      for (int i = 0; i < LENGTH(keyname); i++) {
        if (key_name[0] == keyname[i][0] && strcmp(key_name, keyname[i]) == 0) {
          kbd->keycode = i;
          break;
        }
      }
    }
  } else {
    kbd->keydown = false;
    kbd->keycode = AM_KEY_NONE;
  }
}

static int canvas_w, canvas_h;
static void __am_gpu_init() {
  NDL_OpenCanvas(&canvas_w, &canvas_h);
}

static void __am_gpu_status(AM_GPU_STATUS_T *status) {
  status->ready = true;
}

static void __am_gpu_config(AM_GPU_CONFIG_T *cfg) {
  *cfg = (AM_GPU_CONFIG_T){
      .present = true, .has_accel = false, .width = canvas_w, .height = canvas_h, .vmemsz = 0};
}

static void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {
  NDL_DrawRect(ctl->pixels, ctl->x, ctl->y, ctl->w, ctl->h);
}

static int sbuf_size;
static void __am_audio_init() {
  sbuf_size = NDL_QueryAudio();
}

static void __am_audio_config(AM_AUDIO_CONFIG_T *cfg) {
  cfg->present = true;
  cfg->bufsize = sbuf_size;
}

static void __am_audio_ctrl(AM_AUDIO_CTRL_T *ctrl) {
  NDL_OpenAudio(ctrl->freq, ctrl->channels, ctrl->samples);
}

static void __am_audio_status(AM_AUDIO_STATUS_T *stat) {
  stat->count = sbuf_size - NDL_QueryAudio();
}

void __am_audio_play(AM_AUDIO_PLAY_T *ctl) {
  NDL_PlayAudio(ctl->buf.start, ctl->buf.end - ctl->buf.start);
}

static void *lut[128] = {
    [AM_TIMER_CONFIG] = __am_timer_config,
    // [AM_TIMER_RTC] = __am_timer_rtc,
    [AM_TIMER_UPTIME] = __am_timer_uptime,
    [AM_INPUT_CONFIG] = __am_input_config,
    [AM_INPUT_KEYBRD] = __am_input_keybrd,
    [AM_GPU_CONFIG] = __am_gpu_config,
    [AM_GPU_FBDRAW] = __am_gpu_fbdraw,
    [AM_GPU_STATUS] = __am_gpu_status,
    // [AM_UART_CONFIG] = __am_uart_config,
    [AM_AUDIO_CONFIG] = __am_audio_config,
    [AM_AUDIO_CTRL] = __am_audio_ctrl,
    [AM_AUDIO_STATUS] = __am_audio_status,
    [AM_AUDIO_PLAY] = __am_audio_play,
    // [AM_DISK_CONFIG] = __am_disk_config,
    // [AM_DISK_STATUS] = __am_disk_status,
    // [AM_DISK_BLKIO] = __am_disk_blkio,
    // [AM_NET_CONFIG] = __am_net_config,
};

static void fail(void *buf) { panic("access nonexist register"); }

bool ioe_init() {
  for (int i = 0; i < LENGTH(lut); i++)
    if (!lut[i])
      lut[i] = fail;
  __am_gpu_init();
  __am_audio_init();
  return true;
}

void ioe_read(int reg, void *buf) {
  // printf("ioe_read: %d\n", reg);
  ((handler_t)lut[reg])(buf);
}
void ioe_write(int reg, void *buf) {
  // printf("ioe_write: %d\n", reg);
  ((handler_t)lut[reg])(buf);
}