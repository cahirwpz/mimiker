#include <condvar.h>
#include <mutex.h>
#include <sched.h>
#include <stdc.h>
#include <thread.h>
#include <ktest.h>

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
  log("%s started", thread_self()->td_name);
  while (working) {
    mtx_scoped_lock(&buf.lock);
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
  log("%s finished, produced %d of %d.", thread_self()->td_name, produced,
      buf.all_produced);
}

static void consumer(void *ptr) {
  bool working = true;
  unsigned consumed = 0;
  log("%s started", thread_self()->td_name);
  while (working) {
    mtx_scoped_lock(&buf.lock);
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
  log("%s finished, consumed %d of %d.", thread_self()->td_name, consumed,
      buf.all_consumed);
}

static int test_producer_consumer() {
  buf.items = 0;
  buf.all_produced = 0;
  buf.all_consumed = 0;
  mtx_init(&buf.lock, MTX_DEF);
  cv_init(&buf.not_empty, "not_empty");
  cv_init(&buf.not_full, "not_full");

  for (int i = 0; i < THREADS; i++) {
    char name[20];
    snprintf(name, sizeof(name), "producer-%d", i);
    sched_add(thread_create(name, producer, (void *)i));
    snprintf(name, sizeof(name), "consumer-%d", i);
    sched_add(thread_create(name, consumer, (void *)i));
  }

  sched_run();
  return KTEST_FAILURE;
}

KTEST_ADD(producer_consumer, test_producer_consumer, KTEST_FLAG_NORETURN);
