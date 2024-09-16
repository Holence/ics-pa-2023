#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

void __libc_init_array(void);
int main(int argc, char *argv[], char *envp[]);
extern char **environ;
void call_main(uintptr_t *args) {
  char *empty[] = {NULL};

  // PA4 检查确实在使用用户栈而不是内核栈
  printf("Navy Stack: %p\n", empty);
  assert((uintptr_t)empty > 0x83000000);

  environ = empty;
  __libc_init_array();
  exit(main(0, empty, empty));
  assert(0);
}
