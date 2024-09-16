#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

void __libc_init_array(void);
int main(int argc, char *argv[], char *envp[]);
extern char **environ;

void call_main(uintptr_t *args) {

  // PA4 检查确实在使用用户栈而不是内核栈
  printf("[call_main] Navy Stack: %p\n", args);
  // assert((uintptr_t)args > 0x83000000);

  // PA4 传入参数
  int argc = args[0];
  printf("[call_main] argc: %d\n", argc);

  char **argv = (char **)args + 1;
  for (int i = 0; i < argc; i++) {
    printf("[call_main] argv[%d]: %s\n", i, argv[i]);
  }

  char **envp = argv + argc + 1;
  char **envp_ptr = envp;
  while (*envp_ptr != NULL) {
    printf("[call_main] envp: %s\n", *(envp_ptr++));
  }

  __libc_init_array(); // PA3.5

  environ = envp;
  exit(main(argc, argv, envp));
  assert(0);
}
