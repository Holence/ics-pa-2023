#include <am.h>
#include <nemu.h>
#include <stdio.h>

extern char _heap_start;
extern char _stack_top;
extern char _stack_pointer;
int main(const char *args);

Area heap = RANGE(&_heap_start, PMEM_END);
#ifndef MAINARGS
#define MAINARGS ""
#endif
static const char mainargs[] = MAINARGS;

void putch(char ch) {
  outb(SERIAL_PORT, ch);
}

void halt(int code) {
  nemu_trap(code);

  // should not reach here
  while (1)
    ;
}

void _trm_init() {
  printf("_pmem_start: %p\n", &_pmem_start);
  printf("PMEM_END: %p\n", PMEM_END);
  printf("_heap_start: %p\n", &_heap_start);
  printf("_stack_top: %p\n", &_stack_top);
  printf("_stack_pointer: %p\n\n", &_stack_pointer);
  int ret = main(mainargs);
  halt(ret);
}
