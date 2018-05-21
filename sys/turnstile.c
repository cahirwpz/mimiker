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
  LIST_ENTRY(turnstile) ts_link; /* link on td_contested */
  /* free turnstiles left by threads blocked on this turnstile */
  turnstilelist_t ts_free;
  /* blocked threads sorted by decreasing active priority */
  threadqueue_t ts_blocked;
  void *ts_wchan;     /* waiting channel */
  thread_t *ts_owner; /* who owns the lock */
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
 * has been changed. */
static void turnstile_adjust_thread(turnstile_t *ts, thread_t *td) {
  assert(td->td_state == TDS_LOCKED);
  assert(spin_owned(&ts->ts_lock));

  thread_t *n = TAILQ_NEXT(td, td_lockq);
  thread_t *p = TAILQ_PREV(td, threadqueue, td_lockq);

  bool moved_forward = false;
  if (n != NULL && n->td_prio > td->td_prio) {
    moved_forward = true;
    TAILQ_REMOVE(&ts->ts_blocked, td, td_lockq);

    while (n != NULL && n->td_prio > td->td_prio) {
      n = TAILQ_NEXT(n, td_lockq);
    }

    if (n != NULL)
      TAILQ_INSERT_BEFORE(n, td, td_lockq);
    else
      TAILQ_INSERT_TAIL(&ts->ts_blocked, td, td_lockq);
  }

  if (p != NULL && p->td_prio < td->td_prio) {
    assert(moved_forward == false);
    TAILQ_REMOVE(&ts->ts_blocked, td, td_lockq);

    while (p != NULL && p->td_prio < td->td_prio) {
      p = TAILQ_PREV(p, threadqueue, td_lockq);
    }

    if (p != NULL)
      TAILQ_INSERT_AFTER(&ts->ts_blocked, p, td, td_lockq);
    else
      TAILQ_INSERT_HEAD(&ts->ts_blocked, td, td_lockq);
  }
}

/* Walks the chain of turnstiles and their owners to propagate the priority
 * of td to all the threads holding locks that have to be released before
 * td can run again. */
static void propagate_priority(thread_t *td) {
  turnstile_t *ts = td->td_blocked;
  prio_t prio = td->td_prio;
  spin_acquire(&ts->ts_lock);

  while (1) {
    td = ts->ts_owner;
    SCOPED_SPINLOCK(td->td_spin);
    assert(td != NULL);
    assert(td->td_state != TDS_SLEEPING); /* Deadlock. */

    spin_release(&ts->ts_lock);

    if (td->td_prio >= prio)
      return;

    sched_lend_prio(td, prio);

    /* Lock holder is on run queue or is currently running. */
    if (td->td_state == TDS_READY || td->td_state == TDS_RUNNING) {
      assert(td->td_blocked == NULL);
      return;
    }

    assert(td != thread_self()); /* Deadlock. */
    assert(td->td_state == TDS_LOCKED);

    ts = td->td_blocked;
    assert(ts != NULL);

    /* Resort td on the blocked list if needed. */
    spin_acquire(&ts->ts_lock);
    turnstile_adjust_thread(ts, td);
  }
}

static void turnstile_setowner(turnstile_t *ts, thread_t *owner) {
  assert(spin_owned(&td_contested_lock));
  assert(ts->ts_owner == NULL);

  assert(owner != NULL);

  ts->ts_owner = owner;
  LIST_INSERT_HEAD(&owner->td_contested, ts, ts_link);
}

void turnstile_adjust(thread_t *td, prio_t oldprio) {
  assert(spin_owned(td->td_spin));
  assert(td->td_state == TDS_LOCKED);

  turnstile_t *ts = td->td_blocked;
  assert(ts != NULL);

  WITH_SPINLOCK(&ts->ts_lock) {
    turnstile_adjust_thread(ts, td);
  }

  /* If td got higher priority and it is at the head of ts_blocked,
   * propagate its priority. */
  if (td == TAILQ_FIRST(&ts->ts_blocked) && td->td_prio > oldprio)
    propagate_priority(td);
}

void turnstile_wait(turnstile_t *ts, thread_t *owner, const void *waitpt) {
  assert(spin_owned(&ts->ts_lock));

  turnstile_chain_t *tc = TC_LOOKUP(ts->ts_wchan);
  assert(spin_owned(&tc->tc_lock));

  thread_t *td = thread_self();
  if (ts == td->td_turnstile) {
    LIST_INSERT_HEAD(&tc->tc_turnstiles, ts, ts_hash);

    assert(TAILQ_EMPTY(&ts->ts_blocked));
    assert(LIST_EMPTY(&ts->ts_free));
    assert(ts->ts_wchan != NULL);

    WITH_SPINLOCK(&td_contested_lock) {
      TAILQ_INSERT_TAIL(&ts->ts_blocked, td, td_lockq);
      turnstile_setowner(ts, owner);
    }
  } else {
    thread_t *td1;
    TAILQ_FOREACH (td1, &ts->ts_blocked, td_lockq)
      if (td1->td_prio < td->td_prio)
        break;
    WITH_SPINLOCK(&td_contested_lock) {
      if (td1 != NULL)
        TAILQ_INSERT_BEFORE(td1, td, td_lockq);
      else
        TAILQ_INSERT_TAIL(&ts->ts_blocked, td, td_lockq);
      assert(owner == ts->ts_owner);
    }
    assert(td->td_turnstile != NULL);
    LIST_INSERT_HEAD(&ts->ts_free, td->td_turnstile, ts_hash);
  }

  WITH_SPINLOCK(td->td_spin) {
    td->td_turnstile = NULL;
    td->td_blocked = ts;
    td->td_wchan = ts->ts_wchan;
    td->td_waitpt = waitpt;
    td->td_state = TDS_LOCKED;

    spin_release(&tc->tc_lock);
    propagate_priority(td);
    spin_release(&ts->ts_lock); // TODO maybe it should be before propagate_p..
    sched_switch();
  }
}

/* TODO this comment
 * nie mam pomysłu na lepszą nazwę
 * for each thread td on ts_blocked gives back td its turnstile
 * from ts_free (or gives back ts if ts_free is empty) */
static void turnstile_free_return(turnstile_t *ts) {
  assert(ts != NULL);
  assert(spin_owned(&ts->ts_lock));
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
  assert(spin_owned(&ts->ts_lock));

  thread_t *td = thread_self();
  assert(ts->ts_owner == td);

  prio_t prio = 0; /* lowest priority */

  WITH_SPINLOCK(td->td_spin) {
    WITH_SPINLOCK(&td_contested_lock) {
      ts->ts_owner = NULL;
      LIST_REMOVE(ts, ts_link);

      turnstile_t *ts1;
      LIST_FOREACH(ts1, &td->td_contested, ts_link) {
        assert(ts1->ts_owner == td);
        prio_t p = TAILQ_FIRST(&ts1->ts_blocked)->td_prio;
        if (p > prio)
          prio = p;
      }
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
      assert(td->td_state == TDS_LOCKED);
      td->td_blocked = NULL;
      td->td_wchan = NULL;
      td->td_waitpt = NULL;
      sched_wakeup(td);
    }
  }
}

static turnstile_chain_t *turnstile_chain_lock(void *wchan) {
  turnstile_chain_t *tc = TC_LOOKUP(wchan);
  spin_acquire(&tc->tc_lock);
  return tc;
}

static void turnstile_chain_unlock(void *wchan) {
  turnstile_chain_t *tc = TC_LOOKUP(wchan);
  spin_release(&tc->tc_lock);
}

void turnstile_broadcast(turnstile_t *ts) {
  assert(ts != NULL);
  assert(spin_owned(&ts->ts_lock));
  assert(ts->ts_owner == thread_self());
  assert(!TAILQ_EMPTY(&ts->ts_blocked));

  turnstile_chain_t *tc = TC_LOOKUP(ts->ts_wchan);
  assert(spin_owned(&tc->tc_lock));

  turnstile_free_return(ts);
  turnstile_unlend_self(ts);
  turnstile_wakeup_blocked(&ts->ts_blocked);

  void *wchan = ts->ts_wchan;
  ts->ts_wchan = NULL;

  spin_release(&ts->ts_lock);
  turnstile_chain_unlock(wchan);
}

turnstile_t *turnstile_lookup(void *wchan) {
  turnstile_chain_t *tc = turnstile_chain_lock(wchan);

  turnstile_t *ts;
  LIST_FOREACH(ts, &tc->tc_turnstiles, ts_hash) {
    if (ts->ts_wchan == wchan) {
      spin_acquire(&ts->ts_lock);
      return ts;
    }
  }

  /* The lock wasn't contested. */
  turnstile_chain_unlock(wchan);
  return NULL;
}

turnstile_t *turnstile_acquire(void *wchan) {
  turnstile_t *ts = turnstile_lookup(wchan);
  if (ts != NULL)
    return ts;

  turnstile_chain_lock(wchan);

  ts = thread_self()->td_turnstile;
  assert(ts != NULL);
  spin_acquire(&ts->ts_lock);

  assert(ts->ts_wchan == NULL);
  ts->ts_wchan = wchan;

  return ts;
}
