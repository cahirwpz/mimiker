#include <pool.h>
#include <thread.h>
#include <spinlock.h>
#include <sched.h>

#define TC_TABLESIZE 256 /* Must be power of 2. */
#define TC_MASK (TC_TABLESIZE - 1)
#define TC_SHIFT 8
#define TC_HASH(wc)                                                            \
  ((((uintptr_t)(wc) >> TC_SHIFT) ^ (uintptr_t)(wc)) & TC_MASK)
#define TC_LOOKUP(wc) &turnstile_chains[TC_HASH(wc)]

typedef struct turnstile {
  spinlock_t ts_lock; /* spinlock for this turnstile */
  LIST_ENTRY(turnstile) ts_hash; /* link on turnstile chain or ts_free list */
  LIST_ENTRY(turnstile) ts_link; /* link on td_contested (turnstiles attatched
                                   * to locks that a thread owns) */
  LIST_HEAD(, turnstile) ts_free; /* free turnstiles left by threads
                                   * blocked on this turnstile */
  TAILQ_HEAD(, thread) ts_blocked[2]; /* blocked threads */
  TAILQ_HEAD(, thread) ts_pending; /* threads awakened and waiting to be
                                    * put on run queue */
  void *ts_wchan; /* waiting channel */
  thread_t *ts_owner; /* who owns the lock */
} turnstile_t;

typedef struct turnstile_chain {
  spinlock_t tc_lock;
  LIST_HEAD(, turnstile) tc_turnstiles;
} turnstile_chain_t;

spinlock_t td_contested_lock;
static turnstile_chain_t turnstile_chains[TC_TABLESIZE];

static pool_t P_TURNSTILE;

static void turnstile_ctor(turnstile_t *ts) {
  ts->ts_lock = SPINLOCK_INITIALIZER();
  LIST_INIT(&ts->ts_free);
  TAILQ_INIT(&ts->ts_blocked[TS_EXCLUSIVE_QUEUE]);
  TAILQ_INIT(&ts->ts_blocked[TS_SHARED_QUEUE]);
  TAILQ_INIT(&ts->ts_pending);
  ts->ts_wchan = NULL;
  ts->ts_owner = NULL;
}

void turnstile_init(void) {
  for (int i = 0; i < TC_TABLESIZE; i++) {
    turnstile_chain_t *tc = &turnstile_chains[i];
    LIST_INIT(&tc->tc_turnstiles);
    tc->tc_lock = SPINLOCK_INITIALIZER();
  }

  td_contested_lock = SPINLOCK_INITIALIZER();

  /* we FreeBSD tu jest coś takiego:
   * LIST_INIT(thread0.td_owned);
   * thread0.td_turnstile = NULL;
   * nie wiem, czy u nas też powinno
  */

  P_TURNSTILE = pool_create("turnstile", sizeof(turnstile_t),
                            (pool_ctor_t)turnstile_ctor, NULL);
}

turnstile_t *turnstile_alloc(void) {
  return pool_alloc(P_TURNSTILE, 0);
}

void turnstile_destroy(turnstile_t *ts) {
  pool_free(P_TURNSTILE, ts);
}

/* adjust thread's position on a turnstile after its priority
 * has been changed */
/*static*/ int turnstile_adjust_thread(turnstile_t *ts, thread_t *td) {
  // TODO
  return 42;
}

/* Walks the chain of turnstiles and their owners to propagate the priority
 * of the thread being blocked to all the threads holding locks that have to
 * release their locks before this thread can run again.
 */
/*static*/ void propagate_priority(thread_t *td) {
  // jakaś blokada na td?
  turnstile_t *ts = td->td_blocked;
  td_prio_t prio = td->td_prio;
  spin_acquire(&ts->ts_lock);

  while (1) {
    td = ts->ts_owner;
    if (td == NULL) {
      /* read lock with no owner */
      spin_release(&ts->ts_lock);
      return;
    }

    spin_release(&ts->ts_lock);

    if (td->td_prio >= prio) {
      // thread_unlock(td); -- to robi spin_release(&sched_lock);
      // ale my nie mamy sched_lock
      return;
    }

    sched_lend_prio(td, prio);

    /* lock holder is in runq or running */
    if (td->td_state == TDS_READY || td->td_state == TDS_RUNNING) {
      assert(td->td_blocked == NULL);
      // thread_unlock(td);
      return;
    }

    ts = td->td_blocked;
    // z jakiegoś powodu zakładamy tutaj, że td jest zalokowany na locku
    // a jeżeli śpi to deadlock i chcemy się wywalić
    assert(ts != NULL);

    /* resort td on the list if needed */
    if (!turnstile_adjust_thread(ts, td)) {
      /* td is in fact not blocked on any lock */
      spin_release(&ts->ts_lock);
      return;
    }
  }
}

void turnstile_adjust(thread_t *td, td_prio_t oldprio) {
  // TODO
}

void turnstile_claim(turnstile_t *ts) {
  // TODO
}

void turnstile_wait(turnstile_t *ts, thread_t *owner, int queue) {
  // TODO
}

int turnstile_signal(turnstile_t *ts, int queue) {
  // TODO
  return 42;
}

void turnstile_broadcast(turnstile_t *ts, int queue) {
  // TODO
}

void turnstile_unpend(turnstile_t *ts, int owner_type) {
  // TODO
  // lol, owner_type jest nieużywane we FreeBSD
}

void turnstile_disown(turnstile_t *ts) {
  // TODO
}


/* assumes that we own td_contested_lock */
/*static*/ void turnstile_setowner(turnstile_t *ts, thread_t *owner) {
  assert(spin_owned(&td_contested_lock));
  assert(ts->ts_owner == NULL);

  /* a shared lock might not have an owner */
  if (owner == NULL)
    return;

  ts->ts_owner = owner;
  LIST_INSERT_HEAD(&owner->td_contested, ts, ts_link);
}

void turnstile_chain_lock(void *wchan) {
  turnstile_chain_t *tc = TC_LOOKUP(wchan);
  spin_acquire(&tc->tc_lock);
}

void turnstile_chain_unlock(void *wchan) {
  turnstile_chain_t *tc = TC_LOOKUP(wchan);
  spin_release(&tc->tc_lock);
}

/* Return a pointer to the thread waiting on this turnstile with the
 * most important priority or NULL if the turnstile has no waiters.
 */
/*static*/ thread_t *turnstile_first_waiter(turnstile_t *ts) {
  thread_t *std = TAILQ_FIRST(&ts->ts_blocked[TS_SHARED_QUEUE]);
  thread_t *xtd = TAILQ_FIRST(&ts->ts_blocked[TS_EXCLUSIVE_QUEUE]);
  if (xtd == NULL || (std != NULL && std->td_prio > xtd->td_prio))
    return std;
  return xtd;
}

/* gets turnstile associated with wchan
 * and acquires tc_lock and ts_lock */
turnstile_t *turnstile_trywait(void *wchan) {
  turnstile_chain_t *tc = TC_LOOKUP(wchan);
  spin_acquire(&tc->tc_lock);

  turnstile_t *ts;
  LIST_FOREACH(ts, &tc->tc_turnstiles, ts_hash) {
    if (ts->ts_wchan == wchan) {
      spin_acquire(&ts->ts_lock);
      return ts;
    }
  }

  ts = thread_self()->td_turnstile;
  assert(ts != NULL);
  spin_acquire(&ts->ts_lock);

  assert(ts->ts_wchan == NULL);
  ts->ts_wchan = wchan;

  return ts;
}

/* cancels turnstile_trywait and releases ts_lock and tc_lock */
void turnstile_cancel(turnstile_t *ts) {
  spin_release(&ts->ts_lock);
  void *wchan = ts->ts_wchan;
  if (ts == thread_self()->td_turnstile)
    ts->ts_wchan = NULL;

  turnstile_chain_t *tc = TC_LOOKUP(wchan);
  spin_release(&tc->tc_lock);
}

/* looks for turnstile associated with wchan in turnstile chains
 * assuming that we own tc_lock and returns NULL is no turnstile
 * is found in chain;
 * the function acquires ts_lock */
turnstile_t *turnstile_lookup(void *wchan) {
  turnstile_chain_t *tc = TC_LOOKUP(wchan);
  assert(spin_owned(&tc->tc_lock));

  turnstile_t *ts;
  LIST_FOREACH(ts, &tc->tc_turnstiles, ts_hash) {
    if (ts->ts_wchan == wchan) {
      spin_acquire(&ts->ts_lock);
      return ts;
    }
  }
  return NULL;
}