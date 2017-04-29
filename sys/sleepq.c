#include <common.h>
#include <queue.h>
#include <stdc.h>
#include <sleepq.h>
#include <malloc.h>
#include <sync.h>
#include <sched.h>
#include <thread.h>

#define SC_TABLESIZE 256 /* Must be power of 2. */
#define SC_MASK (SC_TABLESIZE - 1)
#define SC_SHIFT 8
#define SC_HASH(wc)                                                            \
  ((((uintptr_t)(wc) >> SC_SHIFT) ^ (uintptr_t)(wc)) & SC_MASK)
#define SC_LOOKUP(wc) &sleepq_chains[SC_HASH(wc)]

static MALLOC_DEFINE(mp, "sleepqueue pool");

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

void sleepq_init() {
  kmalloc_init(mp);
  kmalloc_add_pages(mp, 1);

  memset(sleepq_chains, 0, sizeof(sleepq_chains));
  for (int i = 0; i < SC_TABLESIZE; i++)
    LIST_INIT(&sleepq_chains[i].sc_queues);
}

sleepq_t *sleepq_alloc() {
  return kmalloc(mp, sizeof(sleepq_t), M_ZERO);
}

void sleepq_destroy(sleepq_t *sq) {
  kfree(mp, sq);
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

void sleepq_wait(void *wchan, const char *wmesg) {
  thread_t *td = thread_self();
  log("Sleep '%s' thread on '%s' (%p)", td->td_name, wmesg, wchan);

  assert(td->td_wchan == NULL);
  assert(td->td_wmesg == NULL);
  assert(td->td_sleepqueue != NULL);
  assert(TAILQ_EMPTY(&td->td_sleepqueue->sq_blocked));
  assert(LIST_EMPTY(&td->td_sleepqueue->sq_free));
  assert(td->td_sleepqueue->sq_nblocked == 0);

  critical_enter();

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
  td->td_wmesg = wmesg;
  td->td_sleepqueue = NULL;
  td->td_state = TDS_WAITING;
  sq->sq_nblocked++;
  realtime_t now = clock_get();
  td->td_slptime += now - td->td_last_slptime;
  td->td_last_slptime = now;

  sched_yield();
  critical_leave();
}

/* Remove a thread from the sleep queue and resume it. */
static void sleepq_resume_thread(sleepq_t *sq, thread_t *td) {
  log("Wakeup '%s' thread from '%s' (%p)", td->td_name, td->td_wmesg,
      td->td_wchan);

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
  td->td_wmesg = NULL;

  sched_add(td);
}

bool sleepq_signal(void *wchan) {
  critical_enter();

  sleepq_t *sq = sleepq_lookup(wchan);
  if (sq == NULL) {
    critical_leave();
    return false;
  }

  thread_t *td, *best_td = NULL;
  TAILQ_FOREACH (td, &sq->sq_blocked, td_sleepq) {
    /* Search for thread with highest priority */
    if (best_td == NULL || td->td_prio > best_td->td_prio)
      best_td = td;
  }
  sleepq_resume_thread(sq, best_td);

  critical_leave();
  return true;
}

bool sleepq_broadcast(void *wchan) {
  critical_enter();

  sleepq_t *sq = sleepq_lookup(wchan);
  if (sq == NULL) {
    critical_leave();
    return false;
  }

  thread_t *td;
  TAILQ_FOREACH (td, &sq->sq_blocked, td_sleepq) {
    sleepq_resume_thread(sq, td);
  }

  critical_leave();
  return true;
}

void sleepq_remove(thread_t *td, void *wchan) {
  critical_enter();
  if (td->td_wchan && td->td_wchan == wchan) {
    sleepq_t *sq = sleepq_lookup(wchan);
    sleepq_resume_thread(sq, td);
  }
  critical_leave();
}
