#include <assert.h>
#include <setjmp.h>
#include <stdnoreturn.h>

/* TODO: Check that setjmp & longjmp restore the signal mask.
   Currently we don't support signal masks in the kernel. */

#define LOCAL_VALUE 0xDEAD10CC

static jmp_buf jump_buffer;

noreturn static void do_longjmp(int count) {
  longjmp(jump_buffer, count);
  assert(0); /* Shouldn't reach here. */
}

int test_setjmp(void) {
  unsigned int local_var = LOCAL_VALUE;
  volatile int count = 0;

  if (setjmp(jump_buffer) != 10) {
    do_longjmp(++count);
  }

  assert(count == 10);
  assert(local_var == LOCAL_VALUE);

  return 0;
}
