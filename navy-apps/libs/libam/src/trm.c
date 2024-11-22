#include <am.h>
#include <klib-macros.h>

#define HEAP_SIZE (4 << 20)
Area heap;
__attribute__((constructor)) void before_main() {
  heap.start = malloc(HEAP_SIZE);
  panic_on(heap.start == NULL, "malloc failed");
  heap.end = heap.start + HEAP_SIZE;
}

void putch(char ch) {
  putc(ch, stdout);
}

void halt(int code) {
  exit(code);
}
