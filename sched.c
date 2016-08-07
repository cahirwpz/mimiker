#include <libkern.h>
#include <sched.h>
#include <runq.h>
#include "context.h"
#include <thread.h>

static runq_t runq;

void sched_init() {
  runq_init(&runq);
}

static thread_t* sched_choose() {
  thread_t* td = runq_choose(&runq);
  runq_remove(&runq, td);
  return td;
}

void sched_run() {
  log("Scheduler is run.");
  thread_t* new_td = sched_choose();

  if (!new_td)
    panic("There are no threads to be executed\n");

  thread_switch_to(new_td);
}

void sched_add(thread_t *td) {
  log("Adding a thread to the runqueue.");
  runq_add(&runq, td);
}

void sched_yield() {
  log("A thread has yielded.");
  thread_t* current_td = td_running;
  thread_t* new_td = sched_choose();

  if (!new_td)
    return;

  sched_add(current_td);
  thread_switch_to(new_td);
}


#ifdef _KERNELSPACE

static void demo_thread_1() {
    
  while (true) {
    kprintf("demo_thread_1 running.\n");
    sched_yield();
  }
}

static void demo_thread_2() {
  while (true) {
    kprintf("demo_thread_2 running\n");
    sched_yield();
  }
}

static void demo_thread_3() {
  while (true) {
    kprintf("demo_thread_3 running\n");
    sched_yield();
  }
}

int main() {
  sched_init();

  thread_t *td1 = thread_create(demo_thread_1);
  thread_t *td2 = thread_create(demo_thread_2);
  thread_t *td3 = thread_create(demo_thread_3);

  sched_add(td1);
  sched_add(td2);
  sched_add(td3);

  sched_run();

  panic("We should never get to this place.\n");
  return 0;
}

#endif // _KERNELSPACE
