#include "utest.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/mman.h>

static void mmap_no_hint(void) {
  void *addr = mmap(NULL, 12345, PROT_READ | PROT_WRITE, MAP_ANON);
  assert(addr != MAP_FAILED);
  printf("mmap returned pointer: %p\n", addr);
  /* Ensure mapped area is cleared. */
  assert(*(char *)(addr + 10) == 0);
  assert(*(char *)(addr + 1000) == 0);
  /* Check whether writing to area works correctly. */
  memset(addr, -1, 12345);
}

#define TESTADDR (void *)0x12345000
static void mmap_with_hint(void) {
  /* Provide a hint address that is page aligned. */
  void *addr = mmap(TESTADDR, 99, PROT_READ | PROT_WRITE, MAP_ANON);
  assert(addr != MAP_FAILED);
  assert(addr >= TESTADDR);
  printf("mmap returned pointer: %p\n", addr);
  /* Ensure mapped area is cleared. */
  assert(*(char *)(addr + 10) == 0);
  assert(*(char *)(addr + 50) == 0);
  /* Check whether writing to area works correctly. */
  memset(addr, -1, 99);
}
#undef TESTADDR

static void mmap_bad(void) {
  void *addr;
  /* Address range spans user and kernel space. */
  addr = mmap((void *)0x7fff0000, 0x20000, PROT_READ | PROT_WRITE, MAP_ANON);
  assert(addr == MAP_FAILED);
  assert(errno == EINVAL);
  /* Address lies in low memory, that cannot be mapped. */
  addr = mmap((void *)0x3ff000, 0x1000, PROT_READ | PROT_WRITE, MAP_ANON);
  assert(addr == MAP_FAILED);
  assert(errno == EINVAL);
  /* Hint address is not page aligned. */
  addr = mmap((void *)0x12345678, 0x1000, PROT_READ | PROT_WRITE, MAP_ANON);
  assert(addr == MAP_FAILED);
  assert(errno == EINVAL);
}

int test_mmap() {
  mmap_no_hint();
  mmap_with_hint();
  mmap_bad();
  return 0;
}
