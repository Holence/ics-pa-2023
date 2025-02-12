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

#ifndef __ISA_RISCV_H__
#define __ISA_RISCV_H__

#include <common.h>

typedef struct {
  word_t gpr[MUXDEF(CONFIG_RVE, 16, 32)];
  word_t csr[4096]; // 添加了这东西后，记得要去difftest中修改diff_context_t的格式，进行统一
  vaddr_t pc;
  bool INTR;
} MUXDEF(CONFIG_RV64, riscv64_CPU_state, riscv32_CPU_state);

#define mstatus 0x300
#define mtvec 0x305
#define mscratch 0x340
#define mepc 0x341
#define mcause 0x342
#define satp 0x180
#define MSTATUS_MIE 0b1000
// #define MSTATUS_MPIE 0b10000000

#define EXP_MECALL 0xb        // mcause of Environment call from M-mode
#define IRQ_MTIMER 0x80000007 // mcause of Machine timer interrupt

// decode
typedef struct {
  union {
    uint32_t val;
  } inst;
} MUXDEF(CONFIG_RV64, riscv64_ISADecodeInfo, riscv32_ISADecodeInfo);

#define isa_mmu_check(vaddr, len, type) (BITS(cpu.csr[satp], 31, 31) == 1 ? MMU_TRANSLATE : MMU_DIRECT)

#endif
