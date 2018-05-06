#include <stdc.h>
#include <sched.h>
#include <mutex.h>
#include <thread.h>
#include <ktest.h>

static mtx_t counter_mtx = MTX_INITIALIZER(MTX_DEF);
static volatile int32_t counter_value;

#define COUNTER_N 1000
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

static int mtx_test_counter(void) {
  counter_value = 0;

  for (int i = 0; i < COUNTER_T; i++) {
    char name[20];
    snprintf(name, sizeof(name), "td%d", i);
    counter_td[i] = thread_create(name, counter_routine, NULL);
  }

  for (int i = 0; i < COUNTER_T; i++)
    sched_add(counter_td[i]);
  for (int i = 0; i < COUNTER_T; i++)
    thread_join(counter_td[i]);

  assert(counter_value == COUNTER_N * COUNTER_T);

  return KTEST_SUCCESS;
}

static mtx_t simple_mtx = MTX_INITIALIZER(MTX_DEF);
static thread_t *simple_td0;
/* `simple_status` equals 2 for a moment but we don't check it during that time
 * (or rather a check shouldn't happen during that time) */
static volatile int simple_status = 0;

static void simple_routine(void *arg) {
  WITH_NO_PREEMPTION {
    simple_status = 1;
    mtx_lock(&simple_mtx);
  }
  WITH_NO_PREEMPTION {
    simple_status = 2;
    mtx_unlock(&simple_mtx);
  }
  simple_status = 3;
}

static int mtx_test_simple(void) {
  simple_td0 = thread_create("td0", simple_routine, NULL);

  mtx_lock(&simple_mtx);

  sched_add(simple_td0);

  while (simple_status != 1) {
    thread_yield();
  }

  assert(simple_status == 1);
  mtx_unlock(&simple_mtx);

  thread_join(simple_td0);
  assert(simple_status == 3);
  /* NOTE This assert is implementation-specific
   * Some changes might make it invalid
   * We could assert `!mtx_owned(...)` but that would be less restrictive */
  assert(simple_mtx.m_owner == NULL);

  return KTEST_SUCCESS;
}

KTEST_ADD(mutex_counter, mtx_test_counter, 0);
KTEST_ADD(mutex_simple, mtx_test_simple, 0);
