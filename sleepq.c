#include <common.h>
#include <queue.h>
#include <libkern.h>
#include <sleepq.h>
#include <thread.h>
#include <sched.h>
#include <critical_section.h>

#define SC_TABLESIZE 256 /* Must be power of 2. */
#define SC_MASK (SC_TABLESIZE - 1)
#define SC_SHIFT 8
#define SC_HASH(wc)                                                            \
  ((((uintptr_t)(wc) >> SC_SHIFT) ^ (uintptr_t)(wc)) & SC_MASK)
#define SC_LOOKUP(wc) &sleepq_chains[SC_HASH(wc)]

static sleepq_chain_t sleepq_chains[SC_TABLESIZE];

void sleepq_init() {
  memset(sleepq_chains, 0, sizeof sleepq_chains);
  for (int i = 0; i < SC_TABLESIZE; i++)
    LIST_INIT(&sleepq_chains[i].sc_queues);
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

void sleepq_add(void *wchan, const char *wmesg, thread_t *td) {
  cs_enter();
  log("Adding a thread to the sleep queue. *td: %p, *wchan: %p", td, wchan);

  assert(td->td_wchan == NULL);
  assert(td->td_wmesg == NULL);
  // assert(td->td_sleepqueue != NULL);
  // assert(TAILQ_EMPTY(&td->td_sleepqueue->sq_blocked));
  // assert(LIST_EMPTY(&td->td_sleepqueue->sq_free));
  // assert(td->td_sleepqueue->sq_nblocked == 0);

  sleepq_chain_t *sc = SC_LOOKUP(wchan);
  sleepq_t *sq = sleepq_lookup(wchan);

  /* If a sleepq already exists for wchan, we insert this thread to it.
     Otherwise use this thread's sleepq. */
  if (sq == NULL) {
    /* We need to insert a new sleepq. */
    sq = td->td_sleepqueue;
    assert(sq);
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

  TAILQ_INSERT_TAIL(&sq->sq_blocked, td, td_sleepq_entry);
  td->td_wchan = wchan;
  td->td_wmesg = wmesg;
  td->td_sleepqueue = NULL;
  sq->sq_nblocked++;
  cs_leave();
}

void sleepq_wait(void *wchan) {
  /* Here will be code that makes this thread go to sleep */
  sleepq_add(wchan, "", thread_self());
  sched_yield(false);
}

/* Remove a thread from the sleep queue and resume it. */
static void sleepq_resume_thread(sleepq_t *sq, thread_t *td) {
  cs_enter();
  log("Resuming thread from sleepq. *td: %p, *td_wchan: %p", td, td->td_wchan);

  assert(td->td_wchan != NULL);
  assert(td->td_sleepqueue == NULL);
  assert(sq->sq_nblocked >= 1);

  TAILQ_REMOVE(&sq->sq_blocked, td, td_sleepq_entry);
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

  /* Here will be code that wakes this thread. */
  sched_add(td);
  cs_leave();
}

void sleepq_signal(void *wchan) {
  cs_enter();
  sleepq_t *sq = sleepq_lookup(wchan);

  if (sq == NULL) {
    cs_leave();
    return;
  }

  thread_t *current_td;
  thread_t *best_td = NULL;

  TAILQ_FOREACH (current_td, &sq->sq_blocked, td_sleepq_entry) {
    if (best_td == NULL || current_td->td_priority > best_td->td_priority)
      best_td = current_td;
  }

  sleepq_resume_thread(sq, best_td);
  cs_leave();
}

void sleepq_broadcast(void *wchan) {
  cs_enter();
  sleepq_t *sq = sleepq_lookup(wchan);
  if (sq == NULL)
    panic("Trying to broadcast a wait channel that isn't in any sleep queue.");

  struct thread *current_td;
  TAILQ_FOREACH (current_td, &sq->sq_blocked, td_sleepq_entry) {
    sleepq_resume_thread(sq, current_td);
  }
  cs_leave();
}

void sleepq_remove(thread_t *td, void *wchan) {
  cs_enter();
  sleepq_t *sq = sleepq_lookup(wchan);

  if (td->td_wchan == NULL || td->td_wchan != wchan)
    return;

  sleepq_resume_thread(sq, td);
  cs_leave();
}
