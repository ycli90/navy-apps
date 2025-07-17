#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <assert.h>

static int evtdev = -1;
static int sbcfgdev = -1;
static int fbdev = -1;
static int sbctldev = -1;
static int sbdev = -1;
static int screen_w = 0, screen_h = 0;
static int canvas_w = 0, canvas_h = 0;
static int canvas_x = 0, canvas_y = 0;

uint32_t NDL_GetTicks() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  uint32_t time_ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;
  return time_ms;
}

int NDL_PollEvent(char *buf, int len) {
  int nread = read(evtdev, buf, len - 1);
  if (nread > 0) {
    buf[nread] = '\0';
    return 1;
  }
  else return 0;
}

void NDL_OpenCanvas(int *w, int *h) {
  if (getenv("NWM_APP")) {
    int fbctl = 4;
    fbdev = 5;
    screen_w = *w; screen_h = *h;
    char buf[64];
    int len = sprintf(buf, "%d %d", screen_w, screen_h);
    // let NWM resize the window and create the frame buffer
    write(fbctl, buf, len);
    while (1) {
      // 3 = evtdev
      int nread = read(3, buf, sizeof(buf) - 1);
      if (nread <= 0) continue;
      buf[nread] = '\0';
      if (strcmp(buf, "mmap ok") == 0) break;
    }
    close(fbctl);
    canvas_w = screen_w;
    canvas_h = screen_h;
  }
  else {
    fbdev = open("/dev/fb", 0, 0);
    size_t N = 32;
    char buf[N];
    int fd = open("/proc/dispinfo", 0, 0);
    int nread = read(fd, (void*)buf, N);
    assert(nread > 0 && nread < N);
    int nscan = sscanf(buf, "WIDTH : %d\nHEIGHT : %d", &screen_w, &screen_h);
    assert(nscan == 2);
    if (*w == 0 && *h == 0) {
      *w = screen_w;
      *h = screen_h;
    } else {
      if (*w > screen_w) *w = screen_w;
      if (*h > screen_h) *h = screen_h;
    }
    canvas_w = *w;
    canvas_h = *h;
    canvas_x = (screen_w - canvas_w) / 2;
    canvas_y = (screen_h - canvas_h) / 2;
    // close(fd);
  }
}

void NDL_DrawRect(uint32_t *pixels, int x, int y, int w, int h) {
  assert(w >= 0 && w <= screen_w);
  if (w == screen_w) {
    assert(canvas_w == screen_w && x == 0);
    assert(y >= 0 && y + h <= canvas_h);
    lseek(fbdev, (canvas_y + y) * screen_w * 4, SEEK_SET);
    write(fbdev, pixels, w * h * 4);
  } else {
    assert(x >= 0 && x + w <= canvas_w);
    assert(y >= 0 && y + h <= canvas_h);
    for (int i = 0; i < h; i++) {
      int sx = canvas_x + x;
      int sy = canvas_y + y + i;
      lseek(fbdev, (sy * screen_w + sx) * 4, SEEK_SET);
      write(fbdev, pixels + i * w, w * 4);
    }
  }
}

void NDL_GetAudioCfg(int *present, int *p_bufsize) {
  char buf[32];
  read(sbcfgdev, buf, 32);
  int ret = sscanf(buf, "%d %d", present, p_bufsize);
  assert(ret == 2);
}

void NDL_OpenAudio(int freq, int channels, int samples) {
  sbctldev = open("/dev/sbctl", 0, 0);
  sbdev = open("/dev/sb", 0, 0);
  int cfg[3] = {freq, channels, samples};
  write(sbctldev, (const void*)cfg, 12);
}

void NDL_CloseAudio() {
  sbctldev = -1;
  sbdev = -1;
}

int NDL_PlayAudio(void *buf, int len) {
  if (sbdev == -1) return 0;
  return write(sbdev, buf, len);
}

int NDL_QueryAudio() {
  if (sbctldev == -1) return 0;
  char buf[16];
  int free_size;
  int nread = read(sbctldev, buf, 16);
  if (nread <= 0) return 0;
  int nscan = sscanf(buf, "%d", &free_size);
  assert(nscan == 1);
  return free_size;
}

int NDL_Init(uint32_t flags) {
  if (getenv("NWM_APP")) {
    evtdev = 3;
  } else {
    evtdev = open("/dev/events", 0, 0);
    sbcfgdev = open("/dev/sbcfg", 0, 0);
  }
  return 0;
}

void NDL_Quit() {
  NDL_CloseAudio();
}
