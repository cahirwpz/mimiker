#define KL_LOG KL_TEST
#include <sys/klog.h>
#include <sys/condvar.h>
#include <sys/mutex.h>
#include <sys/sched.h>
#include <sys/libkern.h>
#include <sys/thread.h>
#include <sys/ktest.h>

#define BUF_MAX 16384
#define THREADS 3
#define ITEMS (BUF_MAX * 64)

static struct {
  mtx_t lock;
  unsigned items;
  unsigned all_produced;
  unsigned all_consumed;
  condvar_t not_empty, not_full;
} buf;

static void producer(void *ptr) {
  bool working = true;
  unsigned produced = 0;
  klog("%s started", thread_self()->td_name);
  while (working) {
    SCOPED_MTX_LOCK(&buf.lock);
    do {
      working = (buf.all_produced < ITEMS);
      if (!working)
        break;
      if (buf.items < BUF_MAX)
        break;
      cv_wait(&buf.not_full, &buf.lock);
    } while (true);
    if (working) {
      produced++;
      buf.all_produced++;
      buf.items++;
      cv_signal(&buf.not_empty);
    }
  }
  klog("%s finished, produced %d of %d.", thread_self()->td_name, produced,
       buf.all_produced);
}

static void consumer(void *ptr) {
  bool working = true;
  unsigned consumed = 0;
  klog("%s started", thread_self()->td_name);
  while (working) {
    SCOPED_MTX_LOCK(&buf.lock);
    do {
      working = (buf.all_consumed < ITEMS);
      if (!working)
        break;
      if (buf.items > 0)
        break;
      cv_wait(&buf.not_empty, &buf.lock);
    } while (true);
    if (working) {
      consumed++;
      buf.all_consumed++;
      buf.items--;
      cv_signal(&buf.not_full);
    }
  }
  klog("%s finished, consumed %d of %d.", thread_self()->td_name, consumed,
       buf.all_consumed);
}

static int test_producer_consumer(void) {
  buf.items = 0;
  buf.all_produced = 0;
  buf.all_consumed = 0;
  mtx_init(&buf.lock, 0);
  cv_init(&buf.not_empty, "not_empty");
  cv_init(&buf.not_full, "not_full");

  for (int i = 0; i < THREADS; i++) {
    char name[20];
    snprintf(name, sizeof(name), "test-producer-%d", i);
    sched_add(
      thread_create(name, producer, (void *)(intptr_t)i, prio_kthread(0)));
    snprintf(name, sizeof(name), "test-consumer-%d", i);
    sched_add(
      thread_create(name, consumer, (void *)(intptr_t)i, prio_kthread(0)));
  }

  sched_run();
  return KTEST_FAILURE;
}

KTEST_ADD(producer_consumer, test_producer_consumer, KTEST_FLAG_BROKEN);
