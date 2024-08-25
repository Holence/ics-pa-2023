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

#include "../monitor/sdb/sdb.h"
#include <cpu/cpu.h>
#include <cpu/decode.h>
#include <cpu/difftest.h>
#include <locale.h>

/* The assembly code of instructions executed is only output to the screen
 * when the number of instructions executed is less than this value.
 * This is useful when you use the `si' command.
 * You can modify this value as you want.
 */
#define MAX_INST_TO_PRINT 10

CPU_state cpu = {};
uint64_t g_nr_guest_inst = 0;
static uint64_t g_timer = 0; // unit: us
static bool g_print_step = false;

void device_update();

#ifdef CONFIG_ITRACE
static void itrace(Decode *s) {
  // instruction trace
  // 会在nemu-log.txt中输出形状如下的指令记录（先记录在s->logbuf中，之后在trace_and_difftest中在通过ITRACE_COND判断log_write）
  // 0x80000000: 00 00 02 97 auipc	t0, 0
  // 0x80000004: 00 02 88 23 sb	zero, 16(t0)
  // 0x80000008: 01 02 c5 03 lbu	a0, 16(t0)
  // 0x8000000c: 00 10 00 73 ebreak
  char *p = s->logbuf;
  p += snprintf(p, sizeof(s->logbuf), FMT_WORD ":", s->pc);
  int ilen = s->snpc - s->pc; // 4, 单条指令字节数
  int i;
  uint8_t *byte_ptr = (uint8_t *)&s->isa.inst.val;
  for (i = ilen - 1; i >= 0; i--) {
    p += snprintf(p, 4, " %02x", byte_ptr[i]);
  }
  int ilen_max = MUXDEF(CONFIG_ISA_x86, 8, 4);
  int space_len = ilen_max - ilen;
  if (space_len < 0)
    space_len = 0;
  space_len = space_len * 3 + 1;
  memset(p, ' ', space_len);
  p += space_len;

// 反汇编，从机器码解析出汇编，具体原理不懂，llvm是啥❓
#ifndef CONFIG_ISA_loongarch32r
  void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);
  disassemble(p, s->logbuf + sizeof(s->logbuf) - p,
              MUXDEF(CONFIG_ISA_x86, s->snpc, s->pc), (uint8_t *)&s->isa.inst.val, ilen);
#else
  p[0] = '\0'; // the upstream llvm does not support loongarch32r
#endif
}
#endif

#ifdef CONFIG_IRINGBUG_TRACE
#define IRINGBUF_SIZE 16
static char iringbuf[IRINGBUF_SIZE][128];
static int iringbuf_index = 0;
static void iringbuf_trace(char *logbuf) {
  strcpy(iringbuf[iringbuf_index], logbuf);
  iringbuf_index = (iringbuf_index + 1) % IRINGBUF_SIZE;
}

static void print_iringbug_log() {
  printf(ANSI_FMT("#################### iringbuf ####################\n", ANSI_FG_RED));
  int index = iringbuf_index;
  int index_end = iringbuf_index == 0 ? IRINGBUF_SIZE - 1 : iringbuf_index - 1;
  while (true) {
    if (strlen(iringbuf[index]) != 0) {
      printf(ANSI_FMT("%s\n", ANSI_FG_RED), iringbuf[index]);
    }
    if (index != index_end) {
      index = (index + 1) % IRINGBUF_SIZE;
    } else {
      break;
    }
  }
  printf(ANSI_FMT("##################################################\n", ANSI_FG_RED));
}
#endif

static void trace_and_difftest(Decode *_this, vaddr_t dnpc) {
#ifdef CONFIG_ITRACE
  // write itrace to _this->logbuf
  itrace(_this);
#endif

#ifdef CONFIG_ITRACE_COND
  if (ITRACE_COND) {
    // write to nemu-log.txt
    log_write("%s\n", _this->logbuf);
  }
#endif

#ifdef CONFIG_IRINGBUG_TRACE
  // record iringbuf
  iringbuf_trace(_this->logbuf);
#endif

  // print to stdout
  if (g_print_step) {
    IFDEF(CONFIG_ITRACE, puts(_this->logbuf));
  }

  IFDEF(CONFIG_DIFFTEST, difftest_step(_this->pc, dnpc));

#ifndef CONFIG_TARGET_AM
  // watchpoint检查是否有更新，若有的话，打印新旧值，并暂停exec
  wp_check_changed();
#endif
}

static void exec_once(Decode *s, vaddr_t pc) {
  s->pc = pc;
  s->snpc = pc;
  isa_exec_once(s);
  // isa_exec_once完事后
  // s->pc是当前指令的地址
  // s->snpc在isa_exec_once()中被inst_fetch()加了4
  // s->dnpc是下一跳的地址
  cpu.pc = s->dnpc;
}

static void execute(uint64_t n) {
  Decode s;
  for (; n > 0; n--) {
    exec_once(&s, cpu.pc);
    g_nr_guest_inst++;
    trace_and_difftest(&s, cpu.pc);
    if (nemu_state.state != NEMU_RUNNING)
      break;
    IFDEF(CONFIG_DEVICE, device_update());
  }
}

static void statistic() {
  IFNDEF(CONFIG_TARGET_AM, setlocale(LC_NUMERIC, ""));
#define NUMBERIC_FMT MUXDEF(CONFIG_TARGET_AM, "%", "%'") PRIu64
  Log("host time spent = " NUMBERIC_FMT " us", g_timer);
  Log("total guest instructions = " NUMBERIC_FMT, g_nr_guest_inst);
  if (g_timer > 0)
    Log("simulation frequency = " NUMBERIC_FMT " inst/s", g_nr_guest_inst * 1000000 / g_timer);
  else
    Log("Finish running in less than 1 us and can not calculate the simulation frequency");
}

void assert_fail_msg() {
  isa_reg_display();
  statistic();
}

/* Simulate how the CPU works. */
void cpu_exec(uint64_t n) {
  g_print_step = (n < MAX_INST_TO_PRINT);
  switch (nemu_state.state) {
  case NEMU_END:
  case NEMU_ABORT:
    printf("Program execution has ended. To restart the program, exit NEMU and run again.\n");
    return;
  default:
    nemu_state.state = NEMU_RUNNING;
  }

  uint64_t timer_start = get_time();

  execute(n);

  uint64_t timer_end = get_time();
  g_timer += timer_end - timer_start;

  switch (nemu_state.state) {
  case NEMU_RUNNING:
    nemu_state.state = NEMU_STOP;
    break;

  case NEMU_END:
  case NEMU_ABORT:

#ifdef CONFIG_IRINGBUG_TRACE
    // 出错: invalid_inst 或者 nemu_state.halt_ret!=0时 在stdout输出iringbuf
    if (nemu_state.state == NEMU_ABORT || nemu_state.halt_ret != 0) {
      print_iringbug_log();
    }
#endif

    Log("nemu: %s at pc = " FMT_WORD,
        (nemu_state.state == NEMU_ABORT ? ANSI_FMT("ABORT", ANSI_FG_RED) : (nemu_state.halt_ret == 0 ? ANSI_FMT("HIT GOOD TRAP", ANSI_FG_GREEN) : ANSI_FMT("HIT BAD TRAP", ANSI_FG_RED))),
        nemu_state.halt_pc);
    // fall through
  case NEMU_QUIT:
    statistic();
  }
}
