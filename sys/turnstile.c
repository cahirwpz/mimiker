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
  spinlock_t ts_lock;            /* spinlock for this turnstile */
  LIST_ENTRY(turnstile) ts_hash; /* link on turnstile chain or ts_free list */
  LIST_ENTRY(turnstile)
  ts_link;                  /* link on td_contested (turnstiles attached
                             * to locks that a thread owns) */
  turnstilelist_t ts_free;  /* free turnstiles left by threads
                                    * blocked on this turnstile */
  threadqueue_t ts_blocked; /* blocked threads sorted by
                                   * decreasing active priority */
  threadqueue_t ts_pending; /* threads awakened and waiting to be
                                   * put on run queue */
  void *ts_wchan;           /* waiting channel */
  thread_t *ts_owner;       /* who owns the lock */
} turnstile_t;

typedef struct turnstile_chain {
  spinlock_t tc_lock;
  turnstilelist_t tc_turnstiles;
} turnstile_chain_t;

spinlock_t td_contested_lock;
static turnstile_chain_t turnstile_chains[TC_TABLESIZE];

static pool_t P_TURNSTILE;

static void turnstile_ctor(turnstile_t *ts) {
  ts->ts_lock = SPINLOCK_INITIALIZER();
  LIST_INIT(&ts->ts_free);
  TAILQ_INIT(&ts->ts_blocked);
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

/* Adjusts thread's position on ts_blocked queue after its priority
 * has been changed.
 * Return values:
 * - 1  -- moved towards the end of the list
 * - 0  -- didn't move
 * - -1 -- moved towards the head of the list
 */

// TODO consider locks
static int turnstile_adjust_thread(turnstile_t *ts, thread_t *td) {
  thread_t *n = TAILQ_NEXT(td, td_turnstileq);
  thread_t *p = TAILQ_PREV(td, threadqueue, td_turnstileq);

  int moved = 0; // 1 - forward, 0 - not moved, -1 - backward

  if (n != NULL && n->td_prio > td->td_prio) {
    moved = 1;
    TAILQ_REMOVE(&ts->ts_blocked, td, td_turnstileq);

    while (n != NULL && n->td_prio > td->td_prio) {
      n = TAILQ_NEXT(n, td_turnstileq);
    }

    if (n != NULL)
      TAILQ_INSERT_BEFORE(n, td, td_turnstileq);
    else
      TAILQ_INSERT_TAIL(&ts->ts_blocked, td, td_turnstileq);
  }

  if (p != NULL && p->td_prio < td->td_prio) {
    assert(moved == 0);
    moved = -1;
    TAILQ_REMOVE(&ts->ts_blocked, td, td_turnstileq);

    while (p != NULL && p->td_prio < td->td_prio) {
      p = TAILQ_PREV(p, threadqueue, td_turnstileq);
    }

    if (p != NULL)
      TAILQ_INSERT_AFTER(&ts->ts_blocked, p, td, td_turnstileq);
    else
      TAILQ_INSERT_HEAD(&ts->ts_blocked, td, td_turnstileq);
  }

  return moved;
}

/* Walks the chain of turnstiles and their owners to propagate the priority
 * of the thread being blocked to all the threads holding locks that have to
 * release their locks before this thread can run again.
 */
void propagate_priority(thread_t *td) {
  // TODO jakaś blokada na td?
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

/* Return a pointer to the thread waiting on this turnstile with the
 * most important priority or NULL if the turnstile has no waiters.
 */
static thread_t *turnstile_first_waiter(turnstile_t *ts) {
  return TAILQ_FIRST(&ts->ts_blocked);
}

static void turnstile_setowner(turnstile_t *ts, thread_t *owner) {
  assert(spin_owned(&td_contested_lock));
  assert(ts->ts_owner == NULL);

  /* a shared lock might not have an owner */
  if (owner == NULL)
    return;

  ts->ts_owner = owner;
  LIST_INSERT_HEAD(&owner->td_contested, ts, ts_link);
}

/* td - blocked thread (on some turnstile)
 * Gotta:
 * - sort the list on which =td= is (=ts->ts_blocked=)
 * - check the priority of the thread owning the lock
 *   - we have to change it only if =td= was the first waiter and
 *     isn't anymore or wasn't then but now is
 */
// TODO consider locks
void turnstile_adjust(thread_t *td, td_prio_t oldprio) {
  turnstile_t *ts = td->td_blocked;

  // we FreeBSD jest jakieś zamieszanie odnośnie td->td_turnstile != NULL,
  // bo cośtam cośtam SMP. Chyba się nie przejmujemy.

  thread_t *first = turnstile_first_waiter(ts);
  turnstile_adjust_thread(ts, td);
  thread_t *new_first = turnstile_first_waiter(ts);

  if (first == new_first)
    return;
  else {
    // we FreeBSD poprawiają priorytet jedynie, jeśli nowy jest lepszy
    // (nie obniżają już pożyczonych priorytetów)

    // If =new_first= isn't =first= then =td= is either of them.
    // If =td='s priority was increased then it's =new_first= and
    // we only have to propagate the new (higher) priority
    if (td->td_prio > oldprio) {
      propagate_priority(new_first);
    } else {
      // patrz wyżej
      // TODO? remove the old lent priority and propagate the new one
    }
  }
}

/*
 * Block the current thread on the turnstile assicated with 'lock'.  This
 * function will context switch and not return until this thread has been
 * woken back up.  This function must be called with the appropriate
 * turnstile chain locked and will return with it unlocked.
 */
void turnstile_wait(turnstile_t *ts, thread_t *owner) {
  assert(spin_owned(&ts->ts_lock));

  turnstile_chain_t *tc = TC_LOOKUP(ts->ts_wchan);
  assert(spin_owned(&tc->tc_lock));

  thread_t *td = thread_self();
  if (ts == td->td_turnstile) {
    LIST_INSERT_HEAD(&tc->tc_turnstiles, ts, ts_hash);

    assert(TAILQ_EMPTY(&ts->ts_pending));
    assert(TAILQ_EMPTY(&ts->ts_blocked));
    assert(LIST_EMPTY(&ts->ts_free));
    assert(ts->ts_wchan != NULL);

    WITH_SPINLOCK(&td_contested_lock) {
      TAILQ_INSERT_TAIL(&ts->ts_blocked, td, td_turnstileq);
      turnstile_setowner(ts, owner);
    }
  } else {
    thread_t *td1;
    TAILQ_FOREACH (td1, &ts->ts_blocked, td_turnstileq)
      if (td1->td_prio < td->td_prio)
        break;
    WITH_SPINLOCK(&td_contested_lock) {
      if (td1 != NULL)
        TAILQ_INSERT_BEFORE(td1, td, td_turnstileq);
      else
        TAILQ_INSERT_TAIL(&ts->ts_blocked, td, td_turnstileq);
      assert(owner == ts->ts_owner);
    }
    assert(td->td_turnstile != NULL);
    LIST_INSERT_HEAD(&ts->ts_free, td->td_turnstile, ts_hash);
  }

  WITH_SPINLOCK(td->td_spin) {
    td->td_turnstile = NULL;
    td->td_blocked = ts;
    td->td_wchan = ts->ts_wchan;
    td->td_waitpt = NULL; // TODO
    td->td_state = TDS_LOCKED;

    spin_release(&tc->tc_lock);
    propagate_priority(td);
    spin_release(&ts->ts_lock);
    sched_switch();
  }
}

void turnstile_broadcast(turnstile_t *ts) {
  assert(ts != NULL);
  assert(spin_owned(&ts->ts_lock));
  assert(ts->ts_owner == thread_self());

  turnstile_chain_t *tc = TC_LOOKUP(ts->ts_wchan);
  assert(spin_owned(&tc->tc_lock));

  spin_acquire(&td_contested_lock);
  TAILQ_CONCAT(&ts->ts_pending, &ts->ts_blocked, td_turnstileq);
  spin_release(&td_contested_lock);

  thread_t *td;
  turnstile_t *ts1;
  TAILQ_FOREACH (td, &ts->ts_pending, td_turnstileq) {
    if (LIST_EMPTY(&ts->ts_free)) {
      assert(TAILQ_NEXT(td, td_turnstileq) == NULL);
      ts1 = ts;
    } else
      ts1 = LIST_FIRST(&ts->ts_free);
    assert(ts1 != NULL);
    LIST_REMOVE(ts1, ts_hash);
    td->td_turnstile = ts1;
  }
}

/*
 * Wakeup all threads on the pending list and adjust the priority of the
 * current thread appropriately.  This must be called with the turnstile
 * chain locked.
 */
void turnstile_unpend(turnstile_t *ts) {
  threadqueue_t pending_threads;
  assert(ts != NULL);
  assert(spin_owned(&ts->ts_lock));
  assert(ts->ts_owner == thread_self());
  assert(!TAILQ_EMPTY(&ts->ts_pending));

  TAILQ_INIT(&pending_threads);
  TAILQ_CONCAT(&pending_threads, &ts->ts_pending, td_turnstileq);

  if (TAILQ_EMPTY(&ts->ts_blocked))
    ts->ts_wchan = NULL;

  thread_t *td = thread_self();
  td_prio_t prio = 0; /* lowest priority */

  WITH_SPINLOCK(td->td_spin) {
    WITH_SPINLOCK(&td_contested_lock) {
      if (ts->ts_owner != NULL) {
        ts->ts_owner = NULL;
        LIST_REMOVE(ts, ts_link);
      }

      turnstile_t *ts1;
      LIST_FOREACH(ts1, &td->td_contested, ts_link) {
        td_prio_t p = turnstile_first_waiter(ts1)->td_prio;
        if (p > prio)
          prio = p;
      }
    }
    sched_unlend_prio(td, prio);
  }

  while (!TAILQ_EMPTY(&pending_threads)) {
    td = TAILQ_FIRST(&pending_threads);
    TAILQ_REMOVE(&pending_threads, td, td_turnstileq);

    WITH_SPINLOCK(td->td_spin) {
      td->td_blocked = NULL;
      td->td_wchan = NULL;
      td->td_waitpt = NULL;
      sched_wakeup(td);
    }
  }

  spin_release(&ts->ts_lock);
}

// TODO consider locks
// void turnstile_disown(turnstile_t *ts) {
//   thread_t *td = ts->ts_owner;

//   // TODO who should be the new owner of the turnstile?
//   // Do we just leave it NULL so that calling procedure will handle it?
//   // Or should the next thread acquiring the lock do something?
//   // NOTE (probably the last one?)
//   ts->ts_owner = NULL;

//   LIST_REMOVE(ts, ts_link);

//   td_prio_t new_prio = td->td_base_prio;

//   turnstile_t *owned;
//   LIST_FOREACH(owned, &td->td_contested, ts_link) {
//     // NOTE one waiter must exist because otherwise the turnstile wouldn't
//     // exist
//     new_prio = max(new_prio, turnstile_first_waiter(owned)->td_prio);
//   }

//   sched_unlend_prio(td, new_prio);
// }

void turnstile_chain_lock(void *wchan) {
  turnstile_chain_t *tc = TC_LOOKUP(wchan);
  spin_acquire(&tc->tc_lock);
}

void turnstile_chain_unlock(void *wchan) {
  turnstile_chain_t *tc = TC_LOOKUP(wchan);
  spin_release(&tc->tc_lock);
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
