/***************************************************************************************
 * Copyright (c) 2014-2022 Zihao Yu, Nanjing University
 *
 * NEMU is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 *
 * See the Mulan PSL v2 for more details.
 ***************************************************************************************/

#include <SDL2/SDL.h>
#include <common.h>
#include <device/map.h>

enum {
  reg_freq,      // 只写
  reg_channels,  // 只写
  reg_samples,   // 只写
  reg_sbuf_size, // 只读
  reg_init,      // 只写 AM中写入reg_freq、reg_channels、reg_samples后，再写入reg_init，表示可以开始SDL的init
  reg_count,     // 读/写 buf中还剩多少
  nr_reg         // 不是寄存器，只是表示总共6个寄存器
};

static uint8_t *sbuf = NULL;
static uint32_t *audio_base = NULL;

static int sbuf_index = 0; // 以循环存储的方式用sbuf

static void add_more_data(void *userdata, Uint8 *stream, int len) {
  // stream是SDL Audio内部，这里要把sbuf中可用的部分，搬运到stream里
  // stream的长度为len，好像只有2048
  // sbuf中可用的长度在reg_count中
  memset(stream, 0, len);
  int sbuf_count = audio_base[reg_count];
  int transfer_count = len < sbuf_count ? len : sbuf_count;
  // method 1
  // for (int i = 0; i < transfer_count; i++) {
  //   // if sbuf has not used data
  //   *stream = sbuf[sbuf_index];
  //   stream++;
  //   sbuf_index = (sbuf_index + 1) % 0x10000;
  // }
  // audio_base[reg_count] -= transfer_count;

  // method 2
  if (sbuf_index + transfer_count > CONFIG_SB_SIZE) {
    int right_part = CONFIG_SB_SIZE - sbuf_index;
    SDL_MixAudio(stream, sbuf + sbuf_index, right_part, SDL_MIX_MAXVOLUME);
    SDL_MixAudio(stream + right_part, sbuf, transfer_count - right_part, SDL_MIX_MAXVOLUME);
  } else {
    SDL_MixAudio(stream, sbuf + sbuf_index, transfer_count, SDL_MIX_MAXVOLUME);
  }
  sbuf_index = (sbuf_index + transfer_count) % CONFIG_SB_SIZE;
  audio_base[reg_count] -= transfer_count;
}

static void audio_io_handler(uint32_t offset, int len, bool is_write) {
  // 只有写reg_init时才有动作
  if (offset == reg_init * 4) {
    // 只写
    assert(is_write);
    SDL_AudioSpec s = {};
    s.format = AUDIO_S16SYS; // 假设系统中音频数据的格式总是使用16位有符号数来表示
    s.userdata = NULL;       // 不使用
    s.freq = (int)audio_base[reg_freq];
    s.channels = (Uint8)audio_base[reg_channels];
    s.samples = (Uint16)audio_base[reg_samples];
    s.callback = add_more_data;

    SDL_InitSubSystem(SDL_INIT_AUDIO);
    SDL_OpenAudio(&s, NULL);
    SDL_PauseAudio(0);
  }
}

void init_audio() {
  // 6个寄存器，24个字节
  uint32_t space_size = sizeof(uint32_t) * nr_reg;

  audio_base = (uint32_t *)new_space(space_size);
  audio_base[reg_sbuf_size] = CONFIG_SB_SIZE; // 64KB
  audio_base[reg_count] = 0;
#ifdef CONFIG_HAS_PORT_IO
  add_pio_map("audio", CONFIG_AUDIO_CTL_PORT, audio_base, space_size, audio_io_handler);
#else
  add_mmio_map("audio", CONFIG_AUDIO_CTL_MMIO, audio_base, space_size, audio_io_handler);
#endif

  // 64KB的buffer
  sbuf = (uint8_t *)new_space(CONFIG_SB_SIZE);
  add_mmio_map("audio-sbuf", CONFIG_SB_ADDR, sbuf, CONFIG_SB_SIZE, NULL);
}
