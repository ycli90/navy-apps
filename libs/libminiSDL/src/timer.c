#include <NDL.h>
#include <sdl-timer.h>
#include <stdio.h>
#include <assert.h>
#include <sdl-general.h>

uint32_t boot_time;
#define N_TIMER 8

struct Timer {
  uint8_t enable;
  uint32_t start;
  uint32_t interval;
  SDL_NewTimerCallback callback;
  void *param;
} timers[N_TIMER];

void update_timer() {
  reent = 1;
  for (int i = 0; i < N_TIMER; i++) {
    if (!timers[i].enable) continue;
    if (SDL_GetTicks() >= timers[i].start + timers[i].interval) {
      timers[i].interval = timers[i].callback(timers[i].interval, timers[i].param);
      timers[i].start = SDL_GetTicks();
    }
  }
  reent = 0;
}

SDL_TimerID SDL_AddTimer(uint32_t interval, SDL_NewTimerCallback callback, void *param) {
  int timer_idx;
  for (timer_idx = 0; timer_idx < N_TIMER; timer_idx++) {
    if (timers[timer_idx].enable == 0) break;
  }
  if (timer_idx == N_TIMER) {
    printf("SDL_AddTimer: Too many timers\n");
    return NULL;
  }
  timers[timer_idx] = (struct Timer){.enable = 1, .start = SDL_GetTicks(), .interval = interval,
    .callback = callback, .param = param};
  return &timers[timer_idx++];
}

int SDL_RemoveTimer(SDL_TimerID id) {
  struct Timer *ptimer = (struct Timer*)id;
  assert(ptimer >= timers && ptimer < timers + N_TIMER);
  if (ptimer->enable) {
    ptimer->enable = 0;
    return 1;
  } else return 0;
}

uint32_t SDL_GetTicks() {
  if (!reent) update_timer();
  return (NDL_GetTicks() - boot_time) >> 0;
}

void SDL_Delay(uint32_t ms) {
  uint32_t t = SDL_GetTicks() + ms;
  while (SDL_GetTicks() < t) { if (!reent) update_timer(); }
}
