#include <libkern.h>
#include <mutex_sleepq.h>
#include <sched.h>
#include <critical_section.h>
#include <clock.h>
#include <sleepq.h>

void mtx_sleepq_init(mtx_sleepq_t *mtx) {
  assert(mtx);

  *mtx = 0;
}
void mtx_sleepq_lock(mtx_sleepq_t *mtx) {
  assert(mtx);

  while (true) {
    while (*mtx == 1)
      sleepq_wait(mtx);

    cs_enter();
    if (*mtx == 0) {
      *mtx = 1;
      cs_leave();
      return;
    }
    cs_leave();
  }
}
void mtx_sleepq_unlock(mtx_sleepq_t *mtx) {
  assert(mtx);

  cs_enter();
  *mtx = 0;
  sleepq_signal(mtx);
  cs_leave();
}

#ifdef _KERNELSPACE

static volatile uint32_t counter = 0;
#define REPS 2000000
static mtx_sleepq_t m;

static void demo_thread_1() {
  for (int i = 0; i < REPS; i++) {
    mtx_sleepq_lock(&m);
    counter++;
    mtx_sleepq_unlock(&m);
  }
  log("%zu", counter);
}

int main() {
  sched_init();
  mtx_sleepq_init(&m);
  sleepq_init();

  thread_t *t1 = thread_create("t1", demo_thread_1);
  thread_t *t2 = thread_create("t2", demo_thread_1);
  thread_t *t3 = thread_create("t3", demo_thread_1);

  sleepq_t sq1, sq2, sq3;
  t1->td_sleepqueue = &sq1;
  t2->td_sleepqueue = &sq2;
  t3->td_sleepqueue = &sq3;

  sched_add(t1);
  sched_add(t2);
  sched_add(t3);

  sched_run(100);
}

#endif // _KERNELSPACE