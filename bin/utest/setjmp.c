#include <assert.h>
#include <setjmp.h>
#include <stdio.h>

int test_setjmp(void) {
  jmp_buf jump_buffer;
  setjmp(jump_buffer);

  volatile int ret = setjmp(jump_buffer);
  printf("ret = %d\n", ret);
  fflush(NULL);

  if (ret != 10) {
    ret++;
    longjmp(jump_buffer, ret);
  }

  return 0;
}