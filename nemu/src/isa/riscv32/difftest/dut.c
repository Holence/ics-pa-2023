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
  if (difftest_check_reg("pc", inst_pc, ref_r->pc, cpu.pc) == false) {
    equals = false;
  }
  for (int i = 0; i < ARRLEN(cpu.gpr); i++) {
    if (difftest_check_reg(reg_name(i), inst_pc, ref_r->gpr[i], cpu.gpr[i]) == false) {
      equals = false;
    }
  }
  return equals;
}

void isa_difftest_attach() {
}
