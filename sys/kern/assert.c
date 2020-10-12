#include <sys/mimiker.h>
#include <sys/libkern.h>
#include <sys/ktest.h>
#include <sys/interrupt.h>

__noreturn void panic_fail(void) {
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
  ktest_failure_hook();
  panic("Assertion failed.");
}
