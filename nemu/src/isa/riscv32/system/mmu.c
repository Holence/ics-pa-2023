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
  /*
  // 按照手册的实现
  word_t pte;
  paddr_t pte_address;

  // step_1
  paddr_t a = SATP_PPN(cpu.csr[satp]) * PAGE_SIZE; // 一级页表地址
  int i = 1;

step_2:
  if (i == 1) {
    // 一级页表
    pte_address = a + (VPN1(vaddr) << 2);          // 偏移量乘以4（每个PTE都是uint32_t）
    pte = paddr_read(pte_address, sizeof(word_t)); // 从一级页表中读出VPN1对应的PTE
  } else {
    // 二级页表
    pte_address = a + (VPN0(vaddr) << 2);          // 偏移量乘以4（每个PTE都是uint32_t）
    pte = paddr_read(pte_address, sizeof(word_t)); // 从二级页表中读出VPN0对应的PTE
  }

  // step_3
  if (PTE_V_BIT(pte) != 1) {
    panic("pc = " FMT_WORD " vaddr = " FMT_PADDR ", i == %d, pte at " FMT_PADDR " is not valid: " FMT_WORD, cpu.pc, vaddr, i, pte_address, pte);
  }

  if (!PTE_IS_LEAF(pte)) {
    // step_4: PTE is a pointer to the next level of the page table
    i--;
    if (i < 0) {
      panic("pages should not deeper than 2 levels");
    }
    a = PTE_PPN(pte) * PAGE_SIZE; // 二级页表地址
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
  */

  // 由于我的nanos中一定是两级页表（不创建superpage），所以翻译的过程可以简化
  word_t pte;
  paddr_t pte_address;
  // 一级页表
  paddr_t a = SATP_PPN(cpu.csr[satp]) * PAGE_SIZE; // 一级页表地址
  pte_address = a + (VPN1(vaddr) << 2);            // 偏移量乘以4（每个PTE都是uint32_t）
  pte = paddr_read(pte_address, sizeof(word_t));   // 从一级页表中读出VPN1对应的PTE
  if (PTE_V_BIT(pte) != 1) {
    panic("Level 1 Type = %d pc = " FMT_WORD " vaddr = " FMT_PADDR ", pte at " FMT_PADDR " is not valid: " FMT_WORD, type, cpu.pc, vaddr, pte_address, pte);
  }

  // 二级页表
  a = PTE_PPN(pte) * PAGE_SIZE;                  // 二级页表地址
  pte_address = a + (VPN0(vaddr) << 2);          // 偏移量乘以4（每个PTE都是uint32_t）
  pte = paddr_read(pte_address, sizeof(word_t)); // 从二级页表中读出VPN0对应的PTE
  if (PTE_V_BIT(pte) != 1) {
    panic("Level 2 Type = %d pc = " FMT_WORD " vaddr = " FMT_PADDR ", pte at " FMT_PADDR " is not valid: " FMT_WORD, type, cpu.pc, vaddr, pte_address, pte);
  }
  paddr_t paddr = PAGE_OFFSET(vaddr);
  paddr = (PTE_PPN(pte) << 12) | paddr;

  return paddr;
}
