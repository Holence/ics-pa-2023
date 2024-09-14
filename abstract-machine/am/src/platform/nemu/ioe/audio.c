#include <am.h>
#include <nemu.h>
#include <string.h>

#define AUDIO_FREQ_ADDR (AUDIO_ADDR + 0x00)
#define AUDIO_CHANNELS_ADDR (AUDIO_ADDR + 0x04)
#define AUDIO_SAMPLES_ADDR (AUDIO_ADDR + 0x08)
#define AUDIO_SBUF_SIZE_ADDR (AUDIO_ADDR + 0x0c)
#define AUDIO_INIT_ADDR (AUDIO_ADDR + 0x10)
#define AUDIO_COUNT_ADDR (AUDIO_ADDR + 0x14)

static int sbuf_index = 0; // 以循环存储的方式索引nemu的sbuf

void __am_audio_init() {
}

void __am_audio_config(AM_AUDIO_CONFIG_T *cfg) {
  cfg->present = true;
  cfg->bufsize = inl(AUDIO_SBUF_SIZE_ADDR);
}

void __am_audio_ctrl(AM_AUDIO_CTRL_T *ctrl) {
  outl(AUDIO_FREQ_ADDR, ctrl->freq);
  outl(AUDIO_CHANNELS_ADDR, ctrl->channels);
  outl(AUDIO_SAMPLES_ADDR, ctrl->samples);
  outl(AUDIO_INIT_ADDR, 1);
  if (ctrl->freq == 0 && ctrl->channels == 0 && ctrl->samples == 0) {
    // SDL_CloseAudio
    sbuf_index = 0; // reset index
  }
}

void __am_audio_status(AM_AUDIO_STATUS_T *stat) {
  stat->count = inl(AUDIO_COUNT_ADDR);
}

void __am_audio_play(AM_AUDIO_PLAY_T *ctl) {
  uint32_t sbuf_size = inl(AUDIO_SBUF_SIZE_ADDR);
  int transfer_count = ctl->buf.end - ctl->buf.start; // 需要搬运到SBUF中的长度
  // 等有足够的空位了再复制进去
  while (sbuf_size - inl(AUDIO_COUNT_ADDR) < transfer_count)
    ;

  if (sbuf_index + transfer_count > sbuf_size) {
    int right_part = sbuf_size - sbuf_index;
    int left_part = transfer_count - right_part;
    memcpy((void *)AUDIO_SBUF_ADDR + sbuf_index, ctl->buf.start, right_part);
    memcpy((void *)AUDIO_SBUF_ADDR, ctl->buf.start + right_part, left_part);
    sbuf_index = left_part;
  } else {
    memcpy((void *)AUDIO_SBUF_ADDR + sbuf_index, ctl->buf.start, transfer_count);
    sbuf_index += transfer_count;
  }
  // 复制完后再告知reg_count
  outl(AUDIO_COUNT_ADDR, inl(AUDIO_COUNT_ADDR) + transfer_count);
}
