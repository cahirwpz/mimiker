#include <pool.h>
#include <spinlock.h>
#include <sched.h>
#include <turnstile.h>
#include <queue.h>

#define TC_TABLESIZE 256 /* Must be power of 2. */
#define TC_MASK (TC_TABLESIZE - 1)
#define TC_SHIFT 8
#define TC_HASH(wc)                                                            \
  ((((uintptr_t)(wc) >> TC_SHIFT) ^ (uintptr_t)(wc)) & TC_MASK)
#define TC_LOOKUP(wc) &turnstile_chains[TC_HASH(wc)]

typedef TAILQ_HEAD(threadqueue, thread) threadqueue_t;
typedef LIST_HEAD(turnstilelist, turnstile) turnstilelist_t;

typedef struct turnstile {
  LIST_ENTRY(turnstile) ts_hash; /* link on turnstile chain or ts_free list */
  LIST_ENTRY(turnstile) ts_link; /* link on td_contested */
  /* free turnstiles left by threads blocked on this turnstile */
  turnstilelist_t ts_free;
  /* blocked threads sorted by decreasing active priority */
  threadqueue_t ts_blocked;
  void *ts_wchan;     /* waiting channel */
  thread_t *ts_owner; /* who owns the lock */
} turnstile_t;

typedef struct turnstile_chain {
  turnstilelist_t tc_turnstiles;
} turnstile_chain_t;

static turnstile_chain_t turnstile_chains[TC_TABLESIZE];

static pool_t P_TURNSTILE;

/* a pair type for passing values around in functions
 * holds a turnstile and its associated chain
 * (at a time it might hold a turnstile that isn't yet inserted onto any chain
 *  but the held chain will be the right one) */
typedef struct ts_pair {
  turnstile_t *ts;
  turnstile_chain_t *tc;
} ts_pair_t;

/* Locks turnstile chain associated with wchan and returns pointer
 * to this chain.
 */
static turnstile_chain_t *turnstile_chain_lock(void *wchan);

static void turnstile_ctor(turnstile_t *ts) {
  LIST_INIT(&ts->ts_free);
  TAILQ_INIT(&ts->ts_blocked);
  ts->ts_wchan = NULL;
  ts->ts_owner = NULL;
}

void turnstile_init(void) {
  for (int i = 0; i < TC_TABLESIZE; i++) {
    turnstile_chain_t *tc = &turnstile_chains[i];
    LIST_INIT(&tc->tc_turnstiles);
  }

  P_TURNSTILE = pool_create("turnstile", sizeof(turnstile_t),
                            (pool_ctor_t)turnstile_ctor, NULL);
}

turnstile_t *turnstile_alloc(void) {
  return pool_alloc(P_TURNSTILE, 0);
}

void turnstile_destroy(turnstile_t *ts) {
  pool_free(P_TURNSTILE, ts);
}

static void adjust_thread_forward(turnstile_t *ts, thread_t *td) {
  thread_t *n = td;

  do {
    n = TAILQ_NEXT(n, td_lockq);
  } while (n != NULL && n->td_prio > td->td_prio);

  if (n == NULL || n != td) {
    TAILQ_REMOVE(&ts->ts_blocked, td, td_lockq);

    if (n == NULL)
      TAILQ_INSERT_TAIL(&ts->ts_blocked, td, td_lockq);
    else /* n != NULL && n != td */
      TAILQ_INSERT_BEFORE(n, td, td_lockq);
  }
}

static void adjust_thread_backward(turnstile_t *ts, thread_t *td) {
  thread_t *p = td;

  do {
    p = TAILQ_PREV(p, threadqueue, td_lockq);
  } while (p != NULL && p->td_prio < td->td_prio);

  if (p != NULL || p != td) {
    TAILQ_REMOVE(&ts->ts_blocked, td, td_lockq);

    if (p == NULL)
      TAILQ_INSERT_HEAD(&ts->ts_blocked, td, td_lockq);
    else /* p != NULL && p != td */
      TAILQ_INSERT_AFTER(&ts->ts_blocked, p, td, td_lockq);
  }
}

/* Adjusts thread's position on ts_blocked queue after its priority
 * has been changed. */
static void adjust_thread(turnstile_t *ts, thread_t *td, prio_t oldprio) {
  assert(td_is_locked(td));

  if (td->td_prio > oldprio) {
    adjust_thread_backward(ts, td);
  } else if (td->td_prio < oldprio) {
    adjust_thread_forward(ts, td);
  }
}

// !!! acquires td_spin
static thread_t *get_owner(turnstile_t *ts) {
  thread_t *td = ts->ts_owner;
  assert(td != NULL); // TODO maybe it would make sense to move this outside
  spin_acquire(td->td_spin);
  assert(!td_is_sleeping(td)); /* Deadlock. */
  return td;
}

/* Walks the chain of turnstiles and their owners to propagate the priority
 * of td to all the threads holding locks that have to be released before
 * td can run again. */
static void propagate_priority(thread_t *td) {
  turnstile_t *ts = td->td_blocked;
  prio_t prio = td->td_prio;

  td = get_owner(ts);

  // walk through blocked threads
  while (td->td_prio < prio && !td_is_ready(td) && !td_is_running(td)) {
    assert(td != thread_self()); /* Deadlock. */
    assert(td_is_locked(td));

    prio_t prev_prio = td->td_prio;
    sched_lend_prio(td, prio);

    ts = td->td_blocked;
    assert(ts != NULL);

    /* Resort td on the blocked list if needed. */
    adjust_thread(ts, td, prev_prio);
    spin_release(td->td_spin);

    td = get_owner(ts);
  }

  // possibly finish at a running/runnable thread
  if (td->td_prio < prio && (td_is_ready(td) || td_is_running(td))) {
    sched_lend_prio(td, prio);
    // TODO we could check this even if td->td_prio >= prio
    assert(td->td_blocked == NULL);
  }

  spin_release(td->td_spin);
}

static void turnstile_setowner(turnstile_t *ts, thread_t *owner) {
  assert(ts->ts_owner == NULL);

  assert(owner != NULL);

  ts->ts_owner = owner;
  LIST_INSERT_HEAD(&owner->td_contested, ts, ts_link);
}

void turnstile_adjust(thread_t *td, prio_t oldprio) {
  assert(spin_owned(td->td_spin));
  assert(td_is_locked(td));

  turnstile_t *ts = td->td_blocked;
  assert(ts != NULL);

  adjust_thread(ts, td, oldprio);

  /* If td got higher priority and it is at the head of ts_blocked,
   * propagate its priority. */
  if (td == TAILQ_FIRST(&ts->ts_blocked) && td->td_prio > oldprio)
    propagate_priority(td);
}

// case 1 of former turnstile_wait
// we use our turnstile to track `owner`
static void turnstile_provide_own(turnstile_t *ts, turnstile_chain_t *tc,
                                  thread_t *owner, const void *waitpt) {
  thread_t *td = thread_self();

  LIST_INSERT_HEAD(&tc->tc_turnstiles, ts, ts_hash);

  assert(TAILQ_EMPTY(&ts->ts_blocked));
  assert(LIST_EMPTY(&ts->ts_free));
  assert(ts->ts_wchan != NULL);

  TAILQ_INSERT_TAIL(&ts->ts_blocked, td, td_lockq);
  turnstile_setowner(ts, owner);
}

// case 2 of former turnstile_wait
// we donate our turnstile to ts_free list
static void turnstile_join_waiting(turnstile_t *ts, turnstile_chain_t *tc,
                                   thread_t *owner, const void *waitpt) {
  thread_t *td = thread_self();
  thread_t *td1;
  TAILQ_FOREACH (td1, &ts->ts_blocked, td_lockq)
    if (td1->td_prio < td->td_prio)
      break;

  if (td1 != NULL)
    TAILQ_INSERT_BEFORE(td1, td, td_lockq);
  else
    TAILQ_INSERT_TAIL(&ts->ts_blocked, td, td_lockq);
  assert(owner == ts->ts_owner);

  assert(td->td_turnstile != NULL);
  LIST_INSERT_HEAD(&ts->ts_free, td->td_turnstile, ts_hash);
}

// final (common) part of former turnstile_wait
// Call this when all turnstile stuff is ready
// This changes appropriate thread fields and switches context
static void turnstile_actually_wait(turnstile_t *ts, turnstile_chain_t *tc,
                                    thread_t *owner, const void *waitpt) {
  thread_t *td = thread_self();

  WITH_SPINLOCK(td->td_spin) {
    td->td_turnstile = NULL;
    td->td_blocked = ts;
    td->td_wchan = ts->ts_wchan;
    td->td_waitpt = waitpt;
    td->td_state = TDS_LOCKED;

    propagate_priority(td);
    sched_switch();
  }
}

/* TODO this comment
 * nie mam pomysłu na lepszą nazwę
 * for each thread td on ts_blocked gives back td its turnstile
 * from ts_free (or gives back ts if ts_free is empty) */
static void turnstile_free_return(turnstile_t *ts) {
  assert(ts != NULL);
  assert(ts->ts_owner == thread_self());

  thread_t *td;
  TAILQ_FOREACH (td, &ts->ts_blocked, td_lockq) {
    turnstile_t *ts_for_td;
    if (LIST_EMPTY(&ts->ts_free)) {
      assert(TAILQ_NEXT(td, td_lockq) == NULL);
      ts_for_td = ts;
    } else
      ts_for_td = LIST_FIRST(&ts->ts_free);
    assert(ts_for_td != NULL);
    LIST_REMOVE(ts_for_td, ts_hash);
    td->td_turnstile = ts_for_td;
  }
}

// TODO comment
static void turnstile_unlend_self(turnstile_t *ts) {
  assert(ts != NULL);

  thread_t *td = thread_self();
  assert(ts->ts_owner == td);

  prio_t prio = 0; /* lowest priority */

  WITH_SPINLOCK(td->td_spin) {
    ts->ts_owner = NULL;
    LIST_REMOVE(ts, ts_link);

    turnstile_t *ts1;
    LIST_FOREACH(ts1, &td->td_contested, ts_link) {
      assert(ts1->ts_owner == td);
      prio_t p = TAILQ_FIRST(&ts1->ts_blocked)->td_prio;
      if (p > prio)
        prio = p;
    }

    sched_unlend_prio(td, prio);
  }
}

// TODO comment
static void turnstile_wakeup_blocked(threadqueue_t *blocked_threads) {
  while (!TAILQ_EMPTY(blocked_threads)) {
    thread_t *td = TAILQ_FIRST(blocked_threads);
    TAILQ_REMOVE(blocked_threads, td, td_lockq);

    WITH_SPINLOCK(td->td_spin) {
      assert(td_is_locked(td));
      td->td_blocked = NULL;
      td->td_wchan = NULL;
      td->td_waitpt = NULL;
      sched_wakeup(td);
    }
  }
}

static void turnstile_broadcast(turnstile_t *ts, turnstile_chain_t *tc) {
  assert(ts != NULL);
  assert(ts->ts_owner == thread_self());
  assert(!TAILQ_EMPTY(&ts->ts_blocked));

  turnstile_free_return(ts);
  turnstile_unlend_self(ts);
  turnstile_wakeup_blocked(&ts->ts_blocked);

  ts->ts_wchan = NULL;
}

static turnstile_chain_t *turnstile_chain_lock(void *wchan) {
  turnstile_chain_t *tc = TC_LOOKUP(wchan);
  return tc;
}

/* Looks for turnstile associated with wchan in turnstile chains and returns the
 * chain and either the turnstile or NULL if no turnstile is found in chains.
 */
static ts_pair_t turnstile_lookup(void *wchan) {
  turnstile_chain_t *tc = turnstile_chain_lock(wchan);

  turnstile_t *ts;
  LIST_FOREACH(ts, &tc->tc_turnstiles, ts_hash) {
    if (ts->ts_wchan == wchan)
      return (ts_pair_t){.ts = ts, .tc = tc};
  }
  return (ts_pair_t){NULL, tc};
}

void turnstile_wait_wchan(void *wchan, thread_t *owner, const void *waitpt) {
  ts_pair_t tp = turnstile_lookup(wchan);
  /* In case of SMP we would have to check now whether some other
   * processor released the mutex while we were spinning for turnstile's
   * spinlock. */

  if (tp.ts != NULL) {
    turnstile_join_waiting(tp.ts, tp.tc, owner, waitpt);
  } else {
    tp.ts = thread_self()->td_turnstile;
    assert(tp.ts != NULL);

    assert(tp.ts->ts_wchan == NULL);
    tp.ts->ts_wchan = wchan;

    turnstile_provide_own(tp.ts, tp.tc, owner, waitpt);
  }
  turnstile_actually_wait(tp.ts, tp.tc, owner, waitpt);
}

void turnstile_broadcast_wchan(void *wchan) {
  ts_pair_t tp = turnstile_lookup(wchan);
  if (tp.ts != NULL)
    turnstile_broadcast(tp.ts, tp.tc);
}
