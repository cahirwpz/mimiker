#include "utest.h"
#include "util.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/mman.h>
#include <stdint.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>
#include <stdlib.h>

#if __SIZEOF_POINTER__ == 4
#define BAD_ADDR_SPAN 0x7fff0000
#define BAD_ADDR_SPAN_LEN 0x20000
#else
#define BAD_ADDR_SPAN 0x00007fffffff0000L
#define BAD_ADDR_SPAN_LEN 0xfffe800000002000
#endif

#define mmap_anon_priv_flags(addr, length, prot, flags)                        \
  mmap((addr), (length), (prot), (flags) | MAP_ANON | MAP_PRIVATE, -1, 0)

#define mmap_anon_priv(addr, length, prot)                                     \
  mmap((addr), (length), (prot), MAP_ANON | MAP_PRIVATE, -1, 0)

#define mmap_anon_prw(addr, length)                                            \
  mmap_anon_priv((addr), (length), PROT_READ | PROT_WRITE)

static void mmap_no_hint(void) {
  void *addr = mmap_anon_prw(NULL, 12345);
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
  void *addr = mmap_anon_prw(TESTADDR, 99);
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
  addr = mmap_anon_prw((void *)BAD_ADDR_SPAN, BAD_ADDR_SPAN_LEN);
  assert(addr == MAP_FAILED);
  assert(errno == EINVAL);
  /* Address lies in low memory, that cannot be mapped. */
  addr = mmap_anon_prw((void *)0x3ff000, 0x1000);
  assert(addr == MAP_FAILED);
  assert(errno == EINVAL);
  /* Hint address is not page aligned. */
  addr = mmap_anon_prw((void *)0x12345678, 0x1000);
  assert(addr == MAP_FAILED);
  assert(errno == EINVAL);
}

static void munmap_good(void) {
  void *addr;
  int result;

  /* mmap & munmap one page */
  addr = mmap_anon_prw(NULL, 0x1000);
  result = munmap(addr, 0x1000);
  assert(result == 0);

  /* munmapping again fails */
  munmap(addr, 0x1000);
  assert(errno == EINVAL);

  /* more pages */
  addr = mmap_anon_prw(NULL, 0x3000);

  result = munmap(addr + 0x1000, 0x1000);
  assert(result == 0);

  result = munmap(addr, 0x1000);
  assert(result == 0);

  result = munmap(addr + 0x2000, 0x1000);
  assert(result == 0);
}

int test_munmap_sigsegv(void) {
  void *addr = mmap_anon_prw(NULL, 0x4000);
  munmap(addr, 0x4000);

  siginfo_t si;
  EXPECT_SIGNAL(SIGSEGV, &si) {
    /* Try to access freed memory. It should raise SIGSEGV */
    int data = *((volatile int *)(addr + 0x2000));
    (void)data;
  }
  CLEANUP_SIGNAL();
  CHECK_SIGSEGV(&si, addr + 0x2000, SEGV_MAPERR);
  return 0;
}

int test_mmap(void) {
  mmap_no_hint();
  mmap_with_hint();
  mmap_bad();
  munmap_good();
  return 0;
}

int test_munmap(void) {
  void *addr;
  int result, child;

  addr =
    mmap(NULL, 0x3000, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
  assert(addr != MAP_FAILED);

  /* write data to parts which will remain mapped after unmap */
  sprintf(addr, "first");
  sprintf(addr + 0x2000, "second");

  result = munmap(addr + 0x1000, 0x1000);
  assert(result == 0);

  /* Now we have to fork to trigger pagefault on both parts of mapped memory. */
  child = fork();
  if (child == 0) {
    int err;
    err = strncmp(addr, "first", 5);
    assert(err == 0);
    err = strncmp(addr + 0x2000, "second", 6);
    assert(err == 0);
    exit(0);
  }

  wait_for_child_exit(child, 0);

  result = munmap(addr, 0x1000);
  assert(result == 0);

  result = munmap(addr + 0x2000, 0x1000);
  assert(result == 0);
  return 0;
}

#define NPAGES 8

int test_mmap_prot_none(void) {
  size_t pgsz = getpagesize();
  size_t size = pgsz * NPAGES;
  volatile void *addr = mmap_anon_priv(NULL, size, PROT_NONE);
  assert(addr != MAP_FAILED);

  siginfo_t si;
  for (int i = 0; i < NPAGES; i++) {
    volatile uint8_t *ptr = addr + i * pgsz;

    EXPECT_SIGNAL(SIGSEGV, &si) {
      (void)(*ptr == 0);
    }
    CLEANUP_SIGNAL();
    CHECK_SIGSEGV(&si, ptr, SEGV_ACCERR);
  }
  return 0;
}

int test_mmap_prot_read(void) {
  size_t pgsz = getpagesize();
  size_t size = pgsz * NPAGES;
  volatile void *addr = mmap_anon_priv(NULL, size, PROT_READ);
  assert(addr != MAP_FAILED);

  /* Ensure mapped area is cleared. */
  for (size_t i = 0; i < size / sizeof(uint32_t); i++)
    assert(((uint32_t *)addr)[i] == 0);

  siginfo_t si;
  for (int i = 0; i < NPAGES; i++) {
    volatile uint8_t *ptr = addr + i * pgsz;

    EXPECT_SIGNAL(SIGSEGV, &si) {
      *ptr = 42;
    }
    CLEANUP_SIGNAL();
    CHECK_SIGSEGV(&si, ptr, SEGV_ACCERR);
    /* Check if nothing changed */
    assert(*ptr == 0);
  }
  return 0;
}

int test_mmap_fixed(void) {
  size_t pgsz = getpagesize();
  void *addr = mmap_anon_priv(NULL, 3 * pgsz, PROT_READ | PROT_WRITE);
  void *new;
  int res;

  res = munmap(addr + pgsz, pgsz);
  assert(res == 0);

  new = mmap_anon_priv_flags(addr + pgsz, pgsz, PROT_READ, MAP_FIXED);
  assert(new == addr + pgsz);

  return 0;
}

int test_mmap_fixed_excl(void) {
  size_t pgsz = getpagesize();
  void *addr = mmap_anon_priv(NULL, 2 * pgsz, PROT_READ);
  void *err;

  err = mmap_anon_priv_flags(addr, pgsz, PROT_READ, MAP_FIXED | MAP_EXCL);
  assert(err == MAP_FAILED && errno == ENOMEM);

  err = mmap_anon_priv_flags(addr, 2 * pgsz, PROT_READ, MAP_FIXED | MAP_EXCL);
  assert(err == MAP_FAILED && errno == ENOMEM);

  err = mmap_anon_priv_flags(addr - pgsz, 3 * pgsz, PROT_READ,
                             MAP_FIXED | MAP_EXCL);
  assert(err == MAP_FAILED && errno == ENOMEM);

  return 0;
}

int test_mmap_fixed_replace(void) {
  size_t pgsz = getpagesize();
  void *addr = mmap_anon_priv(NULL, 2 * pgsz, PROT_READ | PROT_WRITE);
  void *new;
  int err;

  sprintf(addr, "first");
  sprintf(addr + pgsz, "second");

  new = mmap_anon_priv_flags(addr, pgsz, PROT_READ, MAP_FIXED);
  assert(new == addr);

  /* First page should be replaced with new mapping */
  err = strncmp(addr, "first", 5);
  assert(err != 0);

  /* Second page should remain unchanged */
  err = strncmp(addr + pgsz, "second", 6);
  assert(err == 0);

  return 0;
}
