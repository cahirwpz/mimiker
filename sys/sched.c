#define KL_LOG KL_SCHED
#include <klog.h>
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
#include <sysinit.h>

static runq_t runq;
static bool sched_active = false;

#define SLICE 10

static void sched_init() {
  runq_init(&runq);
}

void sched_add(thread_t *td) {
  // klog("Add '%s' {%p} thread to scheduler", td->td_name, td);

  td->td_state = TDS_READY;

  /* Idle thread does not get inserted to the runqueue, and it does not require
     increasing its time slice. */
  if (td == PCPU_GET(idle_thread))
    return;

  td->td_slice = SLICE;

  CRITICAL_SECTION {
    runq_add(&runq, td);
    if (td->td_prio > thread_self()->td_prio)
      thread_self()->td_flags |= TDF_NEEDSWITCH;
  }
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

  SCOPED_CRITICAL_SECTION();

  thread_t *td = thread_self();

  td->td_flags &= ~(TDF_SLICEEND | TDF_NEEDSWITCH);

  if (td->td_state == TDS_RUNNING)
    sched_add(td);

  if (newtd == NULL)
    newtd = sched_choose();

  newtd->td_state = TDS_RUNNING;
  timeval_t now = get_uptime();
  timeval_t diff = timeval_sub(&now, &td->td_last_rtime);
  td->td_rtime = timeval_add(&td->td_rtime, &diff);
  newtd->td_last_rtime = now;

  if (td != newtd) {
    td->td_nctxsw++;
    newtd->td_nctxsw++;
    ctx_switch(td, newtd);
  }
}

noreturn void sched_run() {
  thread_t *td = thread_self();

  PCPU_SET(idle_thread, td);

  td->td_name = "idle-thread";
  td->td_slice = 0;
  sched_active = true;

  while (true) {
    td->td_flags |= TDF_NEEDSWITCH;
  }
}

SYSINIT_ADD(sched, sched_init, DEPS("callout"));
