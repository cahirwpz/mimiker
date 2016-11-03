#include <stdc.h>
#include <sched.h>
#include <runq.h>
#include <context.h>
#include <clock.h>
#include <thread.h>
#include <callout.h>
#include <interrupt.h>
#include <mutex.h>
#include <sync.h>

static thread_t *idle_thread;
static runq_t runq;
static bool sched_active = false;

#define SLICE 10

void sched_init() {
  runq_init(&runq);
}

void sched_add(thread_t *td) {
  // log("Add '%s' {%p} thread to scheduler", td->td_name, td);

  if (td == idle_thread)
    return;

  td->td_state = TDS_READY;
  td->td_slice = SLICE;

  intr_disable();
  runq_add(&runq, td);

  if (td->td_prio > thread_self()->td_prio)
    thread_self()->td_flags |= TDF_NEEDSWITCH;
  intr_enable();
}

void sched_remove(thread_t *td) {
}

void sched_clock() {
  thread_t *td = thread_self();

  if (td != idle_thread)
    if (--td->td_slice <= 0)
      td->td_flags |= TDF_NEEDSWITCH | TDF_SLICEEND;
}

void sched_turnstile_yield() {
}

void sched_yield() {
  sched_switch(NULL);
}

void sched_switch(thread_t *newtd) {
  if (!sched_active)
    return;

  thread_t *td = thread_self();
  //assert(td->td_csnest == 1);

  td->td_flags &= ~(TDF_SLICEEND | TDF_NEEDSWITCH);

  if (td->td_state == TDS_RUNNING)
    sched_add(td);

  if (newtd == NULL) {
    newtd = runq_choose(&runq);
    if (newtd) {
      runq_remove(&runq, newtd);
    }
    else
      newtd = idle_thread;
  }

  newtd->td_state = TDS_RUNNING;

  if (td != newtd)
    ctx_switch(td, newtd);
}

noreturn void sched_run() {
  idle_thread = thread_self();
  idle_thread->td_slice = 0;
  sched_active = true;

  while (true)
    idle_thread->td_flags |= TDF_NEEDSWITCH;
}
