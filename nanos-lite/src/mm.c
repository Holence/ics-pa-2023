#include <memory.h>
#include <proc.h>

static void *pf = NULL;

// 向上申请nr_page页，返回pf的原始值
void *new_page(size_t nr_page) {
  void *ret = pf;
  pf += nr_page * PGSIZE;

  // PA4.3之前，进程处于0x83000000处，需要禁止内核的堆生长到navy被load到内存中的地方
  // assert((uintptr_t)pf < 0x83000000);

  // PA4.3之后，进程被load到独立的页中，内核的堆可以尽情生长，上限就是pemem的上限了
  assert(pf < heap.end);

  return ret;
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

/*
 * The brk() system call handler.
 * 只需要保证 brk <= max_brk 且 [_end, max_brk] 的地址空间都可以分页机制访问即可
 * max_brk 在 loader() 中会被设置到 ROUNDUP(_end, PGSIZE) 的地址
 * 这里只需要让 max_brk 超越过 brk 即可
 */
int mm_brk(uintptr_t brk) {
  while (current->max_brk < brk) {
    void *page = new_page(1);
    map(&(current->as), (void *)current->max_brk, page, MMAP_READ | MMAP_WRITE);
    memset(page, 0, PGSIZE);
    current->max_brk += PGSIZE;
  }
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
