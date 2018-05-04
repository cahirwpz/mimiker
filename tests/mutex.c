#include <stdc.h>
#include <sched.h>
#include <mutex.h>
#include <thread.h>
#include <ktest.h>

static mtx_t mtx = MTX_INITIALIZER(MTX_DEF);
static volatile int32_t value;

#define N 1000
#define T 5

static thread_t *td[T];

static void mtx_test_main(void *arg) {
  for (size_t i = 0; i < N; i++) {
    mtx_lock(&mtx);
    value++;
    mtx_unlock(&mtx);
  }
}

static int mtx_test(void) {
  value = 0;

  for (int i = 0; i < T; i++) {
    char name[20];
    snprintf(name, sizeof(name), "td%d", i);
    td[i] = thread_create(name, mtx_test_main, NULL);
  }

  for (int i = 0; i < T; i++)
    sched_add(td[i]);
  for (int i = 0; i < T; i++)
    thread_join(td[i]);

  assert(value == N * T);

  return KTEST_SUCCESS;
}

KTEST_ADD(mutex, mtx_test, 0);
