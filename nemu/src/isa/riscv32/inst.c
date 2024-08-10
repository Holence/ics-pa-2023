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

#include "local-include/reg.h"
#include <cpu/cpu.h>
#include <cpu/decode.h>
#include <cpu/ifetch.h>

#define R(i) gpr(i)
#define Mr vaddr_read
#define Mw vaddr_write

enum {
  TYPE_R,
  TYPE_I,
  TYPE_S,
  TYPE_B,
  TYPE_U,
  TYPE_J,
  TYPE_N, // none
};

/*
  根据TYPE
  解析寄存器rs1和rs2，读出值，存到src1和src2中
  解析立即数的值，存到imm中
  解析rd
*/
static void decode_operand(Decode *s, int *rd, word_t *src1, word_t *src2, word_t *imm, int type) {
  uint32_t i = s->isa.inst.val;

  int rs1 = BITS(i, 19, 15);
  int rs2 = BITS(i, 24, 20);
  *src1 = R(rs1);
  *src2 = R(rs2);
  *rd = BITS(i, 11, 7);

  switch (type) {
  case TYPE_I:
    *imm = SIGNED_EXTEND(BITS(i, 31, 20), 12);
    break;
  case TYPE_S:
    *imm = (SIGNED_EXTEND(BITS(i, 31, 25), 7) << 5) | BITS(i, 11, 7);
    break;
  case TYPE_B:
    //
    break;
  case TYPE_U:
    *imm = SIGNED_EXTEND(BITS(i, 31, 12), 20) << 12;
    break;
  case TYPE_J:
    *imm = (BITS(i, 31, 31) << 20) | (BITS(i, 19, 12) << 12) | (BITS(i, 20, 20) << 11) | (BITS(i, 30, 21) << 1) | 0;
    break;
  }
}

static int decode_exec(Decode *s) {
  int rd = 0;
  word_t src1 = 0, src2 = 0, imm = 0;

  // s是记录当前指令的结构体
  // s->pc是当前指令的地址
  // s->snpc在isa_exec_once()中被inst_fetch()加了4
  // 之后return出去了会用s->dnpc更新cpu的pc，也就是说需要正确修改s->dnpc的值
  s->dnpc = s->snpc; // 这里先默认为+4,在branch/jump指令中需要正确修改s->dnpc的值

#define INSTPAT_INST(s) ((s)->isa.inst.val)
#define INSTPAT_MATCH(s, useless_op_name, type, EXECUTE_EXPR)        \
  {                                                                  \
    decode_operand(s, &rd, &src1, &src2, &imm, concat(TYPE_, type)); \
    EXECUTE_EXPR;                                                    \
  }

  INSTPAT_START();

  // INSTPAT(模式字符串, 指令名称, 指令类型, 指令执行操作);
  // 指令名称不会被使用，只作为注释
  INSTPAT("??????? ????? ????? 000 ????? 00100 11", addi, I, R(rd) = src1 + imm);

  INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu, I, R(rd) = Mr(src1 + imm, 1));

  INSTPAT("??????? ????? ????? 000 ????? 01000 11", sb, S, Mw(src1 + imm, 1, src2));

  INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc, U, R(rd) = s->pc + imm);

  INSTPAT("??????? ????? ????? ??? ????? 11011 11", jal, J, R(rd) = s->pc + 4; s->dnpc = s->pc + imm);

  INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak, N, NEMUTRAP(s->pc, R(10))); // R(10) is $a0

  INSTPAT("??????? ????? ????? ??? ????? ????? ??", inv, N, INV(s->pc));

  INSTPAT_END();

  R(0) = 0; // reset $zero to 0

  return 0;
}

int isa_exec_once(Decode *s) {
  s->isa.inst.val = inst_fetch(&s->snpc, 4); // 取指令，s->snpc+=4
  return decode_exec(s);
}
