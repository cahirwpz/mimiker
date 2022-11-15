#include "utest.h"
#include "util.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef int (*func_int_t)(int);

/* XXX In theory compiler may reorder those function and indeed Clang does it
 * when they're marked with `static` quantifier. */

int func_inc(int x) {
  return x + 1;
}

int func_dec(int x) {
  return x - 1;
}

int test_vmmap_text_w(void) {
  func_int_t fun = func_dec;

  assert((uintptr_t)func_inc < (uintptr_t)func_dec);

  printf("func_inc: 0x%p\n", func_inc);
  printf("func_dec: 0x%p\n", func_dec);
  printf("Writing data at address: 0x%p\n", fun);

  char *p = (char *)func_inc;
  char prev_value0 = p[0];
  char prev_value1 = p[1];

  /* These memory writes should fail */
  p[0] = prev_value0 + 1;
  p[1] = prev_value1 + 1;

  /* Assertions will fail if memory writes succeeded. */
  assert(p[0] == prev_value0);
  assert(p[1] == prev_value1);
  return -1;
}

int test_vmmap_data_x(void) {
  static char func_buf[1024];

  assert((uintptr_t)func_inc < (uintptr_t)func_dec);

  /* TODO: how to properly determine function length? */
  size_t len = (char *)func_dec - (char *)func_inc;
  memcpy(func_buf, func_inc, len);

  func_int_t ff = (func_int_t)func_buf;

  /* Executing function in data segment (without exec prot) */
  int v = ff(1);

  assert(v != 2);
  assert(0);
  return -1;
}

static const char *ro_data_str = "String in .rodata section";

int test_vmmap_rodata_w(void) {
  volatile char *c = (char *)ro_data_str;

  printf("mem to write: 0x%p\n", c);

  /* Writing to rodata segment. */
  *c = 'X';

  assert(0);
  return -1;
}

int test_vmmap_rodata_x(void) {
  func_int_t fun = (func_int_t)ro_data_str;

  printf("func: 0x%p\n", fun);

  /* Executing from rodata segment. */
  fun(1);

  assert(0);
  return -1;
}
