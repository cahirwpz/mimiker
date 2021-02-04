#include <sys/klog.h>
#include <sys/libkern.h>
#include <sys/ktest.h>
#include <sys/sched.h>
#include <sys/interrupt.h>

/*
 * We used to terminate the current thread. That's not a great way to panic,
 * since other threads will continue executing, so our panic might go unnoticed.
 */
__noreturn void panic(const char *fmt, ...) {
  /* Disable preemption and print the message. */
  preempt_disable();

  va_list ap;
  va_start(ap, fmt);
  vkprintf(fmt, ap);
  va_end(ap);
  ktest_failure_hook();

  /* Permanently lock the kernel. */
  intr_disable();
  for (;;)
    continue;
}

__noreturn void assert_fail(const char *expr, const char *file, unsigned line) {
  panic("Assertion \"%s\" at [%s:%d] failed!", expr, file, line);
}
