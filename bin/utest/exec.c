#include "utest.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <assert.h>

int test_execve() {
  int pid = fork();
  if (pid) {
    /* parent */
    int status;
    int r = wait(&status);
    assert(r == pid);
    printf("parent: child exited with status %d\n", status);
    assert(status == 0);
    return 0;
  }
  /* child */
  char *args[] = {"echo", "test", NULL};
  char *env[] = {"test", "environment", NULL};
  int r = execve("/bin/echo", args, env);
  printf("child: execv failed with %d (%s)\n", r, strerror(errno));
  _exit(1);
}

int test_execv() {
  int pid = fork();
  if (pid) {
    /* parent */
    int status;
    int r = wait(&status);
    assert(r == pid);
    printf("parent: child exited with status %d\n", status);
    assert(status == 0);
    return 0;
  }
  /* child */
  char *args[] = {"echo", "test", NULL};
  int r = execv("/bin/echo", args);
  printf("child: execv failed with %d (%s)\n", r, strerror(errno));
  _exit(1);
}
