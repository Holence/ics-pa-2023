#include <common.h>
#include "syscall.h"
void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1; // a7
  a[1] = c->GPR2; // a0
  a[2] = c->GPR3; // a1
  a[3] = c->GPR4; // a2

  // 根据a7
  switch (a[0]) {

  case SYS_exit:
    halt(a[1]); // a0作为参数给halt
    break;

  case SYS_yield:
    yield();     // 调用am中的yield()，然后还是会到do_event()中的case EVENT_YIELD
    c->GPRx = 0; // 设置返回值a0为0
    break;

  default:
    panic("Unhandled syscall ID = %d", a[0]);
  }
}
