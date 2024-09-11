#include <am.h>
#include <klib-macros.h>

// ❓navy程序的heap，应该是从navy程序中_end向上一直到PMEM_END，但怎么在这里设置
extern char _end;
Area heap = RANGE(&_end, &_end + 0x8000000);

void putch(char ch) {
  putc(ch, stdout);
}

void halt(int code) {
  exit(code);
}
