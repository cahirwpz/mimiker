#define KL_LOG KL_SCHED
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/libkern.h>
#include <sys/sched.h>
#include <sys/runq.h>
#include <sys/interrupt.h>
#include <sys/time.h>
#include <sys/thread.h>
#include <sys/mutex.h>
#include <sys/pcpu.h>
#include <sys/turnstile.h>

static MTX_DEFINE(sched_lock, MTX_SPIN);
static runq_t runq;
static bool sched_active = false;

/* clang-format off */
static const char *thread_state_name[] = {
  [TDS_INACTIVE] = "inactive",
  [TDS_READY] = "ready",
  [TDS_RUNNING] = "running",
  [TDS_SLEEPING] = "sleeping",
  [TDS_BLOCKED] = "blocked",
  [TDS_STOPPED] = "stopped",
  [TDS_DEAD] = "dead",
};
/* clang-format on */

#define SLICE 10

void init_sched(void) {
  thread0.td_lock = &sched_lock;
  runq_init(&runq);
}

void sched_add(thread_t *td) {
  klog("Add thread %ld {%p} to scheduler", td->td_tid, td);

  WITH_MTX_LOCK (td->td_lock)
    sched_wakeup(td);
}

void sched_wakeup(thread_t *td) {
  assert(mtx_owned(td->td_lock));
  assert(td != thread_self());
  assert(!td_is_running(td));

  /* Update sleep time. */
  bintime_t now = binuptime();
  bintime_sub(&now, &td->td_last_slptime);
  bintime_add(&td->td_slptime, &now);

  td->td_state = TDS_READY;
  td->td_slice = SLICE;

  runq_add(&runq, td);

  /* Check if we need to reschedule threads. */
  thread_t *oldtd = thread_self();
  if (prio_gt(td->td_prio, oldtd->td_prio))
    oldtd->td_flags |= TDF_NEEDSWITCH;
}

/*! \brief Set thread's active priority \a td_prio to \a prio.
 *
 * \note Must be called with \a td_lock acquired!
 */
static void sched_set_active_prio(thread_t *td, prio_t prio) {
  assert(mtx_owned(td->td_lock));

  if (prio_eq(td->td_prio, prio))
    return;

  if (td_is_ready(td)) {
    /* Thread is on a run queue. */
    runq_remove(&runq, td);
    td->td_prio = prio;
    runq_add(&runq, td);
  } else {
    td->td_prio = prio;
  }
}

void sched_set_prio(thread_t *td, prio_t prio) {
  assert(mtx_owned(td->td_lock));

  td->td_base_prio = prio;

  /* If thread is borrowing priority, don't lower its active priority. */
  if (td_is_borrowing(td) && prio_gt(td->td_prio, prio))
    return;

  prio_t oldprio = td->td_prio;
  sched_set_active_prio(td, prio);

  /* If thread is locked on a turnstile, let the turnstile adjust
   * thread's position on turnstile's \a ts_blocked list. */
  if (td_is_blocked(td) && prio_ne(oldprio, prio))
    turnstile_adjust(td, oldprio);
}

void sched_lend_prio(thread_t *td, prio_t prio) {
  assert(mtx_owned(td->td_lock));
  assert(prio_lt(td->td_prio, prio));

  td->td_flags |= TDF_BORROWING;
  sched_set_active_prio(td, prio);
}

void sched_unlend_prio(thread_t *td, prio_t prio) {
  assert(mtx_owned(td->td_lock));

  if (prio_le(prio, td->td_base_prio)) {
    td->td_flags &= ~TDF_BORROWING;
    sched_set_active_prio(td, td->td_base_prio);
  } else
    sched_lend_prio(td, prio);
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
  td->td_last_rtime = binuptime();
  return td;
}

void sched_switch(void) {
  thread_t *td = thread_self();

  if (!sched_active)
    goto noswitch;

  assert(mtx_owned(td->td_lock));
  assert(!td_is_running(td));

  td->td_flags &= ~(TDF_SLICEEND | TDF_NEEDSWITCH);

  /* Update running time, */
  bintime_t now = binuptime();
  bintime_sub(&now, &td->td_last_rtime);
  bintime_add(&td->td_rtime, &now);

  if (td_is_ready(td)) {
    /* Idle threads need not to be inserted into the run queue. */
    if (td != PCPU_GET(idle_thread))
      runq_add(&runq, td);
  } else if (td_is_sleeping(td)) {
    /* Record when the thread fell asleep. */
    td->td_last_slptime = now;
  } else if (td_is_dead(td) || td_is_stopped(td)) {
    /* Don't add dead or stopped threads to run queue. */
  }

  thread_t *newtd = sched_choose();

  if (td == newtd)
    goto noswitch;

  /* If we got here then a context switch is required. */
  td->td_nctxsw++;

  if (PCPU_GET(no_switch))
    panic("Switching context while interrupts are disabled is forbidden!");

  WITH_INTR_DISABLED {
    klog("Switching from thread %d (%s) to thread %d", td->td_tid,
         thread_state_name[td->td_state], newtd->td_tid);
    mtx_unlock(td->td_lock);
    ctx_switch(td, newtd);
    return;
    /* XXX Right now all local variables belong to thread we switched to! */
  }

noswitch:
  mtx_unlock(td->td_lock);
}

void sched_clock(void) {
  assert(intr_disabled());

  thread_t *td = thread_self();

  if (td != PCPU_GET(idle_thread)) {
    WITH_MTX_LOCK (td->td_lock) {
      if (--td->td_slice <= 0)
        td->td_flags |= TDF_NEEDSWITCH | TDF_SLICEEND;
    }
  }
}

__noreturn void sched_run(void) {
  thread_t *td = thread_self();

  /* Make sure sched_run is launched once per every CPU */
  assert(PCPU_GET(idle_thread) == NULL);

  PCPU_SET(idle_thread, td);

  td->td_name = "idle-thread";
  td->td_slice = 0;

  sched_active = true;

  while (true) {
    WITH_MTX_LOCK (td->td_lock)
      td->td_flags |= TDF_NEEDSWITCH;
  }
}

void sched_maybe_preempt(void) {
  if (preempt_disabled() || intr_disabled())
    return;

  thread_t *td = thread_self();

  mtx_lock(td->td_lock);
  if (td->td_flags & TDF_NEEDSWITCH) {
    td->td_state = TDS_READY;
    sched_switch();
  } else {
    mtx_unlock(td->td_lock);
  }
}

/*
 * Instrumentation in this function would cause KCSAN to fall into an infinite
 * recursion.
 */
__no_kcsan_sanitize bool preempt_disabled(void) {
  thread_t *td = thread_self();
  return td->td_pdnest > 0;
}

void preempt_disable(void) {
  thread_t *td = thread_self();
  td->td_pdnest++;
}

void preempt_enable(void) {
  thread_t *td = thread_self();
  assert(td->td_pdnest > 0);
  td->td_pdnest--;
  sched_maybe_preempt();
}
