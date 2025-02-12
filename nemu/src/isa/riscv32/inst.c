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

void ftrace_log(vaddr_t address, bool jump_in);

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

static int rs1, rs2;
static int rd = 0;
static word_t src1 = 0, src2 = 0, imm = 0;
#define SHAMT BITS(imm, 4, 0)

/*
  根据TYPE
  解析寄存器rs1和rs2，读出值，存到src1和src2中
  解析立即数的值，存到imm中
  解析rd
*/
static void decode_operand(Decode *s, int type) {
  uint32_t i = s->isa.inst.val;

  rs1 = BITS(i, 19, 15);
  rs2 = BITS(i, 24, 20);
  src1 = gpr(rs1);
  src2 = gpr(rs2);
  rd = BITS(i, 11, 7);

  switch (type) {
  case TYPE_I:
    imm = BITS(i, 31, 20);
    imm = SIGN_EXTEND(imm, 12);
    break;
  case TYPE_S:
    imm = (BITS(i, 31, 25) << 5) | BITS(i, 11, 7);
    imm = SIGN_EXTEND(imm, 12);
    break;
  case TYPE_B:
    imm = (BITS(i, 31, 31) << 12) | (BITS(i, 7, 7) << 11) | (BITS(i, 30, 25) << 5) | (BITS(i, 11, 8) << 1) | 0;
    imm = SIGN_EXTEND(imm, 13);
    break;
  case TYPE_U:
    imm = BITS(i, 31, 12) << 12;
    break;
  case TYPE_J:
    imm = (BITS(i, 31, 31) << 20) | (BITS(i, 19, 12) << 12) | (BITS(i, 20, 20) << 11) | (BITS(i, 30, 21) << 1) | 0;
    imm = SIGN_EXTEND(imm, 21);
    break;
  }
}

#ifdef CONFIG_INST_STATISTIC
static word_t inst_statistic[64] = {0};

// 打印指令频度
void inst_statistic() {
  printf("----------INST STATISTIC----------\n");
  printf("NO COUNT\n");
  for (int i = 1; i <= 51; i++) {
    printf("%02d %015ld\n", i, inst_statistic[i]);
  }
  printf("----------------------------------\n");
}
#endif

static int decode_exec(Decode *s) {
  // s是记录当前指令的结构体
  // s->pc是当前指令的地址
  // s->snpc在isa_exec_once()中被inst_fetch()加了4
  // 之后return出去了会用s->dnpc更新cpu的pc，也就是说需要正确修改s->dnpc的值
  s->dnpc = s->snpc; // 这里先默认为+4,在branch/jump指令中需要正确修改s->dnpc的值

#define INSTPAT_INST(s) ((s)->isa.inst.val)

#ifndef CONFIG_INST_STATISTIC
#define INSTPAT_MATCH(s, useless_op_name, type, EXECUTE_EXPR) \
  {                                                           \
    decode_operand(s, concat(TYPE_, type));                   \
    EXECUTE_EXPR;                                             \
  }
#else
#define INSTPAT_MATCH(s, index, useless_op_name, type, EXECUTE_EXPR) \
  {                                                                  \
    decode_operand(s, concat(TYPE_, type));                          \
    inst_statistic[index] += 1;                                      \
    EXECUTE_EXPR;                                                    \
  }

  int inst_statistic_index = 0;
#endif

  INSTPAT_START();

  // INSTPAT(模式字符串, 指令名称, 指令类型, 指令执行操作);
  // 指令名称不会被使用，只作为注释

  // INSTPAT的顺序已经过按照inst_statistic的结果进行优化，正常排序的版本见git的历史

  INSTPAT("???????????? ????? 000 ????? 0010011", addi, I, gpr(rd) = src1 + imm);
  INSTPAT("???????????? ????? 100 ????? 0000011", lbu, I, gpr(rd) = Mr(src1 + imm, 1));
  INSTPAT("???????????? ????? 010 ????? 0000011", lw, I, gpr(rd) = SIGN_EXTEND(Mr(src1 + imm, 4), 32));
  INSTPAT("??????? ????? ????? 001 ????? 1100011", bne, B, if (src1 != src2) s->dnpc = s->pc + imm);
  INSTPAT("??????? ????? ????? 000 ????? 0100011", sb, S, Mw(src1 + imm, 1, src2));
  INSTPAT("??????? ????? ????? 010 ????? 0100011", sw, S, Mw(src1 + imm, 4, src2));
  INSTPAT("0000000????? ????? 001 ????? 0010011", slli, I, gpr(rd) = src1 << SHAMT);
  INSTPAT("0000000 ????? ????? 000 ????? 0110011", add, R, gpr(rd) = src1 + src2);
  INSTPAT("0000000 ????? ????? 110 ????? 0110011", or, R, gpr(rd) = src1 | src2);
  INSTPAT("0100000 ????? ????? 000 ????? 0110011", sub, R, gpr(rd) = src1 - src2);
  INSTPAT("??????? ????? ????? 110 ????? 1100011", bltu, B, if (src1 < src2) s->dnpc = s->pc + imm); // unsigned compare
  INSTPAT("???????????? ????? 000 ????? 1100111", jalr, I, gpr(rd) = s->pc + 4; s->dnpc = src1 + imm;
#ifdef CONFIG_FTRACE
          // Call: jalr ra, rs1, imm (rd==ra)
          if (rd == 1) { ftrace_log(s->dnpc, true); } // 传入的是跳进哪儿的地址
          // Return: ret == jalr x0, ra, 0 (rs1==ra)
          if (rs1 == 1 && imm == 0) { ftrace_log(s->pc, false); } // 传入的是从哪儿返回的地址
#endif
  );
  INSTPAT("??????? ????? ????? 000 ????? 1100011", beq, B, if (src1 == src2) s->dnpc = s->pc + imm);
  INSTPAT("???????????? ????? 111 ????? 0010011", andi, I, gpr(rd) = src1 & imm);
  INSTPAT("0000000????? ????? 101 ????? 0010011", srli, I, gpr(rd) = src1 >> SHAMT);
  INSTPAT("???????????????????? ????? 1101111", jal, J, gpr(rd) = s->pc + 4; s->dnpc = s->pc + imm;
#ifdef CONFIG_FTRACE
          // Call: jal ra, imm (rd==ra)
          if (rd == 1) { ftrace_log(s->dnpc, true); } // 传入的是跳进哪儿的地址
#endif
  );
  INSTPAT("???????????????????? ????? 0010111", auipc, U, gpr(rd) = s->pc + imm);
  INSTPAT("???????????? ????? 001 ????? 0000011", lh, I, gpr(rd) = SIGN_EXTEND(Mr(src1 + imm, 2), 16));
  INSTPAT("0100000????? ????? 101 ????? 0010011", srai, I, gpr(rd) = (int32_t)src1 < 0 ? ~(~src1 >> SHAMT) : src1 >> SHAMT);        // shift right arithmetic
  INSTPAT("0000001 ????? ????? 000 ????? 0110011", mul, R, gpr(rd) = BITS((int64_t)(int32_t)src1 * (int64_t)(int32_t)src2, 31, 0)); // signed * signed [31:0]
  INSTPAT("???????????? ????? 101 ????? 0000011", lbh, I, gpr(rd) = Mr(src1 + imm, 2));
  INSTPAT("0000000 ????? ????? 111 ????? 0110011", and, R, gpr(rd) = src1 & src2);
  INSTPAT("??????? ????? ????? 100 ????? 1100011", blt, B, if ((int32_t)src1 < (int32_t)src2) s->dnpc = s->pc + imm); // signed compare
  INSTPAT("0000001 ????? ????? 110 ????? 0110011", rem, R, gpr(rd) = (int32_t)src1 % (int32_t)src2);                  // signed % signed
  INSTPAT("???????????? ????? 001 ????? 1110011", csrrw, I, {
    if (rd != 0) {
      gpr(rd) = csr(imm);
    }
    csr(imm) = src1;
  });
  INSTPAT("??????? ????? ????? 111 ????? 1100011", bgeu, B, if (src1 >= src2) s->dnpc = s->pc + imm); // unsigned compare
  INSTPAT("???????????? ????? 010 ????? 1110011", csrrs, I, {
    gpr(rd) = csr(imm);
    if (rs1 != 0) {
      csr(imm) = csr(imm) | src1;
    }
  });
  INSTPAT("???????????????????? ????? 0110111", lui, U, gpr(rd) = imm);
  INSTPAT("0000000 ????? ????? 101 ????? 0110011", srl, R, gpr(rd) = src1 >> src2);
  INSTPAT("??????? ????? ????? 101 ????? 1100011", bge, B, if ((int32_t)src1 >= (int32_t)src2) s->dnpc = s->pc + imm); // signed compare
  INSTPAT("??????? ????? ????? 001 ????? 0100011", sh, S, Mw(src1 + imm, 2, src2));
  INSTPAT("0000000 ????? ????? 001 ????? 0110011", sll, R, gpr(rd) = src1 << src2);
  INSTPAT("0100000 ????? ????? 101 ????? 0110011", sra, R, gpr(rd) = (int32_t)src1 < 0 ? ~(~src1 >> src2) : src1 >> src2); // shift right arithmetic
  INSTPAT("0000000 ????? ????? 011 ????? 0110011", sltu, R, gpr(rd) = (src1 < src2) ? 1 : 0);                              // set less than (unsigned)
  INSTPAT("001100000010 00000 000 00000 1110011", mret, N, {
    s->dnpc = csr(mepc);
    // MIE置1,开中断
    cpu.csr[mstatus] = cpu.csr[mstatus] | MSTATUS_MIE;
    IFDEF(CONFIG_FTRACE, ftrace_log(s->pc, false));
  });
  INSTPAT("000000000000 00000 000 00000 1110011", ecall, N, s->dnpc = isa_raise_intr(EXP_MECALL, s->pc));      // mcause = 11 (Environment call from M-mode)
  INSTPAT("0000001 ????? ????? 100 ????? 0110011", div, R, gpr(rd) = (int32_t)src1 / (int32_t)src2);           // signed / signed
  INSTPAT("0000001 ????? ????? 101 ????? 0110011", divu, R, gpr(rd) = src1 / src2);                            // unsigned / unsigned
  INSTPAT("0000000 ????? ????? 010 ????? 0110011", slt, R, gpr(rd) = ((int32_t)src1 < (int32_t)src2) ? 1 : 0); // set less than (signed)
  INSTPAT("???????????? ????? 000 ????? 0000011", lb, I, gpr(rd) = SIGN_EXTEND(Mr(src1 + imm, 1), 8));
  INSTPAT("0000001 ????? ????? 111 ????? 0110011", remu, R, gpr(rd) = src1 % src2); // unsigned % unsigned
  INSTPAT("???????????? ????? 100 ????? 0010011", xori, I, gpr(rd) = src1 ^ imm);
  INSTPAT("???????????? ????? 110 ????? 0010011", ori, I, gpr(rd) = src1 | imm);
  INSTPAT("0000000 ????? ????? 100 ????? 0110011", xor, R, gpr(rd) = src1 ^ src2);
  INSTPAT("???????????? ????? 011 ????? 0010011", sltiu, I, gpr(rd) = (src1 < imm) ? 1 : 0);                                          // set less than (unsigned)
  INSTPAT("0000001 ????? ????? 011 ????? 0110011", mulhu, R, gpr(rd) = BITS((uint64_t)src1 * (uint64_t)src2, 63, 32));                // unsigned * unsigned [63:32]
  INSTPAT("???????????? ????? 010 ????? 0010011", slti, I, gpr(rd) = ((int32_t)src1 < (int32_t)imm) ? 1 : 0);                         // set less than (signed)
  INSTPAT("0000001 ????? ????? 010 ????? 0110011", mulhsu, R, gpr(rd) = BITS((int64_t)(int32_t)src1 * (uint64_t)src2, 63, 32));       // signed * unsigned [63:32]
  INSTPAT("0000001 ????? ????? 001 ????? 0110011", mulh, R, gpr(rd) = BITS((int64_t)(int32_t)src1 * (int64_t)(int32_t)src2, 63, 32)); // signed * signed [63:32]
  INSTPAT("000000000001 00000 000 00000 1110011", ebreak, N, NEMUTRAP(s->pc, gpr(10)));                                               // gpr(10) is $a0
  INSTPAT("??????? ????? ????? ??? ????? ???????", inv, N, INV(s->pc));

  INSTPAT_END();

  gpr(0) = 0; // reset $zero to 0
  return 0;
}

int isa_exec_once(Decode *s) {
  s->isa.inst.val = inst_fetch(&s->snpc, 4); // 取指令，s->snpc+=4
  return decode_exec(s);
}
