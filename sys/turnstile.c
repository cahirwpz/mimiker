#include <pool.h>

typedef struct turnstile {

} turnstile_t;

typedef struct {

} turnstile_chain_t;

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