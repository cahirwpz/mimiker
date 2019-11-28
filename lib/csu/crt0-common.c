#include <sys/cdefs.h>
#include <stdlib.h>
#include <unistd.h>

extern int main(int, char **, char **);
extern void _libc_init(void);

extern char **environ;
char *__progname = "";

void ___start(int argc, char **argv, char **envp) {
  environ = envp;

  __progname = argv[0];

  _libc_init();

  exit(main(argc, argv, environ));
}
