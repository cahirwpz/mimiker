#define KL_LOG KL_TEST

#include <sys/mimiker.h>
#include <sys/libkern.h>
#include <sys/ktest.h>
#define _KLOG_PRIVATE
#include <sys/klog.h>
#include <sys/thread.h>
#include <sys/sched.h>

#define MAX_THREAD_NUM 8
#define NUM_OF_LOG_PER_THREAD 200
#define NUMBER_OF_ROUNDS 25
#define MAX_SLEEP_TIME 20
thread_t *threads[MAX_THREAD_NUM];
static bintime_t start;

static uint32_t seed = 0; /* Current seed */

static int rand(void) {
  /* Just a standard LCG */
  seed = 1664525 * seed + 1013904223;
  return seed;
}

static int logging_with_custom_mask(void) {
  klog_(KL_NONE, "Testing custom mask %d", KL_NONE);
  assert(klog.first == klog.last);
  klog_(KL_TEST, "Testing custom mask %d", KL_TEST);
  assert((klog.first + 1) % KL_SIZE == klog.last);
  klog_clear();
  return 0;
}

/* Testing logging when klog don't accept logs (klog.mask = KL_NONE) */
static int logging_klog_zero_mask(void) {
  klog("Testing klog while acceptance mask is %d", klog.mask);
  assert(klog.first == klog.last);
  klog_clear();
  return 0;
}

static int test_log_messages(const int number_of_logs) {
  for (int i = 0; i < number_of_logs; ++i)
    klog("Testing logger; Message %d, of %d", i, number_of_logs);

  int saved_logs = (number_of_logs < KL_SIZE) ? number_of_logs : KL_SIZE - 1;
  assert(((klog.first + saved_logs) % KL_SIZE) == klog.last);

  klog_clear();
  return 0;
}

static int testing_different_number_of_parametars(void) {
  klog("Message with zero parameters.");
  klog("Message with zero parameters with custom mask.");
  klog("Message with %d parameter.", 1);
  klog("Message with %d, %d, %d, %d, %d parameters.", 5, 5, 5, 5, 5);
  klog("Message with %d, %d, %d, %d, %d, %d parameters.", 6, 6, 6, 6, 6, 6);
  klog("Message with %d, %d, %d, %d, %d, %d, %d parameters.", 7, 7, 7, 7, 7, 7,
       7);
  klog_dump();

  return 0;
}

static void thread_logs(void *p) {
  int thread_number = thread_self()->td_tid;
  for (int i = 0; i < NUM_OF_LOG_PER_THREAD; i++)
    klog("Message number %d of thread %d.", i, thread_number);

  thread_exit();
}

static int multithreads_test(const int number_of_threads) {
  for (int i = 0; i < number_of_threads; i++) {
    threads[i] = thread_create("test-klog", thread_logs, NULL, prio_kthread(0));
    sched_add(threads[i]);
  }
  for (int i = 0; i < number_of_threads; i++)
    thread_join(threads[i]);

  int saved_logs = (NUM_OF_LOG_PER_THREAD * number_of_threads < KL_SIZE)
                     ? NUM_OF_LOG_PER_THREAD * number_of_threads
                     : KL_SIZE - 1;
  assert(((klog.first + saved_logs) % KL_SIZE) == klog.last);

  klog_clear();

  return 0;
}

/* Function that checks if sleep time is over and assigne new sleep time. */
static int check_time(systime_t *sleep, const uint32_t freq) {
  bintime_t now = getbintime();
  bintime_sub(&now, &start);
  if (bt2st(&now) < (*sleep))
    return 1;
  (*sleep) += (rand() & freq);
  return 0;
}

static void logs(void) {
  int thread_number = thread_self()->td_tid;
  for (int i = 0; i < 20; i++)
    klog("Message number %d of thread %d.", i, thread_number);
}

/* Thread that run function given in parameter on random bases. */
static void thread_test(void *p) {
  /* Initial sleep, waiting for all threads to be created. */
  uint32_t sleep = rand() & 100;
  void (*klog_function)(void) = p;

  for (int i = 0; i < NUMBER_OF_ROUNDS; i++) {
    while (check_time(&sleep, MAX_SLEEP_TIME))
      ;
    (*klog_function)();
    klog("Function %p ended %d time.", klog_function, i);
  }
  thread_exit();
}

static int stress_test(void) {

  threads[0] = thread_create("test-klog-clear-1", thread_test, &klog_clear,
                             prio_kthread(0));
  threads[1] =
    thread_create("test-klog-add-logs-1", thread_test, &logs, prio_kthread(0));
  threads[2] =
    thread_create("test-klog-dump-1", thread_test, &klog_dump, prio_kthread(0));
  threads[3] = thread_create("test-klog-clear-2", thread_test, &klog_clear,
                             prio_kthread(0));
  threads[4] =
    thread_create("test-klog-add-logs-2", thread_test, &logs, prio_kthread(0));
  /* Note, if we use more then one dump some output could be lost and/or
   * misplaced */
  /* threads[5] = thread_create("Thread dump2", thread_test, &klog_dump); */

  int number_of_threads = 5;
  start = getbintime();
  for (int i = 0; i < number_of_threads; i++)
    sched_add(threads[i]);

  for (int i = 0; i < number_of_threads; i++)
    thread_join(threads[i]);

  klog_clear();

  return 0;
}

static int test_klog(void) {
  kprintf("Testing klog.\n");
  int mask_old = klog.mask;
  klog.mask = KL_MASK(KL_LOG);

  /* Deleting logs if there are some old left. */
  klog_clear();
  klog_dump();

  /* Testing logging with different number of messages. */
  assert(test_log_messages(1) == 0);
  assert(test_log_messages(KL_SIZE - 1) == 0);
  assert(test_log_messages(KL_SIZE) == 0);
  assert(test_log_messages(KL_SIZE + 1) == 0);
  assert(test_log_messages(3 * KL_SIZE + 1) == 0);
  /* Testing logging messages with multiple threads. */
  assert(multithreads_test(1) == 0);
  assert(multithreads_test(5) == 0);
  assert(multithreads_test(8) == 0);

  assert(logging_with_custom_mask() == 0);
  assert(testing_different_number_of_parametars() == 0);

  assert(stress_test() == 0);
  /* Testing logging while klog don't accept any logs. */
  klog.mask = 0;
  assert(logging_klog_zero_mask() == 0);

  klog.mask = mask_old;

  return KTEST_SUCCESS;
}

KTEST_ADD(klog, test_klog, KTEST_FLAG_BROKEN);
