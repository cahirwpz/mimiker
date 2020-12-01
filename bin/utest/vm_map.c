#include "utest.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int test_sharing_memory_simple(void) {
  size_t pgsz = getpagesize();
  char *map =
    mmap(NULL, pgsz, PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0);

  assert(map != (char *)MAP_FAILED);

  pid_t pid = fork();
  assert(pid >= 0);

  if (pid == 0) {
    /* child */
    strcpy(map, "Hello, World!");
    exit(0);
  }

  /* parent */
  int status;
  assert(waitpid(-1, &status, 0) == pid);
  assert(WIFEXITED(status));
  assert(strcmp(map, "Hello, World!") == 0);
  assert(munmap(map, pgsz) == 0);
  return 0;
}

int test_sharing_memory_child_and_grandchild(void) {
  size_t pgsz = getpagesize();
  char *map =
    mmap(NULL, pgsz, PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0);

  assert(map != (char *)MAP_FAILED);

  pid_t pid = fork();
  assert(pid >= 0);

  if (pid == 0) {
    /* child */
    pid = fork();
    assert(pid >= 0);

    if (pid == 0) {
      /* grandchild */
      strcpy(map, "Hello from grandchild!");
      exit(0);
    }

    /* child */
    int status;
    assert(waitpid(-1, &status, 0) == pid);
    assert(WIFEXITED(status));
    assert(strcmp(map, "Hello from grandchild!") == 0);
    strcpy(map, "Hello from child!");
    exit(0);
  }

  /* parent */
  int status;
  assert(waitpid(-1, &status, 0) == pid);
  assert(WIFEXITED(status));
  assert(strcmp(map, "Hello from child!") == 0);
  assert(munmap(map, pgsz) == 0);
  return 0;
}

int test_cow_private_simple(void) {
  size_t pgsz = getpagesize();
  char *map = mmap(NULL, pgsz, PROT_READ | PROT_WRITE, MAP_ANON, -1, 0);

  assert(map != (char *)MAP_FAILED);

  strcpy(map, "Hello, World!");

  pid_t pid = fork();

  assert(pid >= 0);

  if (pid == 0) {
    /* child */
    assert(strcmp(map, "Hello, World!") == 0);
    strcpy(map, "Hello from child!");
    assert(strcmp(map, "Hello from child!") == 0);
    exit(0);
  } else {
    /* parent */
    int status;
    assert(waitpid(-1, &status, 0) == pid);
    assert(WIFEXITED(status));

    assert(strcmp(map, "Hello, World!") == 0);
  }

  assert(munmap(map, 4096) == 0);
  return 0;
}
