#include <NDL.h>
#include <SDL.h>
#include <stdlib.h>

static void (*SDL_Audio_callback)(void *userdata, uint8_t *stream, int len);
static uint8_t *SDL_Audio_buf;
static bool SDL_Audio_pause = true;
static uint16_t SDL_Audio_sample;

// static uint32_t SDL_Audio_interval_ms;
// static uint32_t SDL_Audio_tick;

// 打开音频功能, 并根据`*desired`中的成员来初始化声卡设备
// 初始化成功后, 音频播放处于暂停状态
int SDL_OpenAudio(SDL_AudioSpec *desired, SDL_AudioSpec *obtained) {
  printf("SDL_OpenAudio %d %d %d %d - %x\n", desired->freq, desired->channels, desired->samples, desired->size, obtained);
  NDL_OpenAudio(desired->freq, desired->channels, desired->samples);

  // SDL_OpenAudio calculates the size fields for both the desired and obtained specifications
  // The size field stores the total size of the audio buffer in bytes❓

  if (obtained) {
    obtained->freq = desired->freq;
    obtained->channels = desired->channels;
    obtained->samples = desired->samples;
    obtained->format = desired->format;
    obtained->size = desired->size;
  }
  SDL_Audio_sample = desired->samples;
  SDL_Audio_buf = malloc(SDL_Audio_sample);
  SDL_Audio_callback = desired->callback;

  // SDL_Audio_interval_ms = SDL_Audio_sample * 1000 / desired->freq;
  // SDL_Audio_tick = SDL_GetTicks();

  return 0;
}

// 关闭音频功能
void SDL_CloseAudio() {
  free(SDL_Audio_buf);
  SDL_Audio_callback = NULL;
  SDL_Audio_pause = true;
  NDL_CloseAudio();
}

// 在miniSDL中的一些会被频繁调用的API中插入CallbackHelper()
void CallbackHelper() {
  if (SDL_Audio_callback != NULL && !SDL_Audio_pause) {
    // uint32_t now = SDL_GetTicks();
    // if (now - SDL_Audio_tick > SDL_Audio_interval_ms) {
    // SDL_Audio_tick = now;
    if (NDL_QueryAudio() > SDL_Audio_sample) {
      SDL_Audio_callback(NULL, SDL_Audio_buf, SDL_Audio_sample);
      NDL_PlayAudio(SDL_Audio_buf, SDL_Audio_sample);
    }
    // }
  }
}

// 暂停/恢复音频的播放
void SDL_PauseAudio(int pause_on) {
  SDL_Audio_pause = pause_on;
}

void SDL_MixAudio(uint8_t *dst, uint8_t *src, uint32_t len, int volume) {
}

SDL_AudioSpec *SDL_LoadWAV(const char *file, SDL_AudioSpec *spec, uint8_t **audio_buf, uint32_t *audio_len) {
  return NULL;
}

void SDL_FreeWAV(uint8_t *audio_buf) {
}

void SDL_LockAudio() {
}

void SDL_UnlockAudio() {
}
