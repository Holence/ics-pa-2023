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

#ifndef __CPU_DECODE_H__
#define __CPU_DECODE_H__

#include <isa.h>

typedef struct Decode {
  vaddr_t pc;
  vaddr_t snpc; // static next pc
  vaddr_t dnpc; // dynamic next pc
  ISADecodeInfo isa;
  IFDEF(CONFIG_ITRACE, char logbuf[128]);
} Decode;

// --- pattern matching mechanism ---
__attribute__((always_inline)) static inline void pattern_decode(const char *str, int len,
                                                                 uint64_t *key, uint64_t *mask, uint64_t *shift) {
  /*
    "??????? ????? ????? ??? ????? 00101 11"
    key   = 0x17 = 001 0111; 所有的1保留为1,所有的?和0变成0
    mask  = 0x7f = 111 1111; 所有的0和1保留为1,所以的?变成0
    shift = 0; 最右边的?个数，如果shift>0，那么key和mask都要右移shift
  */

  uint64_t __key = 0, __mask = 0, __shift = 0;

  // 写循环的话速度会慢40倍以上？！！
  // 宏展开后判断更少，jump更少
  // for (int i = 0; i < 64; i++) {
  //   if (i >= len) {
  //     break;
  //   } else {
  //     char c = str[i];
  //     if (c != ' ') {
  //       Assert(c == '0' || c == '1' || c == '?',
  //              "invalid character '%c' in pattern string", c);
  //       __key = (__key << 1) | (c == '1' ? 1 : 0);
  //       __mask = (__mask << 1) | (c == '?' ? 0 : 1);
  //       __shift = (c == '?' ? __shift + 1 : 0);
  //     }
  //   }
  // }
  // if (64 <= len) {
  //   panic("pattern too long");
  // }
  // *key = __key >> __shift;
  // *mask = __mask >> __shift;
  // *shift = __shift;

#define macro(i)                                             \
  if ((i) >= len)                                            \
    goto finish;                                             \
  else {                                                     \
    char c = str[i];                                         \
    if (c != ' ') {                                          \
      Assert(c == '0' || c == '1' || c == '?',               \
             "invalid character '%c' in pattern string", c); \
      __key = (__key << 1) | (c == '1' ? 1 : 0);             \
      __mask = (__mask << 1) | (c == '?' ? 0 : 1);           \
      __shift = (c == '?' ? __shift + 1 : 0);                \
    }                                                        \
  }

#define macro2(i) \
  macro(i);       \
  macro((i) + 1)
#define macro4(i) \
  macro2(i);      \
  macro2((i) + 2)
#define macro8(i) \
  macro4(i);      \
  macro4((i) + 4)
#define macro16(i) \
  macro8(i);       \
  macro8((i) + 8)
#define macro32(i) \
  macro16(i);      \
  macro16((i) + 16)
#define macro64(i) \
  macro32(i);      \
  macro32((i) + 32)
  macro64(0);
  panic("pattern too long");
#undef macro
finish:
  *key = __key >> __shift;
  *mask = __mask >> __shift;
  *shift = __shift;
}

__attribute__((always_inline)) static inline void pattern_decode_hex(const char *str, int len,
                                                                     uint64_t *key, uint64_t *mask, uint64_t *shift) {
  uint64_t __key = 0, __mask = 0, __shift = 0;

  // 写循环的话速度会慢40倍以上？！！
  // 宏展开后判断更少，jump更少
  // for (int i = 0; i < 16; i++) {
  //   if (i >= len) {
  //     break;
  //   } else {
  //     char c = str[i];
  //     if (c != ' ') {
  //       Assert((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || c == '?',
  //              "invalid character '%c' in pattern string", c);
  //       __key = (__key << 4) | (c == '?' ? 0 : (c >= '0' && c <= '9') ? c - '0'
  //                                                                     : c - 'a' + 10);
  //       __mask = (__mask << 4) | (c == '?' ? 0 : 0xf);
  //       __shift = (c == '?' ? __shift + 4 : 0);
  //     }
  //   }
  // }
  // if (16 <= len) {
  //   panic("pattern too long");
  // }
  // *key = __key >> __shift;
  // *mask = __mask >> __shift;
  // *shift = __shift;

#define macro(i)                                                                     \
  if ((i) >= len)                                                                    \
    goto finish;                                                                     \
  else {                                                                             \
    char c = str[i];                                                                 \
    if (c != ' ') {                                                                  \
      Assert((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || c == '?',           \
             "invalid character '%c' in pattern string", c);                         \
      __key = (__key << 4) | (c == '?' ? 0 : (c >= '0' && c <= '9') ? c - '0'        \
                                                                    : c - 'a' + 10); \
      __mask = (__mask << 4) | (c == '?' ? 0 : 0xf);                                 \
      __shift = (c == '?' ? __shift + 4 : 0);                                        \
    }                                                                                \
  }

  macro16(0);
  panic("pattern too long");
#undef macro
finish:
  *key = __key >> __shift;
  *mask = __mask >> __shift;
  *shift = __shift;
}

// --- pattern matching wrappers for decode ---
#ifndef CONFIG_INST_STATISTIC
#define INSTPAT(pattern, ...)                                      \
  do {                                                             \
    uint64_t key, mask, shift;                                     \
    pattern_decode(pattern, STRLEN(pattern), &key, &mask, &shift); \
    if ((((uint64_t)INSTPAT_INST(s) >> shift) & mask) == key) {    \
      INSTPAT_MATCH(s, ##__VA_ARGS__);                             \
      goto *(destination);                                         \
    }                                                              \
  } while (0)
#else
#define INSTPAT(pattern, ...)                                      \
  do {                                                             \
    uint64_t key, mask, shift;                                     \
    pattern_decode(pattern, STRLEN(pattern), &key, &mask, &shift); \
    inst_statistic_index++;                                        \
    if ((((uint64_t)INSTPAT_INST(s) >> shift) & mask) == key) {    \
      INSTPAT_MATCH(s, inst_statistic_index, ##__VA_ARGS__);       \
      goto *(destination);                                         \
    }                                                              \
  } while (0)
#endif

#define INSTPAT_START(name) \
  {                         \
    const void **destination = &&concat(destination_label_, name);
#define INSTPAT_END(name)             \
  concat(destination_label_, name) :; \
  }

// { const void **destination = &&destination_label_ ;; // INSTPAT_START(); 的展开，为了提前确定destination的地址
//
//   do {
//     uint64_t key, mask, shift;
//     pattern_decode("??????? ????? ????? ??? ????? 00101 11",
//                     (sizeof("??????? ????? ????? ??? ????? 00101 11") - 1),
//                     &key, &mask, &shift);
//     if ((((uint64_t)((s)->isa.inst.val) >> shift) & mask) == key) {
//       {
//         decode_operand(s, &rd, &src1, &src2, &imm, TYPE_U);
//         (cpu.gpr[check_reg_idx(rd)]) = s->pc + imm;
//       };
//       goto *(destination); // 跳
//     }
//   } while (0);
//
//   do {
//     uint64_t key, mask, shift;
//     pattern_decode("??????? ????? ????? 100 ????? 00000 11",
//                     (sizeof("??????? ????? ????? 100 ????? 00000 11") - 1),
//                     &key, &mask, &shift);
//     if ((((uint64_t)((s)->isa.inst.val) >> shift) & mask) == key) {
//       {
//         decode_operand(s, &rd, &src1, &src2, &imm, TYPE_I);
//         (cpu.gpr[check_reg_idx(rd)]) = vaddr_read(src1 + imm, 1);
//       };
//       goto *(destination); // 跳
//     }
//   } while (0);
//
// destination_label_ : ; }; // INSTPAT_END(); 的展开，就是在设定需要goto的`label:`

#endif
