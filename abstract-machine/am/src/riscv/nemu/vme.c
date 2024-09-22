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

  // 新建内核页表
  // kas的area pgsize不用设置吗❓
  kas.ptr = pgalloc_f(PGSIZE);
  printf("Kernel Page Table 0x%x\n", kas.ptr);

  // 内核有权利访问到任何地址
  // 让内核页表覆盖到 所有内存空间pmem[0x80000000, 0x88000000] 与 外设空间[0xa0000000, 0xa1010000] （NEMU_PADDR_SPACE指派的几个空间）
  // 作 虚拟==物理 的恒等映射
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

// 初始化用户进程的AddrSpace
void protect(AddrSpace *as) {
  // user page directory 用户进程的页目录表（一级页表）的地址
  PTE *updir = (PTE *)(pgalloc_usr(PGSIZE)); // 为用户进程新建一个一级页表
  as->ptr = updir;
  as->area = USER_SPACE;
  as->pgsize = PGSIZE;
  // map kernel space
  // 用户进程也知晓内核的地址空间，是为了让用户进程栈能够调用nanos、AM中的函数❓
  memcpy(updir, kas.ptr, PGSIZE);
}

void unprotect(AddrSpace *as) {
}

// Context的一级页表地址pdir = 当前satp中的值
void __am_get_cur_as(Context *c) {
  c->pdir = (vme_enable ? (void *)get_satp() : NULL);
}

// satp = Context的一级页表地址pdir的值
void __am_switch(Context *c) {
  if (vme_enable && c->pdir != NULL) {
    // printf("Set sapt %x\n", c->pdir);
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

// 建立页表（Sv32两级页表），编写va->pa的页表项。
// 目前不建立4MB superpage，isa_mmu_translate里可以省去很多步骤❓
// 不考虑prot
// pa是物理页的地址
// - 内核页表: 有权利访问全图，无需申请物理页，pa仅仅作为内核页表进行“恒等映射”全图的地标 va==pa
// - 用户进程页表: nanos会先new_page获得物理页面的物理地址pa，再到这里创建的页表，所以 va!=pa
void map(AddrSpace *as, void *va, void *pa, int prot) {
  PTE *level_1_pte_addr = ((PTE *)as->ptr) + (uint32_t)VPN1((uint32_t)va);
  PTE pte = *level_1_pte_addr;
  PTE *level_2_page_table_addr;

  // 检查一级页表项是否存在
  if (!(pte & PTE_V)) {
    // 不存在的话申请创建二级页表
    level_2_page_table_addr = pgalloc_usr(PGSIZE);
    // 写入一级页表项
    *level_1_pte_addr = (((PTE)level_2_page_table_addr >> 12) << 10) | 0b0001;
    printf("Create Level 1 PTE idx=%d level_1_pte_addr=0x%x level_2_page_table_addr=0x%x \n", (uint32_t)VPN1((uint32_t)va), level_1_pte_addr, level_2_page_table_addr);
  } else {
    level_2_page_table_addr = (PTE *)(PTE)(PTE_PPN(pte) << 12);
  }

  PTE *level_2_pte_addr = level_2_page_table_addr + (uint32_t)VPN0((uint32_t)va);
  pte = *level_2_pte_addr;
  // 检查二级页表项是否存在
  if (!(pte & PTE_V)) {
    // 不存在才是正常
    // 二级页表项指向物理地址pa
    // 写入二级页表项
    *level_2_pte_addr = (((uint32_t)pa >> 12) << 10) | 0b1111;
    // printf("Map: 0x%x -> PTE[0x%x] -> PTE[0x%x] -> 0x%x\n", va, level_1_pte_addr, level_2_pte_addr, page_addr);
  } else {
    panic("level 2 page table already mapped before");
  }
}

Context *ucontext(AddrSpace *as, Area kstack, void *entry) {
  Context *p = (Context *)(kstack.end - sizeof(Context));
  p->pdir = as->ptr;          // 设置Context的一级页表地址
  p->mepc = (uintptr_t)entry; // 设置mret将要跳转到entry
  p->mstatus = 0x1800;
  return p;
}
