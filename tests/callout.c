#include <stdc.h>
#include <callout.h>

static void callout_foo(void *arg) {
  kprintf("Someone executed me! After %d ticks.\n", *((int *)arg));
}

int main() {
  callout_t callout[10];
  int timeouts[10] = {2, 5, 3, 1, 6, 3, 7, 10, 5, 3};
  for (int i = 0; i < 10; i++)
    callout_setup(&callout[i], timeouts[i], callout_foo, (void *)&timeouts[i]);

  for (int i = 0; i < 10; i++) {
    kprintf("calling callout_process()\n");
    callout_process(i);
  }

  return 0;
}
