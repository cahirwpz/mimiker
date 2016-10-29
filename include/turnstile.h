#include <queue.h>
#include <thread.h>

#define MTX_UNOWNED   0
#define MTX_CONTESTED 1

typedef struct {
  TAILQ_HEAD(,thread_t) td_queue;
} turnstile_t;

typedef struct {
    uintptr_t mtx_state;  
    turnstile_t *turnstile;
} mtx_t ;

void mtx_lock(mtx_t*);
void mtx_unlock(mtx_t*);

void turnstile_add(turnstile_t*);
void turnstile_signal(turnstile_t*);

