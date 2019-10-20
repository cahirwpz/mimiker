#include <assert.h>
#include <signal.h>
#include <string.h>
#include <setjmp.h>
#include <stdnoreturn.h>

static jmp_buf jump_buffer;

noreturn static void sigint_handler(int signo) {
  longjmp(jump_buffer, 1);
  assert(0); /* Shouldn't reach here. */
}

int test_sigaction_with_setjmp(void) {
  struct sigaction sa;

  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = sigint_handler;
  assert(sigaction(SIGINT, &sa, NULL) == 0);

  if (setjmp(jump_buffer) != 1) {
    raise(SIGINT);
    assert(0); /* Shouldn't reach here. */
  }

  return 0;
}

static volatile int sigusr1_handled;

static void sigusr1_handler(int signo) {
  sigusr1_handled = 1;
}

int test_sigaction_handler_returns(void) {
  struct sigaction sa;
  
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = sigusr1_handler;
  assert(sigaction(SIGUSR1, &sa, NULL) == 0);

  assert(sigusr1_handled == 0);
  raise(SIGUSR1);
  assert(sigusr1_handled == 1);

  return 0;
}
