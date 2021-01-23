#include "utest.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/mman.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>

#ifdef __mips__
#define BAD_ADDR_SPAN 0x7fff0000
#define BAD_ADDR_SPAN_LEN 0x20000
#endif

#ifdef __aarch64__
#define BAD_ADDR_SPAN 0x00007fffffff0000L
#define BAD_ADDR_SPAN_LEN 0xfffe800000002000
#endif

#define mmap_anon_prw(addr, length)                                            \
  mmap((addr), (length), PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0)

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
  addr = mmap_anon_prw(NULL, 0x5000);

  result = munmap(addr, 0x2000);
  assert(result == 0);

  result = munmap(addr + 0x2000, 0x3000);
  assert(result == 0);
}

/* Don't call this function in this module */
int test_munmap_sigsegv(void) {
  void *addr = mmap_anon_prw(NULL, 0x4000);
  munmap(addr, 0x4000);

  /* Try to access freed memory. It should raise SIGSEGV */
  int data = *((volatile int *)(addr + 0x2000));
  (void)data;
  return 1;
}

int test_mmap(void) {
  mmap_no_hint();
  mmap_with_hint();
  mmap_bad();
  munmap_good();
  return 0;
}

static volatile int sigsegv_handled = 0;
static jmp_buf return_to;
static void sigsegv_handler(int signo) {
  sigsegv_handled++;
  siglongjmp(return_to, 5);
}
int test_mmap_readonly(void) {
  size_t pgsz = getpagesize();
  signal(SIGSEGV, sigsegv_handler);
  void *addr = mmap(NULL, pgsz * 8, PROT_READ, MAP_ANON | MAP_PRIVATE, -1, 0);
  assert(addr != MAP_FAILED);

  /* Ensure mapped area is cleared. */
  assert(*(char *)(addr + 100) == 0);
  assert(*(char *)(addr + 1000) == 0);

  sigsegv_handled = 0;

  if (sigsetjmp(return_to, 1) == 0) {
    *(char *)addr = '9';
  }

  assert(sigsegv_handled == 1);
  assert(*(char *)addr == 0);

  /* restore original behavior */
  signal(SIGSEGV, SIG_DFL);

  return 0;
}