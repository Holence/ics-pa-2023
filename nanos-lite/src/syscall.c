#include <common.h>
#include "syscall.h"
#include <fs.h>
#include <time.h>

int sys_brk(void *addr) {
  // 目前堆区大小的调整总是成功
  return 0;
}

int sys_gettimeofday(void *tv, void *tz) {
  int us = io_read(AM_TIMER_UPTIME).us;
  ((struct timeval *)tv)->tv_sec = us / 1000000;
  ((struct timeval *)tv)->tv_usec = us % 1000000;
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
    Log("STRACE🔍: halt(%d)", a[1]);
    halt(a[1]); // a0作为参数给halt
    break;

  case SYS_yield:
    Log("STRACE🔍: yield()");
    yield();     // 调用am中的yield()，然后还是会到do_event()中的case EVENT_YIELD
    c->GPRx = 0; // 设置返回值a0为0
    break;

  case SYS_open:
    // Log("STRACE🔍: fs_open(%s, %d, %d)", (char *)a[1], a[2], a[3]);
    c->GPRx = fs_open((char *)a[1], a[2], a[3]);
    break;

  case SYS_read:
    // Log("STRACE🔍: fs_read(%s, 0x%x, %d)", get_file_name(a[1]), a[2], a[3]);
    c->GPRx = fs_read(a[1], (void *)a[2], a[3]);
    break;

  case SYS_write:
    // Log("STRACE🔍: fs_write(%s, 0x%x, %d)", get_file_name(a[1]), a[2], a[3]);
    c->GPRx = fs_write(a[1], (void *)a[2], a[3]);
    break;

  case SYS_close:
    // Log("STRACE🔍: fs_close(%s)", get_file_name(a[1]));
    c->GPRx = fs_close(a[1]);
    break;

  case SYS_lseek:
    Log("STRACE🔍: fs_lseek(%s, 0x%x, %d)", get_file_name(a[1]), a[2], a[3]);
    c->GPRx = fs_lseek(a[1], a[2], a[3]);
    break;

  case SYS_brk:
    Log("STRACE🔍: sys_brk(0x%x)", a[1]);
    c->GPRx = sys_brk((void *)a[1]);
    break;

  case SYS_gettimeofday:
    // Log("STRACE🔍: sys_gettimeofday(0x%x, 0x%x)", a[1], a[2]);
    c->GPRx = sys_gettimeofday((void *)a[1], (void *)a[2]);
    break;

  default:
    panic("Unhandled syscall ID = %d", a[0]);
  }
}