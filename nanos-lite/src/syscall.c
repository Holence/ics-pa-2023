#include <common.h>
#include "syscall.h"
#include <fs.h>
#include <sys/time.h>
#include <proc.h>

void naive_uload(PCB *pcb, const char *filename);
void context_uload(PCB *pcb, const char *filename, char *const argv[], char *const envp[]);
void switch_boot_pcb();
int mm_brk(uintptr_t brk);

int sys_gettimeofday(void *tv, void *tz) {
  int us = io_read(AM_TIMER_UPTIME).us;
  ((struct timeval *)tv)->tv_sec = us / 1000000;
  ((struct timeval *)tv)->tv_usec = us % 1000000;
  return 0;
}

int sys_execve(const char *fname, char *const argv[], char *const envp[]) {
  // PA3 直接覆盖到0x83000000的地方，内存中只能允许一个用户进程活着
  // naive_uload(NULL, fname); // if succeed, 穿越时空

  // PA4
  if (file_exist(fname)) {
    context_uload(current, fname, argv, envp);
    switch_boot_pcb();
    yield();
    panic("execve should not be here");
  } else {
    return -2;
  }
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
    // PA3
    // halt(a[1]); // a0作为参数给halt
    // naive_uload(NULL, "/bin/menu");
    // naive_uload(NULL, "/bin/nterm");

    // PA4
    // 虽然不像naive_uload那样使函数栈一直增加
    // 但目前每次切换新进程，不会free掉旧进程的用户栈page，还是在无止境的递增啊
    sys_execve(args_menu[0], args_menu, empty);
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
    // Log("STRACE🔍: fs_lseek(%s, 0x%x, %d)", get_file_name(a[1]), a[2], a[3]);
    c->GPRx = fs_lseek(a[1], a[2], a[3]);
    break;

  case SYS_brk:
    Log("STRACE🔍: sys_brk(0x%x)", a[1]);
    c->GPRx = mm_brk((uintptr_t)a[1]);
    break;

  case SYS_execve:
    Log("STRACE🔍: sys_execve(%s, %s, %x)", (char *)a[1], *((char **)a[2]), (char **)a[3]);
    c->GPRx = sys_execve((char *)a[1], (char **)a[2], (char **)a[3]);
    break;

  case SYS_gettimeofday:
    // Log("STRACE🔍: sys_gettimeofday(0x%x, 0x%x)", a[1], a[2]);
    c->GPRx = sys_gettimeofday((void *)a[1], (void *)a[2]);
    break;

  default:
    panic("Unhandled syscall ID = %d", a[0]);
  }
}