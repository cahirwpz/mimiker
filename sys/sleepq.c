#define KL_LOG KL_SLEEPQ
#include <klog.h>
#include <common.h>
#include <queue.h>
#include <stdc.h>
#include <sleepq.h>
#include <malloc.h>
#include <sched.h>
#include <thread.h>

static MALLOC_DEFINE(M_SLEEPQ, "sleepq", 1, 2);

#define SC_TABLESIZE 256 /* Must be power of 2. */
#define SC_MASK (SC_TABLESIZE - 1)
#define SC_SHIFT 8
#define SC_HASH(wc)                                                            \
  ((((uintptr_t)(wc) >> SC_SHIFT) ^ (uintptr_t)(wc)) & SC_MASK)
#define SC_LOOKUP(wc) &sleepq_chains[SC_HASH(wc)]

typedef TAILQ_HEAD(sq_head, thread) sq_head_t;
typedef LIST_HEAD(sq_chain_head, sleepq) sq_chain_head_t;

/*! \brief stores all threads sleeping on the same resource */
typedef struct sleepq {
  LIST_ENTRY(sleepq) sq_entry; /*!< link on sleepq_chain */
  sq_chain_head_t sq_free;     /*!< ??? */
  sq_head_t sq_blocked;        /*!< blocked threads */
  unsigned sq_nblocked;        /*!< number of blocked threads */
  void *sq_wchan;              /*!< associated waiting channel */
} sleepq_t;

/*! \brief bucket of sleep queues */
typedef struct sleepq_chain {
  sq_chain_head_t sc_queues; /*!< list of sleep queues */
} sleepq_chain_t;

static sleepq_chain_t sleepq_chains[SC_TABLESIZE];

void sleepq_init(void) {
  memset(sleepq_chains, 0, sizeof(sleepq_chains));
  for (int i = 0; i < SC_TABLESIZE; i++) {
    sleepq_chain_t *sc = &sleepq_chains[i];
    LIST_INIT(&sc->sc_queues);
  }
}

sleepq_t *sleepq_alloc(void) {
  sleepq_t *sq = kmalloc(M_SLEEPQ, sizeof(sleepq_t), M_ZERO);
  TAILQ_INIT(&sq->sq_blocked);
  LIST_INIT(&sq->sq_free);
  return sq;
}

void sleepq_destroy(sleepq_t *sq) {
  kfree(M_SLEEPQ, sq);
}

/*! \brief Lookup the sleep queue associated with \a wchan.
 *
 * \return NULL if no queue is found
 *
 * \warning returned sleep queue is locked!
 */
static sleepq_t *sleepq_lookup(void *wchan) {
  sleepq_chain_t *sc = SC_LOOKUP(wchan);

  sleepq_t *sq;
  LIST_FOREACH(sq, &sc->sc_queues, sq_entry) {
    if (sq->sq_wchan == wchan)
      return sq;
  }

  return NULL;
}

void sleepq_wait(void *wchan, const void *waitpt) {
  thread_t *td = thread_self();

  if (waitpt == NULL)
    waitpt = __caller(0);

  klog("Thread %ld goes to sleep on %p at pc=%p", td->td_tid, wchan, waitpt);

  SCOPED_NO_PREEMPTION();

  assert(td->td_wchan == NULL);
  assert(td->td_waitpt == NULL);
  assert(td->td_sleepqueue != NULL);
  assert(TAILQ_EMPTY(&td->td_sleepqueue->sq_blocked));
  assert(LIST_EMPTY(&td->td_sleepqueue->sq_free));
  assert(td->td_sleepqueue->sq_nblocked == 0);

  sleepq_chain_t *sc = SC_LOOKUP(wchan);
  sleepq_t *sq = sleepq_lookup(wchan);

  if (sq == NULL) {
    /* No sleep queue for this waiting channel.
     * We take current thread's sleep queue and use it for that purpose. */
    sq = td->td_sleepqueue;
    sq->sq_wchan = wchan;
    LIST_INSERT_HEAD(&sc->sc_queues, sq, sq_entry);
  } else {
    /* A sleep queue for the waiting channel already exists!
     * We add this thread's sleepqueue to the free list. */
    LIST_INSERT_HEAD(&sq->sq_free, td->td_sleepqueue, sq_entry);
  }

  TAILQ_INSERT_TAIL(&sq->sq_blocked, td, td_sleepq);
  sq->sq_nblocked++;

  td->td_wchan = wchan;
  td->td_waitpt = waitpt;
  td->td_sleepqueue = NULL;

  td->td_state = TDS_SLEEPING;
  sched_switch();
}

/* Remove a thread from the sleep queue and resume it. */
static void sleepq_wakeup(sleepq_t *sq, thread_t *td) {
  klog("Wakeup %ld thread from %p at pc=%p", td->td_tid, td->td_wchan,
       td->td_waitpt);

  assert(td->td_wchan != NULL);
  assert(td->td_sleepqueue == NULL);
  assert(sq->sq_nblocked >= 1);

  TAILQ_REMOVE(&sq->sq_blocked, td, td_sleepq);
  sq->sq_nblocked--;

  if (TAILQ_EMPTY(&sq->sq_blocked)) {
    /* If it was the last thread on this sleep queue, \a td gets it. */
    assert(sq->sq_nblocked == 0);
    sq->sq_wchan = NULL;
  } else {
    /* Otherwise \a td gets a sleep queue from the free list. */
    assert(sq->sq_nblocked > 0);
    assert(!LIST_EMPTY(&sq->sq_free));
    sq = LIST_FIRST(&sq->sq_free);
  }

  /* Remove the sleep queue from the chain or the free list. */
  LIST_REMOVE(sq, sq_entry);

  td->td_wchan = NULL;
  td->td_waitpt = NULL;
  td->td_sleepqueue = sq;

  sched_wakeup(td);
}

bool sleepq_signal(void *wchan) {
  SCOPED_NO_PREEMPTION();

  sleepq_t *sq = sleepq_lookup(wchan);
  if (sq == NULL)
    return false;

  thread_t *td, *best_td = NULL;
  TAILQ_FOREACH (td, &sq->sq_blocked, td_sleepq) {
    /* Search for thread with highest priority */
    if (best_td == NULL || td->td_prio > best_td->td_prio)
      best_td = td;
  }
  sleepq_wakeup(sq, best_td);
  return true;
}

bool sleepq_broadcast(void *wchan) {
  SCOPED_NO_PREEMPTION();

  sleepq_t *sq = sleepq_lookup(wchan);
  if (sq == NULL)
    return false;

  thread_t *td;
  TAILQ_FOREACH (td, &sq->sq_blocked, td_sleepq)
    sleepq_wakeup(sq, td);
  return true;
}
