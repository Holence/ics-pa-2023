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

#include <device/map.h>
#include <memory/paddr.h>

#define NR_MAP 16

static IOMap maps[NR_MAP] = {};
static int nr_map = 0;

// 寻找虚拟地址addr对应的IOMap
static IOMap *fetch_mmio_map(paddr_t addr) {
  int mapid = find_mapid_by_addr(maps, nr_map, addr);
  return (mapid == -1 ? NULL : &maps[mapid]);
}

static void report_mmio_overlap(const char *name1, paddr_t l1, paddr_t r1,
                                const char *name2, paddr_t l2, paddr_t r2) {
  panic("MMIO region %s@[" FMT_PADDR ", " FMT_PADDR "] is overlapped "
        "with %s@[" FMT_PADDR ", " FMT_PADDR "]",
        name1, l1, r1, name2, l2, r2);
}

/* device interface */
void add_mmio_map(const char *name, paddr_t addr, void *space, uint32_t len, io_callback_t callback) {
  assert(nr_map < NR_MAP);
  // 规划 mmio地址 在 nemu 地址中的分布
  // 需要几个地址就给几个
  paddr_t left = addr, right = addr + len - 1;

  // 检查地址合法性（防止有人篡改Kconfig中的默认值）
  // 不能落在普通内存的范围内 (addr < 0x8800 0000 == false)
  // 设备的默认地址都是大于0xa0000000的
  if (in_pmem(left) || in_pmem(right)) {
    report_mmio_overlap(name, left, right, "pmem", PMEM_LEFT, PMEM_RIGHT);
  }
  // 设备的地址不能重叠
  for (int i = 0; i < nr_map; i++) {
    if (left <= maps[i].high && right >= maps[i].low) {
      report_mmio_overlap(name, left, right, maps[i].name, maps[i].low, maps[i].high);
    }
  }

  maps[nr_map] = (IOMap){.name = name, .low = addr, .high = addr + len - 1, .space = space, .callback = callback};
  Log("Add mmio map '%s' at [" FMT_PADDR ", " FMT_PADDR "]",
      maps[nr_map].name, maps[nr_map].low, maps[nr_map].high);

  nr_map++;
}

/* bus interface */
word_t mmio_read(paddr_t addr, int len) {
  return map_read(addr, len, fetch_mmio_map(addr));
}

void mmio_write(paddr_t addr, int len, word_t data) {
  map_write(addr, len, data, fetch_mmio_map(addr));
}
