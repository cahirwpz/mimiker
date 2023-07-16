#include "utest.h"
#include "util.h"

#include <errno.h>
#include <sched.h>
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
  xmunmap(addr, 0x1000);

  /* munmapping again is no-op */
  xmunmap(addr, 0x1000);

  /* more pages */
  addr = mmap_anon_prw(NULL, 0x3000);
  assert(addr != MAP_FAILED);

  xmunmap(addr + 0x1000, 0x1000);
  xmunmap(addr, 0x1000);
  xmunmap(addr + 0x2000, 0x1000);
}

TEST_ADD(munmap_sigsegv, 0) {
  void *addr = mmap_anon_prw(NULL, 0x4000);

  xmunmap(addr, 0x4000);

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

TEST_ADD(mmap, 0) {
  mmap_no_hint();
  mmap_with_hint();
  mmap_bad();
  munmap_good();
  return 0;
}

TEST_ADD(munmap, 0) {
  void *addr;
  int child;

  addr =
    mmap(NULL, 0x3000, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
  assert(addr != MAP_FAILED);

  /* write data to parts which will remain mapped after unmap */
  strcpy(addr, "first");
  strcpy(addr + 0x2000, "second");

  xmunmap(addr + 0x1000, 0x1000);

  /* Now we have to fork to trigger pagefault on both parts of mapped memory. */
  child = xfork();
  if (child == 0) {
    string_eq(addr, "first");
    string_eq(addr + 0x2000, "second");
    exit(0);
  }

  wait_child_finished(child);

  xmunmap(addr, 0x1000);
  xmunmap(addr + 0x2000, 0x1000);
  return 0;
}

static volatile int sigcont_handled = 0;

static void sigcont_handler(int signo) {
  debug("sigcont handled!");
  sigcont_handled = 1;
}

TEST_ADD(mmap_private, 0) {
  size_t pgsz = getpagesize();
  xsignal(SIGCONT, sigcont_handler);

  /* mmap & munmap one page */
  char *addr = mmap_anon_prw(NULL, pgsz);
  assert(addr != (char *)MAP_FAILED);

  pid_t pid = xfork();

  strcpy(addr, "parent");

  if (pid == 0) {
    /* child */
    debug("Child read: '%s'", addr);
    /* Check and modify. */
    string_eq(addr, "parent");
    strcpy(addr, "child");
    debug("Child written: '%s'", addr);

    /* Wait for parent to check and modify its memory. */
    xkill(getppid(), SIGCONT);
    while (!sigcont_handled)
      sched_yield();

    debug("Child read again: '%s'", addr);
    string_eq(addr, "child");
    exit(0);
  }

  /* Wait for child to check and modify its memory. */
  while (!sigcont_handled)
    sched_yield();

  debug("Parent read: '%s'", addr);
  /* Check and modify. */
  string_eq(addr, "parent");
  strcpy(addr, "parent again");

  /* Resume child. */
  xkill(pid, SIGCONT);

  wait_child_finished(pid);
  return 0;
}

static volatile int sigcont_handled = 0;

static void sigcont_handler(int signo) {
  printf("sigcont handled!\n");
  sigcont_handled = 1;
}

TEST_ADD(mmap_private) {
  signal(SIGCONT, sigcont_handler);
  char *addr;
  int ppid = getpid();
  size_t pgsz = getpagesize();

  /* mmap & munmap one page */
  addr = mmap_anon_prw(NULL, pgsz);
  assert(addr != (char *)MAP_FAILED);

  pid_t pid = fork();
  assert(pid >= 0);

  sprintf(addr, "parent");

  if (pid == 0) {
    /* child */
    printf("Child read: '%s'\n", addr);
    /* Check and modify. */
    string_eq(addr, "parent");
    sprintf(addr, "child");
    printf("Child written: '%s'\n", addr);

    /* Wait for parent to check and modify its memory. */
    kill(ppid, SIGCONT);
    while (!sigcont_handled)
      sched_yield();

    printf("Child read again: '%s'\n", addr);
    string_eq(addr, "child");
    exit(0);
  }

  /* Wait for child to check and modify its memory. */
  while (!sigcont_handled)
    sched_yield();

  printf("Parent read: '%s'\n", addr);
  /* Check and modify. */
  string_eq(addr, "parent");
  sprintf(addr, "parent again");

  /* Resume child. */
  kill(pid, SIGCONT);

  wait_for_child_exit(pid, 0);
  return 0;
}

#define NPAGES 8

TEST_ADD(mmap_prot_none, 0) {
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

TEST_ADD(mmap_prot_read, 0) {
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

TEST_ADD(mmap_fixed, 0) {
  size_t pgsz = getpagesize();
  void *addr = mmap_anon_priv(NULL, 3 * pgsz, PROT_READ | PROT_WRITE);
  void *new;

  xmunmap(addr + pgsz, pgsz);

  new = mmap_anon_priv_flags(addr + pgsz, pgsz, PROT_READ, MAP_FIXED);
  assert(new == addr + pgsz);

  return 0;
}

TEST_ADD(mmap_fixed_excl, 0) {
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

TEST_ADD(mmap_fixed_replace, 0) {
  size_t pgsz = getpagesize();
  void *addr = mmap_anon_priv(NULL, 2 * pgsz, PROT_READ | PROT_WRITE);
  void *new;

  strcpy(addr, "first");
  strcpy(addr + pgsz, "second");

  new = mmap_anon_priv_flags(addr, pgsz, PROT_READ, MAP_FIXED);
  assert(new == addr);

  /* First page should be replaced with new mapping */
  string_ne(addr, "first");

  /* Second page should remain unchanged */
  string_eq(addr + pgsz, "second");

  return 0;
}

static void *prepare_rw_layout(size_t pgsz) {
  void *addr = prepare_layout(pgsz, PROT_READ | PROT_WRITE);
  if (addr == NULL)
    return NULL;

  strcpy(addr, "first");
  strcpy(addr + pgsz, "second");
  strcpy(addr + 3 * pgsz, "fourth");
  strcpy(addr + 5 * pgsz, "sixth");
  strcpy(addr + 6 * pgsz, "seventh");

  return addr;
}

/* This test unmaps entries from prepared layout (second, fourth, sixth) */
TEST_ADD(munmap_many_1, 0) {
  size_t pgsz = getpagesize();
  void *addr = prepare_rw_layout(pgsz);

  xmunmap(addr + pgsz, 5 * pgsz);

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
TEST_ADD(munmap_many_2, 0) {
  size_t pgsz = getpagesize();
  void *addr = prepare_rw_layout(pgsz);

  xmunmap(addr, 7 * pgsz);

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
TEST_ADD(mmap_fixed_replace_many_1, 0) {
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
TEST_ADD(mmap_fixed_replace_many_2, 0) {
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
