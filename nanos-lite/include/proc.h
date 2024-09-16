#ifndef __PROC_H__
#define __PROC_H__

#include <common.h>
#include <memory.h>

#define STACK_SIZE (8 * PGSIZE)

typedef union {
  uint8_t stack[STACK_SIZE] PG_ALIGN; // 内核栈，在全局变量区
  struct {
    Context *cp; // 指向Context，初始化的时候Context在PCB.stack中，之后Context在__am_asm_trap被调用时的栈上
    AddrSpace as;
    // we do not free memory, so use `max_brk' to determine when to call _map()
    uintptr_t max_brk;
  };
} PCB;

extern PCB *current;

#endif
