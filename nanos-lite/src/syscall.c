#include <common.h>
#include "syscall.h"

int sys_write(int fd, void *buf, size_t count) {
  int ret = 0;
  if (fd == 1 || fd == 2) {
    char *ptr = (char *)buf;
    for (size_t i = 0; i < count; i++) {
      putch(ptr[i]);
      ret++;
    }
    return ret;
  }
  return -1; // error
}

int sys_brk(void *addr) {
  // 目前堆区大小的调整总是成功
  return 0;
}

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1; // a7
  a[1] = c->GPR2; // a0
  a[2] = c->GPR3; // a1
  a[3] = c->GPR4; // a2

  // 根据a7
  switch (a[0]) {

  case SYS_exit:
    // printf("halt(%d)\n", a[1]); // strace
    halt(a[1]); // a0作为参数给halt
    break;

  case SYS_yield:
    // printf("yield()\n"); // strace
    yield();     // 调用am中的yield()，然后还是会到do_event()中的case EVENT_YIELD
    c->GPRx = 0; // 设置返回值a0为0
    break;

  case SYS_write:
    // printf("sys_write(%d, 0x%x, %d)\n", a[1], a[2], a[3]); // strace
    c->GPRx = sys_write(a[1], (void *)a[2], a[3]);
    break;

  case SYS_brk:
    // printf("sys_brk(0x%x)\n", a[1]); // strace
    c->GPRx = sys_brk((void *)a[1]);
    break;

  default:
    panic("Unhandled syscall ID = %d", a[0]);
  }
}