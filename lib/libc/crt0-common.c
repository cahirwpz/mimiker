#include <sys/cdefs.h>
#include <stdlib.h>
#include <unistd.h>
#include "env.h"

extern int main(int, char **, char **);
extern void _libc_init(void);

void ___start(int argc, char **argv, char **envp) {
  environ = envp;

  _libc_init();

  exit(main(argc, argv, environ));
}
