#define KL_LOG KL_SLEEPQ
#include <klog.h>
#include <common.h>
#include <queue.h>
#include <stdc.h>
#include <sleepq.h>
#include <malloc.h>
#include <interrupt.h>
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

typedef struct sleepq {
  LIST_ENTRY(sleepq) sq_entry;
  sq_chain_head_t sq_free;
  sq_head_t sq_blocked; /* Blocked threads. */
  unsigned sq_nblocked; /* Number of blocked threads. */
  void *sq_wchan;       /* Wait channel. */
} sleepq_t;

typedef struct sleepq_chain {
  sq_chain_head_t sc_queues; /* List of sleep queues. */
} sleepq_chain_t;

static sleepq_chain_t sleepq_chains[SC_TABLESIZE];

void sleepq_init(void) {
  memset(sleepq_chains, 0, sizeof(sleepq_chains));
  for (int i = 0; i < SC_TABLESIZE; i++)
    LIST_INIT(&sleepq_chains[i].sc_queues);
}

sleepq_t *sleepq_alloc(void) {
  return kmalloc(M_SLEEPQ, sizeof(sleepq_t), M_ZERO);
}

void sleepq_destroy(sleepq_t *sq) {
  kfree(M_SLEEPQ, sq);
}

sleepq_t *sleepq_lookup(void *wchan) {
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

  assert(td->td_wchan == NULL);
  assert(td->td_waitpt == NULL);
  assert(td->td_sleepqueue != NULL);
  assert(TAILQ_EMPTY(&td->td_sleepqueue->sq_blocked));
  assert(LIST_EMPTY(&td->td_sleepqueue->sq_free));
  assert(td->td_sleepqueue->sq_nblocked == 0);

  SCOPED_INTR_DISABLED();

  sleepq_chain_t *sc = SC_LOOKUP(wchan);
  sleepq_t *sq = sleepq_lookup(wchan);

  /* If a sleepq already exists for wchan, we insert this thread to it.
     Otherwise use this thread's sleepq. */
  if (sq == NULL) {
    /* We need to insert a new sleepq. */
    sq = td->td_sleepqueue;
    LIST_INSERT_HEAD(&sc->sc_queues, sq, sq_entry);
    sq->sq_wchan = wchan;

    /* It can also be done when allocating memory for a thread's sleepqueue. */
    TAILQ_INIT(&sq->sq_blocked);
    LIST_INIT(&sq->sq_free);
  } else {
    /* A sleepqueue already exists. We add this thread's sleepqueue to the free
     * list. */
    LIST_INSERT_HEAD(&sq->sq_free, td->td_sleepqueue, sq_entry);
  }

  TAILQ_INSERT_TAIL(&sq->sq_blocked, td, td_sleepq);
  td->td_wchan = wchan;
  td->td_waitpt = waitpt;
  td->td_sleepqueue = NULL;
  td->td_state = TDS_WAITING;
  sq->sq_nblocked++;
  td->td_last_slptime = get_uptime();

  sched_switch();
}

/* Remove a thread from the sleep queue and resume it. */
static void sleepq_resume_thread(sleepq_t *sq, thread_t *td) {
  klog("Wakeup %ld thread from %p at pc=%p", td->td_tid, td->td_wchan,
       td->td_waitpt);

  assert(td->td_wchan != NULL);
  assert(td->td_sleepqueue == NULL);
  assert(sq->sq_nblocked >= 1);

  TAILQ_REMOVE(&sq->sq_blocked, td, td_sleepq);
  sq->sq_nblocked--;

  /* If it was the last thread on this sleepq, thread td gets this sleepq,
     Otherwise thread td gets a sleepq from the free list. */
  if (TAILQ_EMPTY(&sq->sq_blocked)) {
    td->td_sleepqueue = sq;
    sq->sq_wchan = NULL;
    assert(sq->sq_nblocked == 0);
  } else {
    assert(!LIST_EMPTY(&sq->sq_free));
    td->td_sleepqueue = LIST_FIRST(&sq->sq_free);
    assert(sq->sq_nblocked > 0);
  }

  /* Either remove from the sleepqueue chain or the free list. */
  LIST_REMOVE(td->td_sleepqueue, sq_entry);

  td->td_wchan = NULL;
  td->td_waitpt = NULL;
  timeval_t now = get_uptime();
  now = timeval_sub(&now, &td->td_last_slptime);
  td->td_slptime = timeval_add(&td->td_slptime, &now);
  sched_add(td);
}

bool sleepq_signal(void *wchan) {
  SCOPED_INTR_DISABLED();

  sleepq_t *sq = sleepq_lookup(wchan);
  if (sq == NULL)
    return false;

  thread_t *td, *best_td = NULL;
  TAILQ_FOREACH (td, &sq->sq_blocked, td_sleepq) {
    /* Search for thread with highest priority */
    if (best_td == NULL || td->td_prio > best_td->td_prio)
      best_td = td;
  }
  sleepq_resume_thread(sq, best_td);
  return true;
}

bool sleepq_broadcast(void *wchan) {
  SCOPED_INTR_DISABLED();

  sleepq_t *sq = sleepq_lookup(wchan);
  if (sq == NULL)
    return false;

  thread_t *td;
  TAILQ_FOREACH (td, &sq->sq_blocked, td_sleepq)
    sleepq_resume_thread(sq, td);
  return true;
}

void sleepq_remove(thread_t *td, void *wchan) {
  SCOPED_INTR_DISABLED();

  if (td->td_wchan && td->td_wchan == wchan) {
    sleepq_t *sq = sleepq_lookup(wchan);
    sleepq_resume_thread(sq, td);
  }
}
