#include <NDL.h>
#include <SDL.h>
#include <stdio.h>
#include <string.h>

#define keyname(k) #k,
#define ARRLEN(arr) (sizeof(arr) / sizeof(arr[0]))

static const char *keyname[] = {
  "NONE",
  _KEYS(keyname)
};

static uint8_t key_state[ARRLEN(keyname)];

int SDL_PushEvent(SDL_Event *ev) {
  return 0;
}

int process_event(const char *buf, SDL_Event *event) {
  // printf("receive event: %s", buf);
  if (event == NULL) return 0;
  char action[4], key[16];
  sscanf(buf, "%s %s", action, key);
  if (strcmp(action, "kd") == 0) {
    event->type = SDL_KEYDOWN;
  } else if (strcmp(action, "ku") == 0) {
    event->type = SDL_KEYUP;
  } else {
    printf("SDL: Unrecognized keyboard event action %s\n", action);
    return 0;
  }
  bool found_key = false;
  for (int i = 0; i < ARRLEN(keyname); i++) {
    if (strcmp(key, keyname[i]) == 0) {
      event->key.keysym.sym = i;
      key_state[i] = (event->type == SDL_KEYDOWN ? 1 : 0);
      found_key = true;
      break;
    }
  }
  if (!found_key) {
    printf("SDL: Unrecognized keyboard event key %s\n", key);
    return 0;
  }
  return 1;
}

int SDL_PollEvent(SDL_Event *ev) {
  if (!reent) update_timer();
  if (ev == NULL) {
    printf("SDL_PollEvent: event is NULL\n");
  }
  char buf[32];
  if (NDL_PollEvent(buf, sizeof(buf)) == 0) return 0;
  else return process_event(buf, ev);
}

int SDL_WaitEvent(SDL_Event *event) {
  if (!reent) update_timer();
  if (event == NULL) {
    printf("SDL_WaitEvent: event is NULL\n");
  }
  char buf[64];
  while (NDL_PollEvent(buf, sizeof(buf)) == 0) { SDL_Delay(10); }
  return process_event(buf, event);
}

int SDL_PeepEvents(SDL_Event *ev, int numevents, int action, uint32_t mask) {
  if (!reent) update_timer();
  return 0;
}

uint8_t* SDL_GetKeyState(int *numkeys) {
  if (!reent) update_timer();
  if (numkeys != NULL) *numkeys = ARRLEN(key_state);
  return key_state;
}
