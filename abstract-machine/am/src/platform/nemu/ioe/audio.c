#include <am.h>
#include <nemu.h>
#include <string.h>

#define AUDIO_FREQ_ADDR (AUDIO_ADDR + 0x00)
#define AUDIO_CHANNELS_ADDR (AUDIO_ADDR + 0x04)
#define AUDIO_SAMPLES_ADDR (AUDIO_ADDR + 0x08)
#define AUDIO_SBUF_SIZE_ADDR (AUDIO_ADDR + 0x0c)
#define AUDIO_INIT_ADDR (AUDIO_ADDR + 0x10)
#define AUDIO_COUNT_ADDR (AUDIO_ADDR + 0x14)

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
}

void __am_audio_status(AM_AUDIO_STATUS_T *stat) {
  stat->count = inl(AUDIO_COUNT_ADDR);
}

static int sbuf_index = 0; // 以循环存储的方式用sbuf
void __am_audio_play(AM_AUDIO_PLAY_T *ctl) {
  uint8_t *buf_ptr = ctl->buf.start;
  uint32_t sbuf_size = inl(AUDIO_SBUF_SIZE_ADDR);
  int transfer_count = ctl->buf.end - ctl->buf.start; // 需要搬运到SBUF中的长度

  for (int i = 0; i < transfer_count; i++) {

    while (inl(AUDIO_COUNT_ADDR) == sbuf_size) {
      // 搬运的过程中，如果SBUF满了，则等待播放
      // 播放完一部分后会触发add_more_data，把SBUF输出一部分
    }

    outb(AUDIO_SBUF_ADDR + sbuf_index, buf_ptr[i]);
    sbuf_index = (sbuf_index + 1) % sbuf_size;

    // 每复制一个byte就改一次reg_count，也行吧，但没必要
    // outl(AUDIO_COUNT_ADDR, inl(AUDIO_COUNT_ADDR) + 1);
  }
  // 复制完后再告知reg_count，也不迟
  outl(AUDIO_COUNT_ADDR, inl(AUDIO_COUNT_ADDR) + transfer_count);
}
