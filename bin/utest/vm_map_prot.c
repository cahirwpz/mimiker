#include "utest.h"
#include "util.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <setjmp.h>
#include <signal.h>

typedef int (*func_int_t)(int);

/* XXX In theory compiler may reorder those function and indeed Clang does it
 * when they're marked with `static` quantifier. */

int func_inc(int x) {
  return x + 1;
}

int func_dec(int x) {
  return x - 1;
}

int test_vmmap_text_access(void) {
  func_int_t fun = func_dec;

  assert((uintptr_t)func_inc < (uintptr_t)func_dec);

  printf("func_inc: 0x%p\n", func_inc);
  printf("func_dec: 0x%p\n", func_dec);
  printf("Writing data at address: 0x%p\n", fun);

  char *p = (char *)func_inc;
  char prev_value0 = p[0];

  siginfo_t si;
  EXPECT_SIGNAL(SIGSEGV, &si) {
    /* This memory write should fail and trigger SIGSEGV. */
    p[0] = prev_value0 + 1;
  }
  CLEANUP_SIGNAL();
  CHECK_SIGSEGV(&si, p, SEGV_ACCERR);
  printf("SIGSEGV address: %p\nSIGSEGV code: %d\n", si.si_addr, si.si_code);
  return 0;
}

int test_vmmap_data_access(void) {
  static char func_buf[1024];

  assert((uintptr_t)func_inc < (uintptr_t)func_dec);

  /* TODO: how to properly determine function length? */
  size_t len = (char *)func_dec - (char *)func_inc;
  memcpy(func_buf, func_inc, len);

  func_int_t ff = (func_int_t)func_buf;

  siginfo_t si;
  EXPECT_SIGNAL(SIGSEGV, &si) {
    /* Executing function in data segment (without exec prot) */
    int v = ff(1);
    (void)v;
  }
  CLEANUP_SIGNAL();
  CHECK_SIGSEGV(&si, ff, SEGV_ACCERR);
  printf("SIGSEGV address: %p\nSIGSEGV code: %d\n", si.si_addr, si.si_code);
  return 0;
}

/* String must be alinged as instuctions because we jump here */
static __aligned(4) const char ro_data_str[] = "String in .rodata section";

int test_vmmap_rodata_access(void) {
  siginfo_t si;

  /* Check write */
  volatile char *c = (char *)ro_data_str;
  printf("mem to write: 0x%p\n", c);
  EXPECT_SIGNAL(SIGSEGV, &si) {
    /* Writing to rodata segment. */
    *c = 'X';
  }
  CLEANUP_SIGNAL();
  CHECK_SIGSEGV(&si, c, SEGV_ACCERR);
  printf("SIGSEGV address: %p\nSIGSEGV code: %d\n", si.si_addr, si.si_code);

  /* Check execute */
  func_int_t fun = (func_int_t)ro_data_str;
  printf("func: 0x%p\n", fun);
  EXPECT_SIGNAL(SIGSEGV, &si) {
    /* Executing from rodata segment. */
    fun(1);
  }
  CLEANUP_SIGNAL();
  CHECK_SIGSEGV(&si, fun, SEGV_ACCERR);
  printf("SIGSEGV address: %p\nSIGSEGV code: %d\n", si.si_addr, si.si_code);
  return 0;
}
