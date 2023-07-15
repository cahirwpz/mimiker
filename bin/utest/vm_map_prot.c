#include "utest.h"
#include "util.h"

#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int (*func_int_t)(int);

/* XXX In theory compiler may reorder those function and indeed Clang does it
 * when they're marked with `static` quantifier. */

int func_inc(int x) {
  return x + 1;
}

int func_dec(int x) {
  return x - 1;
}

TEST_ADD(vmmap_text_access, 0) {
  func_int_t fun = func_dec;

  assert((uintptr_t)func_inc < (uintptr_t)func_dec);

  debug("func_inc: %p", func_inc);
  debug("func_dec: %p", func_dec);
  debug("Writing data at address: %p", fun);

  char *p = (char *)func_inc;
  char prev_value0 = p[0];

  siginfo_t si;
  EXPECT_SIGNAL(SIGSEGV, &si) {
    /* This memory write should fail and trigger SIGSEGV. */
    p[0] = prev_value0 + 1;
  }
  CLEANUP_SIGNAL();
  CHECK_SIGSEGV(&si, p, SEGV_ACCERR);
  debug("SIGSEGV address: %p", si.si_addr);
  debug("SIGSEGV code: %d", si.si_code);
  return 0;
}

TEST_ADD(vmmap_data_access, 0) {
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
  debug("SIGSEGV address: %p", si.si_addr);
  debug("SIGSEGV code: %d", si.si_code);
  return 0;
}

/* String must be alinged as instuctions because we jump here */
static __aligned(4) const char ro_data_str[] = "String in .rodata section";

TEST_ADD(vmmap_rodata_access, 0) {
  siginfo_t si;

  /* Check write */
  volatile char *c = (char *)ro_data_str;
  debug("mem to write: 0x%p", c);
  EXPECT_SIGNAL(SIGSEGV, &si) {
    /* Writing to rodata segment. */
    *c = 'X';
  }
  CLEANUP_SIGNAL();
  CHECK_SIGSEGV(&si, c, SEGV_ACCERR);
  debug("SIGSEGV address: %p", si.si_addr);
  debug("SIGSEGV code: %d", si.si_code);

  /* Check execute */
  func_int_t fun = (func_int_t)ro_data_str;
  debug("func: 0x%p", fun);
  EXPECT_SIGNAL(SIGSEGV, &si) {
    /* Executing from rodata segment. */
    fun(1);
  }
  CLEANUP_SIGNAL();
  CHECK_SIGSEGV(&si, fun, SEGV_ACCERR);
  debug("SIGSEGV address: %p", si.si_addr);
  debug("SIGSEGV code: %d", si.si_code);
  return 0;
}
