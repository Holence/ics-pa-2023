#include <memory.h>

static void *pf = NULL;

void *new_page(size_t nr_page) {
  pf += nr_page * PGSIZE;
  // PA4.3之前需要禁止，PA4.3就开始load到堆中申请的页面里了
  // assert((uintptr_t)pf < 0x83000000); // 禁止nanos的堆区生长到navy被load到内存中的地方
  return pf;
}

#ifdef HAS_VME
// n为分配空间的字节数
static void *pg_alloc(int n) {
  int nr_page = n / PGSIZE;
  if (n % PGSIZE != 0) {
    nr_page++;
  }
  void *ret = new_page(nr_page);
  memset(ret, 0, nr_page * PGSIZE);
  return ret;
}
#endif

void free_page(void *p) {
  panic("not implement yet");
}

/* The brk() system call handler. */
int mm_brk(uintptr_t brk) {
  return 0;
}

void init_mm() {
  // 将TRM提供的堆区起始地址作为空闲物理页的首地址
  pf = (void *)ROUNDUP(heap.start, PGSIZE);
  Log("free physical pages starting from %p", pf);

#ifdef HAS_VME
  Log("Allocating Page...");
  vme_init(pg_alloc, free_page);
  Log("Allocating Page Finished");
#endif
}
