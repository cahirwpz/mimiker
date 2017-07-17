#define KL_LOG KL_SCHED
#include <klog.h>
#include <stdc.h>
#include <sched.h>
#include <runq.h>
#include <context.h>
#include <time.h>
#include <thread.h>
#include <callout.h>
#include <interrupt.h>
#include <mutex.h>
#include <pcpu.h>
#include <sysinit.h>

static runq_t runq;
static bool sched_active = false;

#define SLICE 10

static void sched_init(void) {
  runq_init(&runq);
}

void sched_add(thread_t *td) {
  klog("Add thread %ld {%p} to scheduler", td->td_tid, td);

  WITH_NO_PREEMPTION {
    sched_wakeup(td);
  }
}

void sched_wakeup(thread_t *td) {
  assert(preempt_disabled());
  assert(td != thread_self());
  assert(td->td_state == TDS_SLEEPING || td->td_state == TDS_INACTIVE);

  /* Update sleep time. */
  timeval_t now = get_uptime();
  now = timeval_sub(&now, &td->td_last_slptime);
  td->td_slptime = timeval_add(&td->td_slptime, &now);

  td->td_state = TDS_READY;
  td->td_slice = SLICE;

  runq_add(&runq, td);

  /* Check if we need to reschedule threads. */
  thread_t *oldtd = thread_self();
  if (td->td_prio > oldtd->td_prio)
    oldtd->td_flags |= TDF_NEEDSWITCH;
}

/*! \brief Chooses next thread to run.
 *
 * \note Returned thread is marked as running!
 */
static thread_t *sched_choose(void) {
  thread_t *td = runq_choose(&runq);
  if (td == NULL)
    return PCPU_GET(idle_thread);
  runq_remove(&runq, td);
  td->td_state = TDS_RUNNING;
  td->td_last_rtime = get_uptime();
  return td;
}

void sched_switch(void) {
  if (!sched_active)
    return;

  SCOPED_NO_PREEMPTION();

  thread_t *td = thread_self();

  assert(td->td_state != TDS_RUNNING);

  td->td_flags &= ~(TDF_SLICEEND | TDF_NEEDSWITCH);

  /* Update running time, */
  timeval_t now = get_uptime();
  timeval_t diff = timeval_sub(&now, &td->td_last_rtime);
  td->td_rtime = timeval_add(&td->td_rtime, &diff);

  if (td->td_state == TDS_READY) {
    /* Idle threads need not to be inserted into the run queue. */
    if (td != PCPU_GET(idle_thread))
      runq_add(&runq, td);
  } else if (td->td_state == TDS_SLEEPING) {
    /* Record when the thread fell asleep. */
    td->td_last_slptime = now;
  } else if (td->td_state == TDS_DEAD) {
    /* Don't add dead threads to run queue. */
  }

  thread_t *newtd = sched_choose();

  if (td == newtd)
    return;

  /* If we got here then a context switch is required. */
  td->td_nctxsw++;

  ctx_switch(td, newtd);
}

void sched_clock(void) {
  thread_t *td = thread_self();

  if (td != PCPU_GET(idle_thread))
    if (--td->td_slice <= 0)
      td->td_flags |= TDF_NEEDSWITCH | TDF_SLICEEND;
}

noreturn void sched_run(void) {
  thread_t *td = thread_self();

  PCPU_SET(idle_thread, td);

  td->td_name = "idle-thread";
  td->td_slice = 0;

  sched_active = true;

  while (true) {
    td->td_flags |= TDF_NEEDSWITCH;
  }
}

bool preempt_disabled(void) {
  thread_t *td = thread_self();
  return (td->td_pdnest > 0) || intr_disabled();
}

void preempt_disable(void) {
  thread_t *td = thread_self();
  td->td_pdnest++;
}

void preempt_enable(void) {
  thread_t *td = thread_self();
  assert(td->td_pdnest > 0);
  td->td_pdnest--;
}

SYSINIT_ADD(sched, sched_init, DEPS("callout"));
