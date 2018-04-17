#define KL_LOG KL_SCHED
#include <klog.h>
#include <stdc.h>
#include <sched.h>
#include <runq.h>
#include <context.h>
#include <time.h>
#include <thread.h>
#include <spinlock.h>
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

  WITH_SPINLOCK(td->td_spin) {
    sched_wakeup(td);
  }
}

void sched_wakeup(thread_t *td) {
  assert(spin_owned(td->td_spin));
  assert(td != thread_self());
  assert(td->td_state == TDS_LOCKED || td->td_state == TDS_SLEEPING ||
         td->td_state == TDS_INACTIVE);

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

/* adjust the priority of a thread,
 * move it to appropriate run queue if necessary */
static void sched_thread_priority(thread_t *td, td_prio_t prio) {
  // nie wiem, czy nie trzeba założyć td_lock
  // ale mutex używa turnstile, a turnstile używa tego...

  if (td->td_prio == prio)
    return;

  /* if thread is on a run queue */
  if (td->td_state == TDS_READY) {
    runq_remove(&runq, td);
    td->td_prio = prio;
    runq_add(&runq, td);
    return;
  }

  td->td_prio = prio;
}

void sched_lend_prio(thread_t *td, td_prio_t prio) {
  td->td_flags |= TDF_BORROWING;
  sched_thread_priority(td, prio);
}

void sched_unlend_prio(thread_t *td, td_prio_t prio) {
  if (prio <= td->td_base_prio) {
    td->td_flags &= ~TDF_BORROWING;
    sched_thread_priority(td, td->td_base_prio);
  } else
    sched_lend_prio(td, prio);
}

void sched_switch(void) {
  if (!sched_active)
    return;

  thread_t *td = thread_self();

  assert(spin_owned(td->td_spin));
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

  /* make sure we reacquire td_spin lock on return to current context */
  td->td_flags |= TDF_NEEDLOCK;

  ctx_switch(td, newtd);
}

void sched_clock(void) {
  assert(intr_disabled());

  thread_t *td = thread_self();

  if (td != PCPU_GET(idle_thread)) {
    WITH_SPINLOCK(td->td_spin) {
      if (--td->td_slice <= 0)
        td->td_flags |= TDF_NEEDSWITCH | TDF_SLICEEND;
    }
  }
}

noreturn void sched_run(void) {
  thread_t *td = thread_self();

  /* Make sure sched_run is launched once per every CPU */
  assert(PCPU_GET(idle_thread) == NULL);

  PCPU_SET(idle_thread, td);

  td->td_name = "idle-thread";
  td->td_slice = 0;

  sched_active = true;

  while (true) {
    WITH_SPINLOCK(td->td_spin) {
      td->td_flags |= TDF_NEEDSWITCH;
    }
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
