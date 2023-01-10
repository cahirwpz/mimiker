#include "utest.h"
#include "util.h"

#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

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
  /* Ensure mapped area is cleared. */
  assert(*(char *)(addr + 10) == 0);
  assert(*(char *)(addr + 50) == 0);
  /* Check whether writing to area works correctly. */
  memset(addr, -1, 99);
}
#undef TESTADDR

static void mmap_bad(void) {
  /* Address range spans user and kernel space. */
  syscall_fail(mmap_anon_prw((void *)BAD_ADDR_SPAN, BAD_ADDR_SPAN_LEN), EINVAL);
  /* Address lies in low memory, that cannot be mapped. */
  syscall_fail(mmap_anon_prw((void *)0x3ff000, 0x1000), EINVAL);
  /* Hint address is not page aligned. */
  syscall_fail(mmap_anon_prw((void *)0x12345678, 0x1000), EINVAL);
}

static void munmap_good(void) {
  void *addr;

  /* mmap & munmap one page */
  addr = mmap_anon_prw(NULL, 0x1000);
  syscall_ok(munmap(addr, 0x1000));

  /* munmapping again is no-op */
  syscall_ok(munmap(addr, 0x1000));

  /* more pages */
  addr = mmap_anon_prw(NULL, 0x3000);
  assert(addr != MAP_FAILED);

  syscall_ok(munmap(addr + 0x1000, 0x1000));
  syscall_ok(munmap(addr, 0x1000));
  syscall_ok(munmap(addr + 0x2000, 0x1000));
}

TEST_ADD(munmap_sigsegv) {
  void *addr = mmap_anon_prw(NULL, 0x4000);

  syscall_ok(munmap(addr, 0x4000));

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
  int child;

  addr =
    mmap(NULL, 0x3000, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
  assert(addr != MAP_FAILED);

  /* write data to parts which will remain mapped after unmap */
  sprintf(addr, "first");
  sprintf(addr + 0x2000, "second");

  syscall_ok(munmap(addr + 0x1000, 0x1000));

  /* Now we have to fork to trigger pagefault on both parts of mapped memory. */
  child = fork();
  if (child == 0) {
    string_eq(addr, "first");
    string_eq(addr + 0x2000, "second");
    exit(0);
  }

  wait_for_child_exit(child, 0);

  syscall_ok(munmap(addr, 0x1000));
  syscall_ok(munmap(addr + 0x2000, 0x1000));
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

  syscall_ok(munmap(addr + pgsz, pgsz));

  new = mmap_anon_priv_flags(addr + pgsz, pgsz, PROT_READ, MAP_FIXED);
  assert(new == addr + pgsz);

  return 0;
}

TEST_ADD(mmap_fixed_excl) {
  size_t pgsz = getpagesize();
  void *addr = mmap_anon_priv(NULL, 2 * pgsz, PROT_READ);

  syscall_fail(
    mmap_anon_priv_flags(addr, pgsz, PROT_READ, MAP_FIXED | MAP_EXCL), ENOMEM);

  syscall_fail(
    mmap_anon_priv_flags(addr, 2 * pgsz, PROT_READ, MAP_FIXED | MAP_EXCL),
    ENOMEM);

  syscall_fail(mmap_anon_priv_flags(addr - pgsz, 3 * pgsz, PROT_READ,
                                    MAP_FIXED | MAP_EXCL),
               ENOMEM);

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
  string_ne(addr, "first");

  /* Second page should remain unchanged */
  string_eq(addr + pgsz, "second");

  return 0;
}

/*
 * Layout of mapped pages:
 * 0       1        2        3        4        5       6
 * +----------------+        +--------+        +-----------------+
 * | first | second |        | fourth |        | sixth | seventh |
 * +----------------+        +--------+        +-----------------+
 */
static void *prepare_layout(size_t pgsz, int prot) {
  void *addr = mmap_anon_priv(NULL, 7 * pgsz, prot);

  syscall_ok(munmap(addr + 2 * pgsz, pgsz));
  syscall_ok(munmap(addr + 4 * pgsz, pgsz));

  return addr;
}

static void *prepare_rw_layout(size_t pgsz) {
  void *addr = prepare_layout(pgsz, PROT_READ | PROT_WRITE);
  if (addr == NULL)
    return NULL;

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
  void *addr = prepare_rw_layout(pgsz);
  int res;

  res = munmap(addr + pgsz, 5 * pgsz);
  syscall_ok(res);

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

  string_eq(addr, "first");
  string_eq(addr + 6 * pgsz, "seventh");

  return 0;
}

/* This test unmaps all entries from prepared layout */
TEST_ADD(munmap_many_2) {
  size_t pgsz = getpagesize();
  void *addr = prepare_rw_layout(pgsz);
  int res;

  res = munmap(addr, 7 * pgsz);
  syscall_ok(res);

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
  void *addr = prepare_rw_layout(pgsz);
  void *new;

  new = mmap_anon_priv_flags(addr + pgsz, 5 * pgsz, PROT_READ, MAP_FIXED);
  assert(new == addr + pgsz);

  string_ne(addr + pgsz, "second");
  string_ne(addr + 3 * pgsz, "fourth");
  string_ne(addr + 5 * pgsz, "sixth");

  string_eq(addr, "first");
  string_eq(addr + 6 * pgsz, "seventh");

  return 0;
}

/* This test unmaps all entries from prepared layout */
TEST_ADD(mmap_fixed_replace_many_2) {
  size_t pgsz = getpagesize();
  void *addr = prepare_rw_layout(pgsz);
  void *new;

  new = mmap_anon_priv_flags(addr, 7 * pgsz, PROT_READ, MAP_FIXED);
  assert(new == addr);

  string_ne(addr, "first");
  string_ne(addr + pgsz, "second");
  string_ne(addr + 3 * pgsz, "fourth");
  string_ne(addr + 5 * pgsz, "sixth");
  string_ne(addr + 6 * pgsz, "seventh");

  return 0;
}

TEST_ADD(mprotect_simple) {
  size_t pgsz = getpagesize();
  void *addr = mmap_anon_priv(NULL, pgsz, PROT_NONE);
  siginfo_t si;

  EXPECT_SIGNAL(SIGSEGV, &si) {
    strcmp(addr, "xxx");
  }
  CLEANUP_SIGNAL();
  CHECK_SIGSEGV(&si, addr, SEGV_ACCERR);

  syscall_ok(mprotect(addr, pgsz, PROT_READ | PROT_WRITE));

  sprintf(addr, "first");
  string_eq(addr, "first");

  return 0;
}

#define check_none_region(si, addr)                                            \
  {                                                                            \
    EXPECT_SIGNAL(SIGSEGV, &(si)) {                                            \
      strcmp((addr), "xxx");                                                   \
    }                                                                          \
    CLEANUP_SIGNAL();                                                          \
    CHECK_SIGSEGV(&(si), (addr), SEGV_ACCERR);                                 \
  }

static void *prepare_none_layout(size_t pgsz) {
  void *addr = prepare_layout(pgsz, PROT_NONE);
  if (addr == NULL)
    return NULL;

  siginfo_t si;

  check_none_region(si, addr);
  check_none_region(si, addr + pgsz);
  check_none_region(si, addr + 3 * pgsz);
  check_none_region(si, addr + 5 * pgsz);
  check_none_region(si, addr + 6 * pgsz);

  return addr;
}

/* This test unmaps entries from prepared layout (second, fourth, sixth) */
TEST_ADD(mprotect_many_1) {
  size_t pgsz = getpagesize();
  void *addr = prepare_none_layout(pgsz);
  siginfo_t si;

  syscall_ok(mprotect(addr + pgsz, 5 * pgsz, PROT_READ | PROT_WRITE));

  check_none_region(si, addr);
  check_none_region(si, addr + 6 * pgsz);

  sprintf(addr + pgsz, "some string");
  string_eq(addr + pgsz, "some string");

  sprintf(addr + 3 * pgsz, "some string");
  string_eq(addr + 3 * pgsz, "some string");

  sprintf(addr + 5 * pgsz, "some string");
  string_eq(addr + 5 * pgsz, "some string");

  return 0;
}

/* This test unmaps all entries from prepared layout */
TEST_ADD(mprotect_many_2) {
  size_t pgsz = getpagesize();
  void *addr = prepare_none_layout(pgsz);

  syscall_ok(mprotect(addr, 7 * pgsz, PROT_READ | PROT_WRITE));

  sprintf(addr, "some string");
  string_eq(addr, "some string");

  sprintf(addr + pgsz, "some string");
  string_eq(addr + pgsz, "some string");

  sprintf(addr + 3 * pgsz, "some string");
  string_eq(addr + 3 * pgsz, "some string");

  sprintf(addr + 5 * pgsz, "some string");
  string_eq(addr + 5 * pgsz, "some string");

  sprintf(addr + 6 * pgsz, "some string");
  string_eq(addr + 6 * pgsz, "some string");

  return 0;
}
