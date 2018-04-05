#include <pool.h>
#include <thread.h>
#include <spinlock.h>

#define TC_TABLESIZE 256 /* Must be power of 2. */
#define TC_MASK (TC_TABLESIZE - 1)
#define TC_SHIFT 8
#define TC_HASH(wc)                                                            \
  ((((uintptr_t)(wc) >> TC_SHIFT) ^ (uintptr_t)(wc)) & TC_MASK)
#define TC_LOOKUP(wc) &turnstile_chains[TC_HASH(wc)]

typedef struct turnstile {
  spinlock_t ts_lock;
  LIST_HEAD(, turnstile) ts_free;
  TAILQ_HEAD(, thread) ts_blocked[2];
} turnstile_t;

typedef struct turnstile_chain {
  spinlock_t tc_lock;
  LIST_HEAD(, turnstile) sc_queues;
} turnstile_chain_t;

static turnstile_chain_t turnstile_chains[TC_TABLESIZE];

static pool_t P_TURNSTILE;

static void turnstile_ctor(turnstile_t *ts) {
  // TODO
}

void turnstile_init(void) {
  // memset(turnstile_chains, 0, sizeof(turnstile_chains));
  // for (int i = 0; i < SC_TABLESIZE; i++) {
  //   turnstile_chain_t *tc = &turnstile_chains[i];
  //   LIST_INIT(&tc->tc_queues);
  // }

  P_TURNSTILE = pool_create("turnstile", sizeof(turnstile_t),
                            (pool_ctor_t)turnstile_ctor, NULL);
}

turnstile_t *turnstile_alloc(void) {
  return pool_alloc(P_TURNSTILE, 0);
}

void turnstile_destroy(turnstile_t *ts) {
  pool_free(P_TURNSTILE, ts);
}

turnstile_t *turnstile_trywait(void *wchan) {
  // code from http://bxr.su/FreeBSD/sys/kern/subr_turnstile.c#545

  turnstile_chain_t *tc = TC_LOOKUP(wchan);
  spin_acquire(&tc->tc_lock);

  // TODO

  turnstile_t *ts = (turnstile_t *)42; // TO BE DELETED
  return ts;
}