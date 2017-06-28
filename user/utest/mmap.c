#include "utest.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/mman.h>

int test_mmap() {
  void *addr;
  addr = mmap((void *)0x0, 12345, PROT_READ | PROT_WRITE, MMAP_ANON);
  assert(addr != (void *)-1);
  printf("mmap returned pointer: %p\n", addr);
  /* Ensure mapped area is cleared. */
  assert(*(char *)(addr + 10) == 0);
  assert(*(char *)(addr + 1000) == 0);
  /* Check whether writing to area works correctly. */
  memset(addr, -1, 12345);
  /* Provide a hint address that is not page aligned or anything. */
  addr = mmap((void *)0x12345678, 99, PROT_READ | PROT_WRITE, MMAP_ANON);
  assert(addr != (void *)-1);
  printf("mmap returned pointer: %p\n", addr);
  /* Ensure mapped area is cleared. */
  assert(*(char *)(addr + 10) == 0);
  assert(*(char *)(addr + 50) == 0);
  /* Check whether writing to area works correctly. */
  memset(addr, -1, 99);
  return 0;
}
