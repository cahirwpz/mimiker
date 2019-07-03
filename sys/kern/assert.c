#include <sys/cdefs.h>
#include <sys/stdc.h>
#include <sys/ktest.h>
#include <sys/interrupt.h>

noreturn void panic_fail(void) {
  /* We used to terminate the current thread, but that is not a great way of
     panicking, as other threads will continue executing, and our panic might go
     unnoticed. */
  /* Permanently lock the kernel. */
  intr_disable();
  while (1)
    ;
}

void assert_fail(const char *expr, const char *file, unsigned int line) {
  kprintf("Assertion \"%s\" at [%s:%d] failed!\n", expr, file, line);
  if (ktest_test_running_flag)
    ktest_failure();
  else
    panic("Assertion failed.");
}
