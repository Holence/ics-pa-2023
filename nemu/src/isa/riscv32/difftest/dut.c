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

#include "../local-include/reg.h"
#include <cpu/difftest.h>
#include <isa.h>

bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t inst_pc) {
  bool equals = true;
  if (ref_r->pc != cpu.pc) {
    equals = false;
    printf(ANSI_FMT("nemu's next pc: " FMT_WORD "\n", ANSI_FG_RED), cpu.pc);
    printf(ANSI_FMT("ref's next pc: " FMT_WORD "\n", ANSI_FG_RED), ref_r->pc);
  }
  for (int i = 0; i < ARRLEN(cpu.gpr); i++) {
    if (cpu.gpr[i] != ref_r->gpr[i]) {
      equals = false;
      printf(ANSI_FMT("nemu's %s: " FMT_WORD "\n", ANSI_FG_RED), reg_name(i), cpu.gpr[i]);
      printf(ANSI_FMT("ref's %s: " FMT_WORD "\n", ANSI_FG_RED), reg_name(i), ref_r->gpr[i]);
    }
  }
  if (!equals) {
    printf(ANSI_FMT("State Mismatch!!!\nCurrent Instruction PC: " FMT_WORD "\n", ANSI_FG_RED), inst_pc);
  }
  return equals;
}

void isa_difftest_attach() {
}
