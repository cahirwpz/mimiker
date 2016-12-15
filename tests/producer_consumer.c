#include <stdc.h>
#include <condvar.h>
#include <thread.h>
#include <mutex.h>
#include <sched.h>

#define NUM_OF_PRODUCTS 1024
#define BUFFER_LIMIT 32

static mtx_t buffer_mtx;
static unsigned value = 0;
static unsigned produced = 0;
static unsigned consumed = 0;
static condvar_t not_empty, not_full;

static void producer(void *ptr) {
  log("Producer start");
  for (int i = 0; i < NUM_OF_PRODUCTS; i++) {
    log("Producer loop %d.", i);
    mtx_lock(&buffer_mtx);
    if (value == BUFFER_LIMIT) {
      log("Buffer full");
      cv_wait(&not_full, &buffer_mtx);
    }
    produced++;
    value++;
    cv_signal(&not_empty);
    mtx_unlock(&buffer_mtx);
  }
  log("Producer ended. Produced = %d, value = %d.", produced, value);
}

static void consumer(void *ptr) {
  log("Consumer start");
  for (int i = 0; i < NUM_OF_PRODUCTS; i++) {
    log("Consumer loop %d.", i);
    mtx_lock(&buffer_mtx);
    if (value == 0) {
      log("Buffer empty");
      cv_wait(&not_empty, &buffer_mtx);
    }
    consumed++;
    value--;
    cv_signal(&not_full);
    mtx_unlock(&buffer_mtx);
  }
  log("Consumer ended. Consumed = %d, value = %d.", consumed, value);
}

int main() {
  thread_t *td1 = thread_create("Producer", producer, NULL);
  thread_t *td2 = thread_create("Consumer", consumer, NULL);

  mtx_init(&buffer_mtx);
  cv_init(&not_empty, "not_empty");
  cv_init(&not_full, "not_full");

  sched_add(td1);
  sched_add(td2);
  sched_run();
  return 0;
}
