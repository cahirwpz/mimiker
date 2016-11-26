#include <sync.h>
#include <stdc.h>
#include <sched.h>
#include <runq.h>
#include <context.h>
#include <clock.h>
#include <thread.h>
#include <callout.h>
#include <interrupt.h>
#include <mutex.h>
#include <pcpu.h>

static runq_t runq;
static bool sched_active = false;

#define SLICE 10

void sched_init() {
  runq_init(&runq);
}

void sched_add(thread_t *td) {
  // log("Add '%s' {%p} thread to scheduler", td->td_name, td);

  if (td == PCPU_GET(idle_thread))
    return;

  td->td_state = TDS_READY;
  td->td_slice = SLICE;
  cs_enter();

  runq_add(&runq, td);

  if (td->td_prio > thread_self()->td_prio)
    thread_self()->td_flags |= TDF_NEEDSWITCH;
  cs_leave();
}

void sched_remove(thread_t *td) {
  runq_remove(&runq, td);
}

thread_t *sched_choose() {
  thread_t *td = runq_choose(&runq);
  if (td) {
    sched_remove(td);
    return td;
  }
  return PCPU_GET(idle_thread);
}

void sched_clock() {
  thread_t *td = thread_self();

  if (td != PCPU_GET(idle_thread))
    if (--td->td_slice <= 0)
      td->td_flags |= TDF_NEEDSWITCH | TDF_SLICEEND;
}

void sched_yield() {
  sched_switch(NULL);
}

void sched_switch(thread_t *newtd) {
  if (!sched_active)
    return;

  cs_enter();

  thread_t *td = thread_self();

  td->td_flags &= ~(TDF_SLICEEND | TDF_NEEDSWITCH);

  if (td->td_state == TDS_RUNNING)
    sched_add(td);

  if (newtd == NULL)
    newtd = sched_choose();

  newtd->td_state = TDS_RUNNING;
  cs_leave();

  if (td != newtd)
    ctx_switch(td, newtd);
}

noreturn void sched_run() {
  thread_t *td = thread_self(); 

  PCPU_SET(idle_thread, td);

  td->td_slice = 0;
  sched_active = true;

  while (true) {
    td->td_flags |= TDF_NEEDSWITCH;
  }
}

