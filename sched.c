#include <stdc.h>
#include <clock.h>
#include <thread.h>
#include <sched.h>

static void demo_thread_1() {
  while (true) {
    realtime_t now = clock_get();
    kprintf("[%8zu] Running '%s' thread.\n", (size_t)now,
            thread_self()->td_name);
    while (clock_get() < now + 20);
  }
}

static void demo_thread_2() {
  kprintf("Running '%s' thread. Let's yield!\n", thread_self()->td_name);
  sched_yield();
  demo_thread_1();
}

int main() {
  sched_init();

  thread_t *t1 = thread_create("t1", demo_thread_1);
  thread_t *t2 = thread_create("t2", demo_thread_1);
  thread_t *t3 = thread_create("t3", demo_thread_1);
  thread_t *t4 = thread_create("t4", demo_thread_2);
  thread_t *t5 = thread_create("t5", demo_thread_2);

  sched_add(t1);
  sched_add(t2);
  sched_add(t3);
  sched_add(t4);
  sched_add(t5);

  sched_run();
}
