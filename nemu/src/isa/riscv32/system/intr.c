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

void ftrace_log(vaddr_t address, bool jump_in);

word_t isa_raise_intr(word_t cause, vaddr_t epc) {
  /* Trigger an interrupt/exception with ``cause''.
   * Then return the address of the interrupt/exception vector.
   */

#ifdef CONFIG_ETRACE
  _Log("Trap [" FMT_WORD "] : %d\n", epc, cause);
#endif
  cpu.csr[mepc] = epc;
  cpu.csr[mcause] = cause;

  IFDEF(CONFIG_FTRACE, ftrace_log(cpu.csr[mtvec], true));

  return cpu.csr[mtvec];
}

word_t isa_query_intr() {
  return INTR_EMPTY;
}
