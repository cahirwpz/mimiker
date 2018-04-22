#include <stdc.h>
#include <sched.h>
#include <mutex.h>
#include <thread.h>
#include <ktest.h>

static mtx_t mtx_counter = MTX_INITIALIZER(MTX_DEF);
static volatile int32_t value;

/* For T=10 N=13287 : randomly passes or fails mutex; fails turnstile
 * For lower N there is higher chance of passing mutex
 */
#define N 20000
#define T 10

static thread_t *td[T];

static void mtx_test_counter_routine(void *arg) {
  for (size_t i = 0; i < N; i++) {
    mtx_lock(&mtx_counter);
    value++;
    mtx_unlock(&mtx_counter);
  }
}

static int mtx_test_counter(void) {
  value = 0;

  for (int i = 0; i < T; i++) {
    char name[20];
    snprintf(name, sizeof(name), "td%d", i);
    td[i] = thread_create(name, mtx_test_counter_routine, NULL);
  }

  for (int i = 0; i < T; i++)
    sched_add(td[i]);
  for (int i = 0; i < T; i++)
    thread_join(td[i]);

  assert(value == N * T);

  return KTEST_SUCCESS;
}

static mtx_t mtx1 = MTX_INITIALIZER(MTX_DEF);
static thread_t *t1;
static int s1 = 0;

static void mtx_test_simple_routine1(void *arg) {
  WITH_NO_PREEMPTION {
    s1 = 1;
    mtx_lock(&mtx1);
  }
  WITH_NO_PREEMPTION {
    s1 = 2;
    mtx_unlock(&mtx1);
  }
  s1 = 3;
}

static int mtx_test_simple(void) {
  t1 = thread_create("td1", mtx_test_simple_routine1, NULL);

  mtx_lock(&mtx1);

  sched_add(t1);
  while (s1 != 1) {
    thread_yield();
  }

  assert(s1 == 1);
  mtx_unlock(&mtx1);

  thread_join(t1);
  assert(s1 == 3);
  // NOTE this assert is implementation-specific
  // some changes might make it invalid
  assert(mtx1.m_owner == NULL);

  return KTEST_SUCCESS;
}

KTEST_ADD(mutex_counter, mtx_test_counter, 0);
KTEST_ADD(mutex_simple, mtx_test_simple, 0);
