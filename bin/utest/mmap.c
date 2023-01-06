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
  assert_fail(addr, EINVAL);
  /* Address lies in low memory, that cannot be mapped. */
  addr = mmap_anon_prw((void *)0x3ff000, 0x1000);
  assert_fail(addr, EINVAL);
  /* Hint address is not page aligned. */
  addr = mmap_anon_prw((void *)0x12345678, 0x1000);
  assert_fail(addr, EINVAL);
}

static void munmap_good(void) {
  void *addr;
  int result;

  /* mmap & munmap one page */
  addr = mmap_anon_prw(NULL, 0x1000);
  result = munmap(addr, 0x1000);
  assert_ok(result);

  /* munmapping again fails */
  result = munmap(addr, 0x1000);
  printf("result: %d\n", result);
  assert(errno == EINVAL);
  // assert_fail(result, EINVAL);

  /* more pages */
  addr = mmap_anon_prw(NULL, 0x3000);
  assert(addr != MAP_FAILED);

  result = munmap(addr + 0x1000, 0x1000);
  assert_ok(result);

  result = munmap(addr, 0x1000);
  assert_ok(result);

  result = munmap(addr + 0x2000, 0x1000);
  assert_ok(result);
}

TEST_ADD(munmap_sigsegv) {
  void *addr = mmap_anon_prw(NULL, 0x4000);
  int res;

  res = munmap(addr, 0x4000);
  assert_ok(res);

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

TEST_ADD(mmap) {
  mmap_no_hint();
  mmap_with_hint();
  mmap_bad();
  munmap_good();
  return 0;
}

TEST_ADD(munmap) {
  void *addr;
  int result, child;

  addr =
    mmap(NULL, 0x3000, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
  assert(addr != MAP_FAILED);

  /* write data to parts which will remain mapped after unmap */
  sprintf(addr, "first");
  sprintf(addr + 0x2000, "second");

  result = munmap(addr + 0x1000, 0x1000);
  assert_ok(result);

  /* Now we have to fork to trigger pagefault on both parts of mapped memory. */
  child = fork();
  if (child == 0) {
    int err;
    err = strcmp(addr, "first");
    assert_ok(err);
    err = strcmp(addr + 0x2000, "second");
    assert_ok(err);
    exit(0);
  }

  wait_for_child_exit(child, 0);

  result = munmap(addr, 0x1000);
  assert_ok(result);

  result = munmap(addr + 0x2000, 0x1000);
  assert_ok(result);
  return 0;
}

#define NPAGES 8

TEST_ADD(mmap_prot_none) {
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

TEST_ADD(mmap_prot_read) {
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

TEST_ADD(mmap_fixed) {
  size_t pgsz = getpagesize();
  void *addr = mmap_anon_priv(NULL, 3 * pgsz, PROT_READ | PROT_WRITE);
  void *new;
  int res;

  res = munmap(addr + pgsz, pgsz);
  assert_ok(res);

  new = mmap_anon_priv_flags(addr + pgsz, pgsz, PROT_READ, MAP_FIXED);
  assert(new == addr + pgsz);

  return 0;
}

TEST_ADD(mmap_fixed_excl) {
  size_t pgsz = getpagesize();
  void *addr = mmap_anon_priv(NULL, 2 * pgsz, PROT_READ);
  void *err;

  err = mmap_anon_priv_flags(addr, pgsz, PROT_READ, MAP_FIXED | MAP_EXCL);
  assert_fail(err, ENOMEM);

  err = mmap_anon_priv_flags(addr, 2 * pgsz, PROT_READ, MAP_FIXED | MAP_EXCL);
  assert_fail(err, ENOMEM);

  err = mmap_anon_priv_flags(addr - pgsz, 3 * pgsz, PROT_READ,
                             MAP_FIXED | MAP_EXCL);
  assert_fail(err, ENOMEM);

  return 0;
}

TEST_ADD(mmap_fixed_replace) {
  size_t pgsz = getpagesize();
  void *addr = mmap_anon_priv(NULL, 2 * pgsz, PROT_READ | PROT_WRITE);
  void *new;

  sprintf(addr, "first");
  sprintf(addr + pgsz, "second");

  new = mmap_anon_priv_flags(addr, pgsz, PROT_READ, MAP_FIXED);
  assert(new == addr);

  /* First page should be replaced with new mapping */
  STRING_NE(addr, "first");

  /* Second page should remain unchanged */
  STRING_EQ(addr + pgsz, "second");

  return 0;
}

/*
 * Layout of mapped pages:
 * 0       1        2        3        4        5       6
 * +----------------+        +--------+        +-----------------+
 * | first | second |        | fourth |        | sixth | seventh |
 * +----------------+        +--------+        +-----------------+
 */
static void *prepare_layout(size_t pgsz) {
  void *addr = mmap_anon_priv(NULL, 7 * pgsz, PROT_READ | PROT_WRITE);
  int res;

  res = munmap(addr + 2 * pgsz, pgsz);
  assert_ok(res);

  res = munmap(addr + 4 * pgsz, pgsz);
  assert_ok(res);

  sprintf(addr, "first");
  sprintf(addr + pgsz, "second");
  sprintf(addr + 3 * pgsz, "fourth");
  sprintf(addr + 5 * pgsz, "sixth");
  sprintf(addr + 6 * pgsz, "seventh");

  return addr;
}

/* This test unmaps entries from prepared layout (second, fourth, sixth) */
TEST_ADD(munmap_many_1) {
  size_t pgsz = getpagesize();
  void *addr = prepare_layout(pgsz);
  int res;

  res = munmap(addr + pgsz, 5 * pgsz);
  assert_ok(res);

  siginfo_t si;
  EXPECT_SIGNAL(SIGSEGV, &si) {
    strcmp(addr + pgsz, "second");
  }
  CLEANUP_SIGNAL();
  CHECK_SIGSEGV(&si, addr + pgsz, SEGV_MAPERR);

  EXPECT_SIGNAL(SIGSEGV, &si) {
    strcmp(addr + 3 * pgsz, "fourth");
  }
  CLEANUP_SIGNAL();
  CHECK_SIGSEGV(&si, addr + 3 * pgsz, SEGV_MAPERR);

  EXPECT_SIGNAL(SIGSEGV, &si) {
    strcmp(addr + 5 * pgsz, "sixth");
  }
  CLEANUP_SIGNAL();
  CHECK_SIGSEGV(&si, addr + 5 * pgsz, SEGV_MAPERR);

  STRING_EQ(addr, "first");
  STRING_EQ(addr + 6 * pgsz, "seventh");

  return 0;
}

/* This test unmaps all entries from prepared layout */
TEST_ADD(munmap_many_2) {
  size_t pgsz = getpagesize();
  void *addr = prepare_layout(pgsz);
  int res;

  res = munmap(addr, 7 * pgsz);
  assert_ok(res);

  siginfo_t si;
  EXPECT_SIGNAL(SIGSEGV, &si) {
    strcmp(addr, "first");
  }
  CLEANUP_SIGNAL();
  CHECK_SIGSEGV(&si, addr, SEGV_MAPERR);

  EXPECT_SIGNAL(SIGSEGV, &si) {
    strcmp(addr + pgsz, "second");
  }
  CLEANUP_SIGNAL();
  CHECK_SIGSEGV(&si, addr + pgsz, SEGV_MAPERR);

  EXPECT_SIGNAL(SIGSEGV, &si) {
    strcmp(addr + 3 * pgsz, "fourth");
  }
  CLEANUP_SIGNAL();
  CHECK_SIGSEGV(&si, addr + 3 * pgsz, SEGV_MAPERR);

  EXPECT_SIGNAL(SIGSEGV, &si) {
    strcmp(addr + 5 * pgsz, "sixth");
  }
  CLEANUP_SIGNAL();
  CHECK_SIGSEGV(&si, addr + 5 * pgsz, SEGV_MAPERR);

  EXPECT_SIGNAL(SIGSEGV, &si) {
    strcmp(addr + 6 * pgsz, "seventh");
  }
  CLEANUP_SIGNAL();
  CHECK_SIGSEGV(&si, addr + 6 * pgsz, SEGV_MAPERR);

  return 0;
}

/* This test unmaps entries from prepared layout (second, fourth, sixth) */
TEST_ADD(mmap_fixed_replace_many_1) {
  size_t pgsz = getpagesize();
  void *addr = prepare_layout(pgsz);
  void *new;

  new = mmap_anon_priv_flags(addr + pgsz, 5 * pgsz, PROT_READ, MAP_FIXED);
  assert(new == addr + pgsz);

  STRING_NE(addr + pgsz, "second");
  STRING_NE(addr + 3 * pgsz, "fourth");
  STRING_NE(addr + 5 * pgsz, "sixth");

  STRING_EQ(addr, "first");
  STRING_EQ(addr + 6 * pgsz, "seventh");

  return 0;
}

/* This test unmaps all entries from prepared layout */
TEST_ADD(mmap_fixed_replace_many_2) {
  size_t pgsz = getpagesize();
  void *addr = prepare_layout(pgsz);
  void *new;

  new = mmap_anon_priv_flags(addr, 7 * pgsz, PROT_READ, MAP_FIXED);
  assert(new == addr);

  STRING_NE(addr, "first");
  STRING_NE(addr + pgsz, "second");
  STRING_NE(addr + 3 * pgsz, "fourth");
  STRING_NE(addr + 5 * pgsz, "sixth");
  STRING_NE(addr + 6 * pgsz, "seventh");

  return 0;
}
