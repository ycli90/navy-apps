#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

int main(int argc, char *argv[], char *envp[]);
extern char **environ;
void call_main(uintptr_t *args) {
  int *argc_p = (int*)args;
  int argc = *argc_p;
  char **argv = (char**)(argc_p + 1);
  char **envp = argv + argc + 1;
  environ = envp;
  __libc_init_array();
  exit(main(argc, argv, envp));
  assert(0);
}
