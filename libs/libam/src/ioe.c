#include <am.h>
#include <klib-macros.h>
#include <NDL.h>

#define NAME(key) \
  [AM_KEY_##key] = #key,

static const char *keyname[] __attribute__((used)) = {
  [AM_KEY_NONE] = "NONE",
  AM_KEYS(NAME)
};

void __am_input_keybrd(AM_INPUT_KEYBRD_T *event) {
  char buf[32];
  if (NDL_PollEvent(buf, sizeof(buf)) == 0) {
    event->keycode = AM_KEY_NONE;
    event->keydown = false; // necessary because uninitialized bool variable may take values other than 0 or 1
    return;
  }
  char action[4], key[16];
  sscanf(buf, "%s %s", action, key);
  if (strcmp(action, "kd") == 0) {
    event->keydown = true;
  } else if (strcmp(action, "ku") == 0) {
    event->keydown = false;
  } else {
    printf("libam: Unrecognized keyboard event action %s, buf: %s\n", action, buf);
    halt(1);
  }
  bool found_key = false;
  for (int i = 0; i < LENGTH(keyname); i++) {
    if (strcmp(key, keyname[i]) == 0) {
      event->keycode = i;
      found_key = true;
      break;
    }
  }
  if (!found_key) {
    printf("libam: Unrecognized keyboard event key %s\n", key);
    halt(1);
  }
}

uint64_t boot_time = 0;

void __am_timer_rtc(AM_TIMER_RTC_T *);
void __am_timer_uptime(AM_TIMER_UPTIME_T *t) {
  struct timeval now;
  gettimeofday(&now, NULL);
  t->us = now.tv_sec * 1000000LL + now.tv_usec - boot_time;
}

void __am_gpu_config(AM_GPU_CONFIG_T *cfg) {
  panic_on(getenv("NWM_APP"), "environment variable NWM_APP should not be defined");
  int w = 0, h = 0;
  NDL_OpenCanvas(&w, &h);
  cfg->width = w;
  cfg->height = h;
  cfg->present = true;
  cfg->has_accel = false;
  cfg->vmemsz = 0;
}

void __am_gpu_status(AM_GPU_STATUS_T *);
void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *reg) {
  NDL_DrawRect(reg->pixels, reg->x, reg->y, reg->w, reg->h);
}

void __am_audio_config(AM_AUDIO_CONFIG_T *cfg) {
  int present, bufsize;
  NDL_GetAudioCfg(&present, &bufsize);
  cfg->present = present;
  cfg->bufsize = bufsize;
}

void __am_audio_ctrl(AM_AUDIO_CTRL_T *ctrl) {
  NDL_OpenAudio(ctrl->freq, ctrl->channels, ctrl->samples);
}

void __am_audio_status(AM_AUDIO_STATUS_T *status) {
  int present, bufsize;
  NDL_GetAudioCfg(&present, &bufsize);
  status->count = bufsize - NDL_QueryAudio();
}

void __am_audio_play(AM_AUDIO_PLAY_T *data) {
  int len = data->buf.end - data->buf.start;
  NDL_PlayAudio(data->buf.start, len);
}

void __am_disk_config(AM_DISK_CONFIG_T *cfg);
void __am_disk_status(AM_DISK_STATUS_T *stat);
void __am_disk_blkio(AM_DISK_BLKIO_T *io);

static void __am_timer_config(AM_TIMER_CONFIG_T *cfg) { cfg->present = true; cfg->has_rtc = true; }
static void __am_input_config(AM_INPUT_CONFIG_T *cfg) { cfg->present = true;  }
static void __am_uart_config(AM_UART_CONFIG_T *cfg)   { cfg->present = false; }
static void __am_net_config (AM_NET_CONFIG_T *cfg)    { cfg->present = false; }

typedef void (*handler_t)(void *buf);
static void *lut[128] = {
  [AM_TIMER_CONFIG] = __am_timer_config,
  // [AM_TIMER_RTC   ] = __am_timer_rtc,
  [AM_TIMER_UPTIME] = __am_timer_uptime,
  [AM_INPUT_CONFIG] = __am_input_config,
  [AM_INPUT_KEYBRD] = __am_input_keybrd,
  [AM_GPU_CONFIG  ] = __am_gpu_config,
  [AM_GPU_FBDRAW  ] = __am_gpu_fbdraw,
  // [AM_GPU_STATUS  ] = __am_gpu_status,
  [AM_UART_CONFIG ] = __am_uart_config,
  [AM_AUDIO_CONFIG] = __am_audio_config,
  [AM_AUDIO_CTRL  ] = __am_audio_ctrl,
  [AM_AUDIO_STATUS] = __am_audio_status,
  [AM_AUDIO_PLAY  ] = __am_audio_play,
  // [AM_DISK_CONFIG ] = __am_disk_config,
  // [AM_DISK_STATUS ] = __am_disk_status,
  // [AM_DISK_BLKIO  ] = __am_disk_blkio,
  [AM_NET_CONFIG  ] = __am_net_config,
};

bool ioe_init() {
  struct timeval now;
  gettimeofday(&now, NULL);
  boot_time = now.tv_sec * 1000000LL + now.tv_usec;
  NDL_Init(0);
  return true;
}

void ioe_read (int reg, void *buf) { ((handler_t)lut[reg])(buf); }
void ioe_write(int reg, void *buf) { ((handler_t)lut[reg])(buf); }
