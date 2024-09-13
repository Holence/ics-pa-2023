#include <NDL.h>
#include <SDL.h>
#include <string.h>

#define keyname(k) #k,

static const char *keyname[] = {
    "NONE",
    _KEYS(keyname)};

#define LENGTH(arr) (sizeof(arr) / sizeof((arr)[0]))

static uint8_t keystate[LENGTH(keyname)] = {0};

int SDL_PushEvent(SDL_Event *ev) {
  return 0;
}

// Polls for currently pending events, and returns 1 if there are any pending events, or 0 if there are none available.
int SDL_PollEvent(SDL_Event *ev) {
  CallbackHelper();
  static char event_str[64];
  static char key_name[20];
  static char key_down;
  if (NDL_PollEvent(event_str, sizeof(event_str)) > 0) {
    sscanf(event_str, "k%c %s\n", &key_down, key_name);
    if (key_down == 'd') {
      ev->type = SDL_KEYDOWN;
      for (int i = 0; i < LENGTH(keyname); i++) {
        if (key_name[0] == keyname[i][0] && strcmp(key_name, keyname[i]) == 0) {
          ev->key.keysym.sym = i;
          break;
        }
      }
      keystate[ev->key.keysym.sym] = 1;
    } else {
      ev->type = SDL_KEYUP;
      for (int i = 0; i < LENGTH(keyname); i++) {
        if (key_name[0] == keyname[i][0] && strcmp(key_name, keyname[i]) == 0) {
          ev->key.keysym.sym = i;
          break;
        }
      }
      keystate[ev->key.keysym.sym] = 0;
    }
    return 1;
  } else {
    return 0;
  }
}

// Waits indefinitely for the next available event, returning 1, or 0 if there was an error while waiting for events.
int SDL_WaitEvent(SDL_Event *event) {
  while (1) {
    if (SDL_PollEvent(event))
      break;
  }
  return 1;
}

int SDL_PeepEvents(SDL_Event *ev, int numevents, int action, uint32_t mask) {
  return 0;
}

// 返回keystate数组，并告诉调用者numkeys为keystate数组的长度
uint8_t *SDL_GetKeyState(int *numkeys) {
  if (numkeys) {
    *numkeys = LENGTH(keyname);
  }
  return keystate;
}
