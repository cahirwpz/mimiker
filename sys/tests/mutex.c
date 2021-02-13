#include <sys/klog.h>
#include <sys/libkern.h>
#include <sys/sched.h>
#include <sys/mutex.h>
#include <sys/thread.h>
#include <sys/ktest.h>

static mtx_t counter_mtx = MTX_INITIALIZER(counter_mtx, 0);
static volatile int32_t counter_value;

/* Good test to measure context switch time. */
#define COUNTER_N 100
#define COUNTER_T 5

static thread_t *counter_td[COUNTER_T];

static void counter_routine(void *arg) {
  for (size_t i = 0; i < COUNTER_N; i++) {
    mtx_lock(&counter_mtx);
    int32_t v = counter_value;
    thread_yield();
    counter_value = v + 1;
    mtx_unlock(&counter_mtx);
  }
}

static int test_mutex_counter(void) {
  counter_value = 0;

  for (int i = 0; i < COUNTER_T; i++) {
    char name[20];
    snprintf(name, sizeof(name), "test-mutex-%d", i);
    counter_td[i] = thread_create(name, counter_routine, NULL, prio_kthread(0));
  }

  for (int i = 0; i < COUNTER_T; i++)
    sched_add(counter_td[i]);
  for (int i = 0; i < COUNTER_T; i++)
    thread_join(counter_td[i]);

  assert(counter_value == COUNTER_N * COUNTER_T);

  return KTEST_SUCCESS;
}

typedef enum rtn_state {
  ST_INITIAL,
  ST_LOCKING,
  ST_UNLOCKING,
  ST_DONE
} rtn_state_t;

static mtx_t simple_mtx = MTX_INITIALIZER(simple_mtx, 0);
static thread_t *simple_td0;
/* `simple_status` equals ST_UNLOCKING for a moment but we don't check
 *  it during that time (or rather a check shouldn't happen during that time) */
static volatile rtn_state_t simple_status;

static void simple_routine(void *arg) {
  WITH_NO_PREEMPTION {
    simple_status = ST_LOCKING;
    mtx_lock(&simple_mtx);
  }
  WITH_NO_PREEMPTION {
    simple_status = ST_UNLOCKING;
    mtx_unlock(&simple_mtx);
  }
  simple_status = ST_DONE;
}

static int test_mutex_simple(void) {
  simple_td0 =
    thread_create("test-mutex", simple_routine, NULL, prio_kthread(0));
  simple_status = ST_INITIAL;

  mtx_lock(&simple_mtx);

  sched_add(simple_td0);

  while (simple_status != ST_LOCKING) {
    thread_yield();
  }

  assert(simple_status == ST_LOCKING);
  mtx_unlock(&simple_mtx);

  thread_join(simple_td0);
  assert(simple_status == ST_DONE);
  assert(mtx_owner(&simple_mtx) == NULL);

  return KTEST_SUCCESS;
}

KTEST_ADD(mutex_counter, test_mutex_counter, 0);
KTEST_ADD(mutex_simple, test_mutex_simple, 0);
