#include <proc.h>

#define MAX_NR_PROC 4

uintptr_t loader(PCB *pcb, const char *filename);
void naive_uload(PCB *pcb, const char *filename);
void play_boot_music();

static PCB pcb[MAX_NR_PROC] __attribute__((used)) = {};
static PCB pcb_boot = {};
PCB *current = NULL;

char *empty[] = {NULL};
char *args_menu[] = {"/bin/menu", NULL};
char *args_pal[] = {"/bin/pal", "--skip", NULL};

void switch_boot_pcb() {
  current = &pcb_boot;
}

void hello_fun(void *arg) {
  int j = 1;
  while (1) {
    Log("Hello World from Nanos-lite with arg '%p' for the %dth time!", (uintptr_t)arg, j);
    j++;
    yield();
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
  pcb->cp = kcontext((Area){pcb->stack, pcb + 1}, entry, arg);
  return;
}

void context_uload(PCB *pcb, const char *filename, char *const argv[], char *const envp[]) {

  // 每个用户进程的栈，开设在堆区申请的一个32KB的页面中
  char *string_area_ptr = (char *)new_page(8);

  char **string_ptr = (char **)argv;
  while (*string_ptr != NULL) {
    int size = strlen(*string_ptr) + 1;
    string_area_ptr -= size;
    strcpy(string_area_ptr, *string_ptr);
    *string_ptr = string_area_ptr;
    string_ptr++;
  }
  int argv_len = string_ptr - argv + 1; // +1是算上NULL

  string_ptr = (char **)envp;
  while (*string_ptr != NULL) {
    int size = strlen(*string_ptr) + 1;
    string_area_ptr -= size;
    strcpy(string_area_ptr, *string_ptr);
    *string_ptr = string_area_ptr;
    string_ptr++;
  }
  int evnp_len = string_ptr - envp + 1; // +1是算上NULL

  char **pointer_area_ptr = (char **)string_area_ptr;
  pointer_area_ptr -= evnp_len;
  for (int i = 0; i < evnp_len; i++) {
    pointer_area_ptr[i] = envp[i];
  }

  pointer_area_ptr -= argv_len;
  for (int i = 0; i < argv_len; i++) {
    pointer_area_ptr[i] = argv[i];
  }

  pointer_area_ptr--;
  *((size_t *)pointer_area_ptr) = argv_len - 1;

  AddrSpace addr;
  uintptr_t entry = loader(pcb, filename);
  // 初始Context依旧在内核的PCB中
  pcb->cp = ucontext(&addr, (Area){pcb->stack, pcb + 1}, (void *)entry);

  // 为了让栈开设到堆区上申请出来的页里，就得等schedule、__am_asm_trap用sp得到PCB中Context的地址并初始化完Context后，mret进入进程函数后的第一句指令，立刻设置sp为页上的地址
  // Nanos-lite和Navy作了一项约定: Nanos-lite把进程的栈顶地址记录到GPRx中, 然后由Navy里面的_start中把栈顶地址设置到sp寄存器中
  pcb->cp->GPRx = (uintptr_t)pointer_area_ptr;

  return;
}

void init_proc() {
  context_kload(&pcb[0], hello_fun, (void *)1);

  context_uload(&pcb[1], args_menu[0], args_menu, empty);
  // context_uload(&pcb[1], args_pal[0], args_pal, empty);

  switch_boot_pcb();
}

Context *schedule(Context *prev) {
  current->cp = prev;
  current = (current == &pcb[0] ? &pcb[1] : &pcb[0]);
  return current->cp; // 这里返回的Context*，将会在__am_asm_trap中被用于更新sp
}
