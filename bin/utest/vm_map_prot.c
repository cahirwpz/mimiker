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

static volatile void *sigsegv_address = 0;
static volatile int sigsegv_code = 0;
static jmp_buf return_to;

static void sigsegv_handler(int signo, siginfo_t *info, void *uctx) {
  sigsegv_address = info->si_addr;
  sigsegv_code = info->si_code;
  siglongjmp(return_to, 1);
}

static void setup_sigaction(void) {
  sigaction_t sa;
  memset(&sa, 0, sizeof(sigaction_t));
  sa.sa_sigaction = sigsegv_handler;
  sa.sa_flags = SA_SIGINFO;
  int err = sigaction(SIGSEGV, &sa, NULL);
  assert(err == 0);
}

int test_vmmap_text_w(void) {
  setup_sigaction();

  func_int_t fun = func_dec;

  assert((uintptr_t)func_inc < (uintptr_t)func_dec);

  printf("func_inc: 0x%p\n", func_inc);
  printf("func_dec: 0x%p\n", func_dec);
  printf("Writing data at address: 0x%p\n", fun);

  char *p = (char *)func_inc;
  char prev_value0 = p[0];

  if (sigsetjmp(return_to, 1) == 0) {
    /* This memory write should fail and trigger SIGSEGV. */
    p[0] = prev_value0 + 1;
    assert(0);
  }

  printf("SIGSEGV address: %p\nSIGSEGV code: %d\n", sigsegv_address,
         sigsegv_code);

  assert(sigsegv_address == p);
  assert(sigsegv_code == SEGV_ACCERR);

  signal(SIGSEGV, SIG_DFL);
  return 0;
}

int test_vmmap_data_x(void) {
  setup_sigaction();

  static char func_buf[1024];

  assert((uintptr_t)func_inc < (uintptr_t)func_dec);

  /* TODO: how to properly determine function length? */
  size_t len = (char *)func_dec - (char *)func_inc;
  memcpy(func_buf, func_inc, len);

  func_int_t ff = (func_int_t)func_buf;

  if (sigsetjmp(return_to, 1) == 0) {
    /* Executing function in data segment (without exec prot) */
    int v = ff(1);
    assert(v != 2);
  }

  printf("SIGSEGV address: %p\nSIGSEGV code: %d\n", sigsegv_address,
         sigsegv_code);

  assert(sigsegv_address == ff);
  assert(sigsegv_code == SEGV_ACCERR);

  signal(SIGSEGV, SIG_DFL);
  return 0;
}

static const char *ro_data_str = "String in .rodata section";

int test_vmmap_rodata_w(void) {
  setup_sigaction();

  volatile char *c = (char *)ro_data_str;

  printf("mem to write: 0x%p\n", c);

  if (sigsetjmp(return_to, 1) == 0) {
    /* Writing to rodata segment. */
    *c = 'X';
    assert(*c == 'X');
  }

  printf("SIGSEGV address: %p\nSIGSEGV code: %d\n", sigsegv_address,
         sigsegv_code);

  assert(sigsegv_address == c);
  assert(sigsegv_code == SEGV_ACCERR);

  signal(SIGSEGV, SIG_DFL);
  return 0;
}

int test_vmmap_rodata_x(void) {
  setup_sigaction();

  func_int_t fun = (func_int_t)ro_data_str;

  printf("func: 0x%p\n", fun);

  if (sigsetjmp(return_to, 1) == 0) {
    /* Executing from rodata segment. */
    fun(1);
    assert(0);
  }

  printf("SIGSEGV address: %p\nSIGSEGV code: %d\n", sigsegv_address,
         sigsegv_code);

  /* for some reason this assertion fails on riscv because fail address is fun +
   * 1 */
  assert(sigsegv_address == fun);
  assert(sigsegv_code == SEGV_ACCERR);

  signal(SIGSEGV, SIG_DFL);
  return 0;
}
