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

#include <device/alarm.h>
#include <device/map.h>
#include <utils.h>

static uint32_t *rtc_port_base = NULL;

// 两个32位的数，组成一个64位的数，总共8个字节
// CONFIG_RTC_MMIO:   {31-0}
// CONFIG_RTC_MMIO+4: {63-32}
static void rtc_io_handler(uint32_t offset, int len, bool is_write) {
  assert(offset == 0 || offset == 4);
  // 读低32位的时候，记录64位的时间到堆上，并在外面host_read读出低32位
  // /abstract-machine/am/src/platform/nemu/ioe/timer.c中也要保证先读低32位，后读高32位
  if (!is_write && offset == 0) {
    uint64_t us = get_time();
    rtc_port_base[0] = (uint32_t)us;
    rtc_port_base[1] = us >> 32;
  }
  // 读高32位的时候，这里什么都不做，仅仅只是让host_read去读出高32位
}

#ifndef CONFIG_TARGET_AM
static void timer_intr() {
  if (nemu_state.state == NEMU_RUNNING) {
    extern void dev_raise_intr();
    dev_raise_intr();
  }
}
#endif

void init_timer() {
  rtc_port_base = (uint32_t *)new_space(8);
#ifdef CONFIG_HAS_PORT_IO
  add_pio_map("rtc", CONFIG_RTC_PORT, rtc_port_base, 8, rtc_io_handler);
#else
  add_mmio_map("rtc", CONFIG_RTC_MMIO, rtc_port_base, 8, rtc_io_handler);
#endif
  IFNDEF(CONFIG_TARGET_AM, add_alarm_handle(timer_intr));
}
