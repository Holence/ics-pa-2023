#include <NDL.h>
#include <sdl-timer.h>
#include <stdio.h>

SDL_TimerID SDL_AddTimer(uint32_t interval, SDL_NewTimerCallback callback, void *param) {
  return NULL;
}

int SDL_RemoveTimer(SDL_TimerID id) {
  return 1;
}

static uint32_t start_tick;
void SDL_InitTick() {
  start_tick = NDL_GetTicks();
}

uint32_t SDL_GetTicks() {
  return NDL_GetTicks() - start_tick;
}

void SDL_Delay(uint32_t ms) {
}
