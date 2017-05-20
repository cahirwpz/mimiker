#include "utest.h"

#include <stdio.h>
#include <string.h>



void sigusr1_handler(int signo) {
  printf("sigusr1 handled!\n");
}

void sigint_handler(int signo) {
  printf("sigint handled!\n");
  raise(SIGUSR1); /* Recursive signals! */
}

void sigusr2_handler(int signo) {
  printf("Child process handles sigusr2.\n");
  raise(SIGABRT); /* Terminate self. */
}

void signal_test() {
  /* TODO: Cannot use signal(...) here, because the one provided by newlib
     emulates signals in userspace. Please recompile newlib with
     -DSIGNAL_PROVIDED. */
  signal(SIGINT, sigint_handler);
  signal(SIGUSR1, sigusr1_handler);
  raise(SIGINT);

  /* Restore original behavior. */
  signal(SIGINT, SIG_DFL);
  signal(SIGUSR1, SIG_DFL);

  /* Test sending a signal to a different thread. */
  signal(SIGUSR2, sigusr2_handler);
  int pid = fork();
  if (pid == 0) {
    printf("This is child (mypid = %d)\n", getpid());
    while (1)
      ;
  } else {
    printf("This is parent (childpid = %d, mypid = %d)\n", pid, getpid());
    kill(pid, SIGUSR2);
    int status;
    printf("Waiting for child...\n");
    wait(&status);
    assert(WIFSIGNALED(status));
    assert(WTERMSIG(status) == SIGABRT);
    printf("Child was stopped by SIGABRT.\n");
  }

/* Test invalid memory access. */
#if 0
  struct {int x;} *ptr = 0x0;
  ptr->x = 42;
#endif
}

/* TODO: Move away the signal test. */


int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Not enough arguments provided to utest.\n");
    return 1;
  }
  char *test_name = argv[1];
  printf("Starting user test \"%s\".\n", test_name);

#define CHECKRUN_TEST(name)                                                    \
  if (strcmp(test_name, #name) == 0)                                           \
    return test_##name();

  /* Linker set in userspace would be quite difficult to set up, and it feels
     like an overkill to me. */
  CHECKRUN_TEST(mmap);
  CHECKRUN_TEST(sbrk);
  CHECKRUN_TEST(misbehave);
  CHECKRUN_TEST(fd_read);
  CHECKRUN_TEST(fd_devnull);
  CHECKRUN_TEST(fd_multidesc);
  CHECKRUN_TEST(fd_readwrite);
  CHECKRUN_TEST(fd_copy);
  CHECKRUN_TEST(fd_bad_desc);
  CHECKRUN_TEST(fd_open_path);
  CHECKRUN_TEST(fd_dup);
  CHECKRUN_TEST(fd_all);

  printf("No user test \"%s\" available.\n", test_name);
  return 1;
}
