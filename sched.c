#include <libkern.h>
#include <sched.h>
#include <runq.h>
#include "context.h"
#include <thread.h>
#include <callout.h>
#include <interrupts.h>
#include <mutex.h>

static runq_t runq;
static callout_t sched_callout[2];
static int current_callout = 0;

void sched_init() {
  runq_init(&runq);
}

static thread_t *sched_choose() {
  thread_t *td = runq_choose(&runq);
  if (td)
    runq_remove(&runq, td);
  return td;
}

void sched_switch() {
  thread_t *current_td = td_running;
  thread_t *new_td = sched_choose();

  callout_stop(&sched_callout[current_callout]);
  current_callout = (current_callout + 1) % 2;
  callout_setup(&sched_callout[current_callout], 5, sched_preempt, NULL);

  if (!new_td) {
    log("new_td is NULL.");
    return;
  }

  sched_add(current_td);
  thread_switch_to(new_td);
}

void sched_preempt() {
  log("Preempting a thread.");
  sched_switch();
}

void sched_yield() {
  log("Yielding a thread.");
  __extension__({ asm("teq $0, $0"); });
}

void sched_run() {
  intr_disable();
  log("Scheduler is run.");
  thread_t *new_td = sched_choose();

  if (!new_td)
    panic("There are no threads to be executed\n");

  current_callout = 0;
  callout_setup(&sched_callout[current_callout], 5, sched_preempt, NULL);

  thread_switch_to(new_td);
  panic("We shouldn't be here");
}

void sched_add(thread_t *td) {
  log("Adding a thread to the runqueue.");
  runq_add(&runq, td);
}


#ifdef _KERNELSPACE

static void demo_thread_1() {
  log("entering demo_thread_1");
  while (true) {
    kprintf("demo_thread_1 running.\n");
    for (int i = 0; i < 20000; i++) {
    };
  }
}

static void demo_thread_2() {
  log("entering demo_thread_2");    
  while (true) {
    kprintf("demo_thread_2 running\n");
    for (int i = 0; i < 20000; i++) {
    };
  }
}

static void demo_thread_3() {
  while (true) {
    kprintf("demo_thread_3 running\n");
    for (int i = 0; i < 20000; i++) {
    };
  }
}

static void demo_thread_4() {
  while (true) {
    kprintf("demo_thread_4 running. Let's yield.\n");
    sched_yield();
  }
}

static void demo_thread_5() {
  while (true) {
    kprintf("demo_thread_5 running. Let's yield.\n");
    sched_yield();
  }
}

int main() {
  sched_init();

  thread_t *td1 = thread_create(demo_thread_1);
  thread_t *td2 = thread_create(demo_thread_2);
  thread_t *td3 = thread_create(demo_thread_3);
  thread_t *td4 = thread_create(demo_thread_4);
  thread_t *td5 = thread_create(demo_thread_5);

  sched_add(td1);
  sched_add(td2);
  sched_add(td3);
  sched_add(td4);
  sched_add(td5);

  sched_run();

  panic("We should never get to this place.\n");
  return 0;
}

#endif // _KERNELSPACE
