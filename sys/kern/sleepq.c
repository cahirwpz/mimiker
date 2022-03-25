#define KL_LOG KL_SLEEPQ
#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/queue.h>
#include <sys/libkern.h>
#include <sys/sleepq.h>
#include <sys/pool.h>
#include <sys/sched.h>
#include <sys/spinlock.h>
#include <sys/thread.h>
#include <sys/interrupt.h>
#include <sys/errno.h>
#include <sys/callout.h>

#define SC_TABLESIZE 256 /* Must be power of 2. */
#define SC_MASK (SC_TABLESIZE - 1)
#define SC_SHIFT 8
#define SC_HASH(wc)                                                            \
  ((((uintptr_t)(wc) >> SC_SHIFT) ^ (uintptr_t)(wc)) & SC_MASK)
#define SC_LOOKUP(wc) &sleepq_chains[SC_HASH(wc)]

/*! \brief bucket of sleep queues */
typedef struct sleepq_chain {
  spin_t sc_lock;
  TAILQ_HEAD(, sleepq) sc_queues; /*!< list of sleep queues */
} sleepq_chain_t;

static sleepq_chain_t sleepq_chains[SC_TABLESIZE];

static sleepq_chain_t *sc_acquire(void *wchan) {
  sleepq_chain_t *sc = SC_LOOKUP(wchan);
  spin_lock(&sc->sc_lock);
  return sc;
}

static bool sc_owned(sleepq_chain_t *sc) {
  return spin_owned(&sc->sc_lock);
}

static void sc_release(sleepq_chain_t *sc) {
  spin_unlock(&sc->sc_lock);
}

/*! \brief stores all threads sleeping on the same resource.
 *
 * All fields below are protected by corresponding sc_lock. */
typedef struct sleepq {
  TAILQ_ENTRY(sleepq) sq_entry;    /*!< link on sleepq_chain */
  TAILQ_HEAD(, sleepq) sq_free;    /*!< unused sleep queue records */
  TAILQ_HEAD(, thread) sq_blocked; /*!< blocked threads */
  unsigned sq_nblocked;            /*!< number of blocked threads */
  void *sq_wchan;                  /*!< associated waiting channel */
} sleepq_t;

static void sq_ctor(sleepq_t *sq) {
  TAILQ_INIT(&sq->sq_blocked);
  TAILQ_INIT(&sq->sq_free);
  sq->sq_nblocked = 0;
  sq->sq_wchan = NULL;
}

void init_sleepq(void) {
  memset(sleepq_chains, 0, sizeof(sleepq_chains));

  for (int i = 0; i < SC_TABLESIZE; i++) {
    sleepq_chain_t *sc = &sleepq_chains[i];
    spin_init(&sc->sc_lock, 0);
    TAILQ_INIT(&sc->sc_queues);
  }
}

static POOL_DEFINE(P_SLEEPQ, "sleepq", sizeof(sleepq_t));

sleepq_t *sleepq_alloc(void) {
  sleepq_t *sq = pool_alloc(P_SLEEPQ, M_ZERO);
  sq_ctor(sq);
  return sq;
}

void sleepq_destroy(sleepq_t *sq) {
  pool_free(P_SLEEPQ, sq);
}

/*! \brief Lookup the sleep queue associated with \a wchan.
 *
 * \return NULL if no queue is found
 *
 * \warning returned sleep queue is locked!
 */
static sleepq_t *sq_lookup(sleepq_chain_t *sc, void *wchan) {
  assert(sc_owned(sc));

  sleepq_t *sq;
  TAILQ_FOREACH (sq, &sc->sc_queues, sq_entry) {
    if (sq->sq_wchan == wchan)
      return sq;
  }

  return NULL;
}

/* XXX For gdb use only !!! */
static __used sleepq_t *sleepq_lookup(void *wchan) {
  sleepq_chain_t *sc = SC_LOOKUP(wchan);
  sleepq_t *sq;
  TAILQ_FOREACH (sq, &sc->sc_queues, sq_entry) {
    if (sq->sq_wchan == wchan)
      return sq;
  }

  return NULL;
}

static void sq_enter(thread_t *td, sleepq_chain_t *sc, void *wchan,
                     const void *waitpt) {
  assert(sc_owned(sc));
  assert(spin_owned(td->td_lock));

  klog("Thread %u goes to sleep on %p at pc=%p", td->td_tid, wchan, waitpt);

  assert(td->td_wchan == NULL);
  assert(td->td_waitpt == NULL);
  assert(td->td_sleepqueue != NULL);

  sleepq_t *td_sq = td->td_sleepqueue;

  assert(TAILQ_EMPTY(&td_sq->sq_blocked));
  assert(TAILQ_EMPTY(&td_sq->sq_free));
  assert(td_sq->sq_nblocked == 0);

  sleepq_t *sq = sq_lookup(sc, wchan);

  if (sq == NULL) {
    /* No sleep queue for this waiting channel.
     * We take current thread's sleep queue and use it for that purpose. */
    sq = td_sq;
    sq->sq_wchan = wchan;
    TAILQ_INSERT_HEAD(&sc->sc_queues, sq, sq_entry);
  } else {
    /* A sleep queue for the waiting channel already exists!
     * We add this thread's sleepqueue to the free list. */
    TAILQ_INSERT_HEAD(&sq->sq_free, td->td_sleepqueue, sq_entry);
  }

  TAILQ_INSERT_TAIL(&sq->sq_blocked, td, td_sleepq);
  sq->sq_nblocked++;
  sc_release(sc);

  td->td_wchan = wchan;
  td->td_waitpt = waitpt;
  td->td_sleepqueue = NULL;
  td->td_state = TDS_SLEEPING;
  sched_switch();
}

static void sq_leave(thread_t *td, sleepq_chain_t *sc, sleepq_t *sq) {
  klog("Wakeup thread %u from %p at pc=%p", td->td_tid, td->td_wchan,
       td->td_waitpt);

  assert(sc_owned(sc));
  assert(spin_owned(td->td_lock));

  assert(td->td_wchan != NULL);
  assert(td->td_sleepqueue == NULL);
  assert(sq->sq_nblocked >= 1);

  TAILQ_REMOVE(&sq->sq_blocked, td, td_sleepq);
  sq->sq_nblocked--;

  if (TAILQ_EMPTY(&sq->sq_blocked)) {
    /* If it was the last thread on this sleep queue, \a td gets it. */
    assert(sq->sq_nblocked == 0);
    sq->sq_wchan = NULL;
    /* Remove the sleep queue from the chain. */
    TAILQ_REMOVE(&sc->sc_queues, sq, sq_entry);
  } else {
    /* Otherwise \a td gets a sleep queue from the free list. */
    assert(sq->sq_nblocked > 0);
    assert(!TAILQ_EMPTY(&sq->sq_free));
    sq = TAILQ_FIRST(&sq->sq_free);
    /* Remove the sleep queue from the free list. */
    TAILQ_REMOVE(&sq->sq_free, sq, sq_entry);
  }

  td->td_wchan = NULL;
  td->td_waitpt = NULL;
  td->td_sleepqueue = sq;
  sched_wakeup(td);
}

/* Remove a thread from the sleep queue and resume it. */
static bool sq_wakeup(thread_t *td, sleepq_chain_t *sc, sleepq_t *sq,
                      int wakeup) {
  assert(sc_owned(sc));

  WITH_SPIN_LOCK (td->td_lock) {
    if ((wakeup == EINTR) && (td->td_flags & TDF_SLPINTR)) {
      td->td_flags &= ~TDF_SLPTIMED; /* Not woken up by timeout. */
    } else if ((wakeup == ETIMEDOUT) && (td->td_flags & TDF_SLPTIMED)) {
      td->td_flags &= ~TDF_SLPINTR; /* Not woken up by signal. */
    } else if (wakeup == 0) {
      td->td_flags &= ~(TDF_SLPINTR | TDF_SLPTIMED); /* Regular wakeup. */
    } else {
      return false;
    }
    sq_leave(td, sc, sq);
  }

  return true;
}

bool sleepq_signal(void *wchan) {
  sleepq_chain_t *sc = sc_acquire(wchan);
  sleepq_t *sq = sq_lookup(sc, wchan);

  if (sq == NULL) {
    sc_release(sc);
    return false;
  }

  thread_t *td, *best_td = TAILQ_FIRST(&sq->sq_blocked);
  TAILQ_FOREACH (td, &sq->sq_blocked, td_sleepq) {
    /* Search for thread with highest priority */
    if (prio_gt(td->td_prio, best_td->td_prio))
      best_td = td;
  }

  sq_wakeup(best_td, sc, sq, 0);
  sc_release(sc);

  sched_maybe_preempt();
  return true;
}

bool sleepq_broadcast(void *wchan) {
  sleepq_chain_t *sc = sc_acquire(wchan);
  sleepq_t *sq = sq_lookup(sc, wchan);

  if (sq == NULL) {
    sc_release(sc);
    return false;
  }

  thread_t *td;
  TAILQ_FOREACH (td, &sq->sq_blocked, td_sleepq)
    sq_wakeup(td, sc, sq, 0);
  sc_release(sc);

  sched_maybe_preempt();
  return true;
}

static bool _sleepq_abort(thread_t *td, int wakeup) {
  sleepq_chain_t *sc = sc_acquire(td->td_wchan);
  sleepq_t *sq = sq_lookup(sc, td->td_wchan);

  if (sq == NULL) {
    sc_release(sc);
    return false;
  }

  /* Remove a thread from the sleep queue and resume it. */
  bool aborted = sq_wakeup(td, sc, sq, wakeup);
  sc_release(sc);

  /* If we woke up higher priority thread, we should switch to it immediately.
   * This is useful if `_sleepq_abort` gets called in thread context and
   * preemption is enabled. */
  sched_maybe_preempt();
  return aborted;
}

bool sleepq_abort(thread_t *td) {
  return _sleepq_abort(td, EINTR);
}

void sleepq_wait(void *wchan, const void *waitpt) {
  thread_t *td = thread_self();

  if (waitpt == NULL)
    waitpt = __caller(0);

  sleepq_chain_t *sc = sc_acquire(wchan);
  spin_lock(td->td_lock);
  td->td_state = TDS_SLEEPING;
  sq_enter(td, sc, wchan, waitpt);

  /* Panic if we were interrupted by timeout or signal. */
  assert((td->td_flags & (TDF_SLPINTR | TDF_SLPTIMED)) == 0);
}

static void sq_timeout(thread_t *td) {
  _sleepq_abort(td, ETIMEDOUT);
}

int sleepq_wait_timed(void *wchan, const void *waitpt, systime_t timeout) {
  thread_t *td = thread_self();

  if (waitpt == NULL)
    waitpt = __caller(0);

  sleepq_chain_t *sc = sc_acquire(wchan);
  spin_lock(td->td_lock);

  /* If there are pending signals, interrupt the sleep immediately. */
  if ((td->td_flags & TDF_NEEDSIGCHK) && (timeout == 0)) {
    spin_unlock(td->td_lock);
    sc_release(sc);
    return EINTR;
  }

  if (timeout > 0) {
    callout_setup(&td->td_slpcallout, (timeout_t)sq_timeout, td);
    callout_schedule(&td->td_slpcallout, timeout);
  }

  td->td_flags |= (timeout > 0) ? TDF_SLPTIMED : TDF_SLPINTR;
  sq_enter(td, sc, wchan, waitpt);

  /* After wakeup, only one of the following flags may be set:
   *  - TDF_SLPINTR if sleep was aborted,
   *  - TDF_SLPTIMED if sleep has timed out. */
  int error = 0;
  WITH_SPIN_LOCK (td->td_lock) {
    if (td->td_flags & TDF_SLPINTR) {
      error = EINTR;
    } else if (td->td_flags & TDF_SLPTIMED) {
      error = ETIMEDOUT;
    }
    td->td_flags &= ~(TDF_SLPINTR | TDF_SLPTIMED);
  }

  if (timeout > 0)
    callout_stop(&td->td_slpcallout);

  return error;
}
