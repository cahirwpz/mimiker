#include "utest.h"
#include "util.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

TEST_ADD(sharing_memory_simple) {
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
  wait_for_child_exit(pid, 0);
  assert(strcmp(map, "Hello, World!") == 0);
  assert(munmap(map, pgsz) == 0);
  return 0;
}

TEST_ADD(sharing_memory_child_and_grandchild) {
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
    wait_for_child_exit(pid, 0);
    assert(strcmp(map, "Hello from grandchild!") == 0);
    strcpy(map, "Hello from child!");
    exit(0);
  }

  /* parent */
  wait_for_child_exit(pid, 0);
  assert(strcmp(map, "Hello from child!") == 0);
  assert(munmap(map, pgsz) == 0);
  return 0;
}
