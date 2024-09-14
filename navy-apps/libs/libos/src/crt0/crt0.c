#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

void __libc_init_array(void);
int main(int argc, char *argv[], char *envp[]);
extern char **environ;
void call_main(uintptr_t *args) {
  char *empty[] = {NULL};
  environ = empty;
  __libc_init_array();
  exit(main(0, empty, empty));
  assert(0);
}
