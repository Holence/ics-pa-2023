#include <proc.h>

#define MAX_NR_PROC 4

uintptr_t loader(PCB *pcb, const char *filename);
void naive_uload(PCB *pcb, const char *filename);
void play_boot_music();

static PCB pcb[MAX_NR_PROC] __attribute__((used)) = {};
static PCB pcb_boot = {};
PCB *current = NULL;

char *empty[] = {NULL};
char *args_0[] = {"/bin/hello", NULL};
char *args_1[] = {"/bin/menu", NULL};
char *args_2[] = {"/bin/nterm", NULL};
char *args_3[] = {"/bin/pal", "--skip", NULL};

static int greedy_process_timer = 0;

void switch_boot_pcb() {
  greedy_process_timer = 0;
  current = &pcb_boot;
}

void hello_fun(void *arg) {
  int j = 1;
  while (1) {
    if (j % 1000000 == 0) {
      Log("Hello World from Nanos-lite with arg '%p' for the %dth time!", (uintptr_t)arg, j);
    }
    j++;
  }
}

// PA3
// void init_proc() {
//   switch_boot_pcb();
//   play_boot_music();

//   Log("Initializing processes...");

//   // load program here
//   naive_uload(NULL, "/bin/menu");
//   // naive_uload(NULL, "/bin/nterm");
// }

// PA4
void context_kload(PCB *pcb, void (*entry)(void *), void *arg) {
  // 内核线程的栈，就直接长在PCB中
  pcb->cp = kcontext((Area){pcb, pcb + 1}, entry, arg);
  return;
}

void context_uload(PCB *pcb, const char *filename, char *const argv[], char *const envp[]) {
  // 初始化进程的AddrSpace
  // 设置pgsize为PGSIZE
  // 设置Area为[0x40000000, 0x80000000]
  // 新建进程的一级页表，并用as->ptr记录地址
  protect(&(pcb->as));

  // 将程序的代码段、数据段装入内存
  uintptr_t entry = loader(pcb, filename);

  ///////////////////////////////////////////////////////
#define PROCESS_STACK_PG_NUM 8

  // 初始Context，放在PCB kstack的顶端，等schedule后被__am_asm_trap恢复
  pcb->cp = ucontext(&(pcb->as), (Area){pcb, pcb + 1}, (void *)entry);

  // 在堆区申请的一个32KB的页面作为用户进程栈
  void *page = new_page(PROCESS_STACK_PG_NUM);

  // 将用户进程栈也通过分页机制管理
  // 规定在虚拟空间中的末尾[0x80000000 - 8*PGSIZE, 0x80000000]
  {
    void *page_vaddr = pcb->as.area.end - PROCESS_STACK_PG_NUM * PGSIZE;
    void *page_paddr = page;
    for (int i = 0; i < PROCESS_STACK_PG_NUM; i++) {
      map(&(pcb->as), page_vaddr, page_paddr, MMAP_READ | MMAP_WRITE);
      page_vaddr += PGSIZE;
      page_paddr += PGSIZE;
    }
  }

  // 初始化用户进程栈（string area, envp, argv, argc）
  char *string_area_ptr = (char *)page + PROCESS_STACK_PG_NUM * PGSIZE;

  // string area - argv
  char **string_ptr = (char **)argv;
  while (*string_ptr != NULL) {
    int size = strlen(*string_ptr) + 1;
    string_area_ptr -= size;
    strcpy(string_area_ptr, *string_ptr);
    *string_ptr = string_area_ptr;
    string_ptr++;
  }
  int argv_len = string_ptr - argv + 1; // +1是算上NULL

  // string area - envp
  string_ptr = (char **)envp;
  while (*string_ptr != NULL) {
    int size = strlen(*string_ptr) + 1;
    string_area_ptr -= size;
    strcpy(string_area_ptr, *string_ptr);
    *string_ptr = string_area_ptr;
    string_ptr++;
  }
  int evnp_len = string_ptr - envp + 1; // +1是算上NULL

  // envp array
  char **pointer_area_ptr = (char **)string_area_ptr;
  pointer_area_ptr -= evnp_len;
  for (int i = 0; i < evnp_len; i++) {
    pointer_area_ptr[i] = envp[i];
  }
  // argv array
  pointer_area_ptr -= argv_len;
  for (int i = 0; i < argv_len; i++) {
    pointer_area_ptr[i] = argv[i];
  }
  // argc
  pointer_area_ptr--;
  *((size_t *)pointer_area_ptr) = argv_len - 1;
  ///////////////////////////////////////////////////////

  // 为了让栈开设到堆区上申请出来的页里，就得等schedule、__am_asm_trap用sp得到PCB中Context的地址并初始化完Context后，mret进入进程函数后的第一句指令，立刻设置sp为页上的地址
  // Nanos-lite和Navy作了一项约定: Nanos-lite把进程的栈顶地址记录到GPRx中, 然后由Navy里面的_start中把栈顶地址设置到sp寄存器中
  pcb->cp->GPRx = (uintptr_t)pointer_area_ptr;

  return;
}

static PCB *fg_pcb, *bg_pcb;
void init_proc() {
  // context_kload(&pcb[0], hello_fun, (void *)1);

  Log("Load User Process \"%s\"", args_0[0]);
  context_uload(&pcb[0], args_0[0], args_0, empty);
  bg_pcb = &pcb[0];

  Log("Load User Process \"%s\"", args_1[0]);
  context_uload(&pcb[1], args_1[0], args_1, empty);
  fg_pcb = &pcb[1];

  Log("Load User Process \"%s\"", args_2[0]);
  context_uload(&pcb[2], args_2[0], args_2, empty);

  Log("Load User Process \"%s\"", args_3[0]);
  context_uload(&pcb[3], args_3[0], args_3, empty);

  switch_boot_pcb();
}

void set_fg_pcb(int index) {
  fg_pcb = pcb + index;
}

Context *schedule(Context *prev) {
  current->cp = prev;
  if (current == bg_pcb) {
    current = fg_pcb;
    printf("Switch To Foreground PCB[%d] %x\n", fg_pcb - pcb, current->cp);
  } else {
    if (greedy_process_timer % 50 == 0) {
      current = bg_pcb;
      printf("Switch To Background PCB[%d] %x\n", bg_pcb - pcb, current->cp);
      greedy_process_timer = 0;
    }
  }

  greedy_process_timer++;
  return current->cp; // 这里返回的Context*，将会在__am_asm_trap中被用于更新sp
}
