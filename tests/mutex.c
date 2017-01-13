#include <stdc.h>
#include <sched.h>
#include <mutex.h>
#include <thread.h>
#include <test.h>

mtx_t mtx;
mtx_t mtx1;
mtx_t mtx2;
volatile int32_t value;

thread_t *td1;
thread_t *td2;
thread_t *td3;
thread_t *td4;
thread_t *td5;

void mtx_test_main() {
  mtx_lock(&mtx);
  for (size_t i = 0; i < 20000; i++) {
    value++;
    kprintf("%s: %ld\n", thread_self()->td_name, (long)value);
  }
  mtx_unlock(&mtx);
  while (1)
    ;
}

void mtx_test() {
  mtx_init(&mtx);
  mtx_init(&mtx);
  td1 = thread_create("td1", mtx_test_main, NULL);
  td2 = thread_create("td2", mtx_test_main, NULL);
  td3 = thread_create("td3", mtx_test_main, NULL);
  td4 = thread_create("td4", mtx_test_main, NULL);
  td5 = thread_create("td5", mtx_test_main, NULL);
  sched_add(td1);
  sched_add(td2);
  sched_add(td3);
  sched_add(td4);
  sched_add(td5);
  sched_run();
}

void deadlock_main1() {
  mtx_lock(&mtx1);
  for (size_t i = 0; i < 400000; i++)
    ;
  mtx_lock(&mtx2);
  kprintf("no deadlock!");
  mtx_unlock(&mtx2);
  mtx_unlock(&mtx1);
}

void deadlock_main2() {
  mtx_lock(&mtx2);
  for (size_t i = 0; i < 400000; i++)
    ;
  mtx_lock(&mtx1);
  kprintf("no deadlock!");
  mtx_unlock(&mtx1);
  mtx_unlock(&mtx2);
}

void deadlock_test() {
  mtx_init(&mtx1);
  mtx_init(&mtx2);
  td1 = thread_create("td1", deadlock_main1, NULL);
  td2 = thread_create("td2", deadlock_main2, NULL);
  sched_add(td1);
  sched_add(td2);
  sched_run();
  while (1)
    kprintf("elo!\n");
}

int test_mutex() {
  mtx_test();
  return 0;
}

TEST_ADD(mutex, test_mutex);
