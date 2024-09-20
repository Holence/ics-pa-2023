#include <am.h>
#include <nemu.h>
#include <klib.h>

static AddrSpace kas = {};
static void *(*pgalloc_usr)(int) = NULL;
static void (*pgfree_usr)(void *) = NULL;
static int vme_enable = 0;

static Area segments[] = { // Kernel memory mappings
    NEMU_PADDR_SPACE};

#define USER_SPACE RANGE(0x40000000, 0x80000000)

// pdir == page directory 页目录表（一级页表）的地址？
// updir == user page directory 用户进程的页目录表（一级页表）的地址？

// 设置satp寄存器 开启分页机制、页目录表（一级页表）的地址为pdir
static inline void set_satp(void *pdir) {
  uintptr_t mode = 1ul << (__riscv_xlen - 1);
  asm volatile("csrw satp, %0" : : "r"(mode | ((uintptr_t)pdir >> 12)));
}

static inline uintptr_t get_satp() {
  uintptr_t satp;
  asm volatile("csrr %0, satp" : "=r"(satp));
  return satp << 12;
}

bool vme_init(void *(*pgalloc_f)(int), void (*pgfree_f)(void *)) {
  pgalloc_usr = pgalloc_f;
  pgfree_usr = pgfree_f;

  // kas的area pgsize不用设置吗❓
  kas.ptr = pgalloc_f(PGSIZE);
  printf("Kernel Page Table %x\n", kas.ptr);

  int i;
  for (i = 0; i < LENGTH(segments); i++) {
    void *va = segments[i].start;
    for (; va < segments[i].end; va += PGSIZE) {
      map(&kas, va, va, 0);
    }
  }

  set_satp(kas.ptr);
  vme_enable = 1;

  return true;
}

void protect(AddrSpace *as) {
  PTE *updir = (PTE *)(pgalloc_usr(PGSIZE));
  as->ptr = updir;
  as->area = USER_SPACE;
  as->pgsize = PGSIZE;
  // map kernel space
  memcpy(updir, kas.ptr, PGSIZE);
}

void unprotect(AddrSpace *as) {
}

void __am_get_cur_as(Context *c) {
  c->pdir = (vme_enable ? (void *)get_satp() : NULL);
}

void __am_switch(Context *c) {
  if (vme_enable && c->pdir != NULL) {
    set_satp(c->pdir);
  }
}

#define BITMASK(x) ((1ull << (x)) - 1)
#define BITS(x, high, low) (((x) >> (low)) & BITMASK((high) - (low) + 1))
#define VPN1(x) BITS(x, 31, 22)
#define VPN0(x) BITS(x, 21, 12)
#define PTE_PPN1(x) BITS(x, 31, 20)
#define PTE_PPN0(x) BITS(x, 19, 10)
#define PTE_PPN(x) BITS(x, 31, 10)
#define PTE_V_BIT(x) BITS(x, 0, 0)

// 建立va->pa的页表项（Sv32两级页表）
// 内核的虚拟地址空间中恒等映射 虚拟地址==物理地址
// 这里要对页表进行读写，这里假设虚拟地址==物理地址。那用户进程空间的页表呢，也在“虚拟地址==物理地址”的内核空间中吗❓
// 目前不建立4MB superpage，isa_mmu_translate里可以省去很多步骤❓
// prot没用，但pa是啥❓
void map(AddrSpace *as, void *va, void *pa, int prot) {
  PTE *level_1_pte_addr = ((PTE *)as->ptr) + VPN1((uint32_t)va);
  PTE pte = *level_1_pte_addr;
  PTE *level_2_page_table_addr;

  // 检查一级页表项是否存在
  if (!PTE_V_BIT(pte)) {
    // 不存在的话申请创建二级页表
    level_2_page_table_addr = pgalloc_usr(PGSIZE);
    // 写入一级页表项
    *level_1_pte_addr = (((PTE)level_2_page_table_addr >> 12) << 10) | 0b0001;
  } else {
    level_2_page_table_addr = (PTE *)(PTE)(PTE_PPN(pte) << 12);
  }

  PTE *level_2_pte_addr = level_2_page_table_addr + VPN0((uint32_t)va);
  pte = *level_2_pte_addr;
  PTE *page_addr;
  // 检查二级页表项是否存在
  if (!PTE_V_BIT(pte)) {
    // 不存在的话申请创建页❓
    // page_addr = pgalloc_usr(PGSIZE);

    // 还是直接填入pa的值？pa指向的页在哪里申请过❓
    page_addr = pa;

    // 写入二级页表项
    *level_2_pte_addr = (((uint32_t)page_addr >> 12) << 10) | 0b1111;
    // printf("Map: 0x%x -> PTE[0x%x] -> PTE[0x%x] -> 0x%x\n", va, level_1_pte_addr, level_2_pte_addr, page_addr);
  } else {
    panic("level 2 page table already mapped before");
  }
}

Context *ucontext(AddrSpace *as, Area kstack, void *entry) {
  Context *p = (Context *)(kstack.end - sizeof(Context));
  p->mepc = (uintptr_t)entry; // 设置mret将要跳转到entry
  // p->GPR2 = (uintptr_t)arg;   // 设置即将传入entry的第一个参数a0的值为arg
  p->mstatus = 0x1800;
  return p;
}
