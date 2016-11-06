#include <queue.h>
#include <thread.h>

#define MTX_UNOWNED 0
#define MTX_CONTESTED 1

typedef struct { TAILQ_HEAD(, thread) td_queue; } turnstile_t;

typedef struct {
  volatile uint32_t mtx_state;
  turnstile_t turnstile;
} mtx_sleep_t;

void mtx_sleep_init(mtx_sleep_t *);
void mtx_sleep_lock(mtx_sleep_t *);
void mtx_sleep_unlock(mtx_sleep_t *);

void turnstile_init(turnstile_t *);
void turnstile_wait(turnstile_t *);
void turnstile_signal(turnstile_t *);
