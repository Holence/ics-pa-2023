#include <am.h>
#include <nemu.h>
#include <stdio.h>

extern char _end;
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

#include "platform/nemu/ioe/ioe.c"
void _trm_init() {
  printf("PMEM_END:       %p\n", PMEM_END);
  printf("                👆HEAP\n");
  printf("_heap_start:    %p\n", &_heap_start);
  printf("_end:           %p\n", &_end);
  printf("_stack_pointer: %p\n", &_stack_pointer);
  printf("                👇STACK\n");
  printf("_stack_top:     %p\n", &_stack_top);
  printf("lut[128]:       %p\n", lut);
  printf("_pmem_start:    %p\n", &_pmem_start);
  int ret = main(mainargs);
  halt(ret);
}
