#include <libkern.h>
#include <mutex_yield.h>
#include <sched.h>
#include <critical_section.h>
#include <clock.h>

void mtx_yield_init(mtx_yield_t *mtx) {
  assert(mtx);

  *mtx = 0;
}
void mtx_yield_lock(mtx_yield_t *mtx) {
  assert(mtx);

  while (true) {
    while (*mtx == 1)
      sched_yield(true);

    cs_enter();
    if (*mtx == 0) {
      *mtx = 1;
      cs_leave();
      return;
    }
    cs_leave();
  }
}
void mtx_yield_unlock(mtx_yield_t *mtx) {
  assert(mtx);

  cs_enter();
  *mtx = 0;
  cs_leave();
}

#ifdef _KERNELSPACE

static volatile uint32_t counter = 0;
#define REPS 1000000
static mtx_yield_t m;

static void demo_thread_1() {
  for (int i = 0; i < REPS; i++) {
    mtx_yield_lock(&m);
    counter++;
    mtx_yield_unlock(&m);
  }
  log("%zu", counter);
}

static void demo_thread_2() {
  for (int i = 0; i < REPS; i++) {
    mtx_yield_lock(&m);
    counter++;
    mtx_yield_unlock(&m);
  }
  log("%zu", counter);
}

int main() {
  sched_init();
  mtx_yield_init(&m);

  thread_t *t1 = thread_create("t1", demo_thread_1);
  thread_t *t2 = thread_create("t2", demo_thread_2);

  sched_add(t1);
  sched_add(t2);

  sched_run(100);
}

#endif // _KERNELSPACE
