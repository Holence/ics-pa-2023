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

#include <isa.h>
#include <memory/paddr.h>
#include <memory/vaddr.h>

#define VPN1(x) BITS(x, 31, 22)
#define VPN0(x) BITS(x, 21, 12)
#define PAGE_OFFSET(x) BITS(x, 11, 0)
#define PTE_PPN1(x) BITS(x, 31, 20)
#define PTE_PPN0(x) BITS(x, 19, 10)
#define PTE_PPN(x) BITS(x, 31, 10)
#define PTE_V_BIT(x) BITS(x, 0, 0)
#define SATP_PPN(x) BITS(x, 21, 0)
#define PTE_IS_LEAF(x) (BITS(x, 3, 1) != 0x000) // RWX不全0,则是leaf PTE

// Sv32分页机制，将vaddr转换为paddr
// len对riscv没用，目前type不考虑
// 参考 The RISC-V Instruction Set Manual: Volume II Privileged Architecture - 11.3.2. Virtual Address Translation Process
paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type) {
  word_t pte;
  paddr_t pte_address;

  // step_1
  paddr_t a = SATP_PPN(cpu.csr[satp]) * PAGE_SIZE; // 一级页表地址
  int i = 1;

step_2:
  if (i == 1) {
    pte_address = a + (VPN1(vaddr) << 2);          // 偏移量乘以4（每个PTE都是uint32_t）
    pte = paddr_read(pte_address, sizeof(word_t)); // 从一级页表中读出VPN1对应的PTE
  } else {
    pte_address = a + (VPN0(vaddr) << 2);          // 偏移量乘以4（每个PTE都是uint32_t）
    pte = paddr_read(pte_address, sizeof(word_t)); // 从二级页表中读出VPN0对应的PTE
  }

  // step_3
  if (PTE_V_BIT(pte) != 1) {
    panic("pte at " FMT_PADDR " is not valid: " FMT_WORD, pte_address, pte);
  }

  if (!PTE_IS_LEAF(pte)) {
    // step_4: PTE is a pointer to the next level of the page table
    i--;
    if (i < 0) {
      panic("pages should not deeper than 2 levels");
    }
    a = PTE_PPN(pte) * PAGE_SIZE; // 二级页表
    goto step_2;
  }

  // step_5: skipped
  // step_6: check 4MB superpage is aligned
  if (i == 1 && PTE_PPN0(pte) != 0) {
    panic("pte at " FMT_PADDR " is 4MB megapages, but is not aligned: " FMT_WORD, pte_address, pte);
  }
  // step_7: skipped

  // step_8: The translation is successful
  paddr_t paddr = PAGE_OFFSET(vaddr);
  if (i == 1) {
    // superpage translation
    // pa.ppn[i-1:0] = va.vpn[i-1:0]
    paddr = VPN0(vaddr) | paddr;
    // pa.ppn[LEVELS-1:i] = pte.ppn[LEVELS-1:i]
    paddr = (PTE_PPN1(pte) << 12) | paddr;
  } else {
    // pa.ppn[LEVELS-1:i] = pte.ppn[LEVELS-1:i]
    paddr = (PTE_PPN(pte) << 12) | paddr;
  }

  // 目前全部都在内核空间，恒等映射
  if (paddr != vaddr) {
    panic("paddr != vaddr : %x != %x", paddr, vaddr);
  }

  return paddr;
}
