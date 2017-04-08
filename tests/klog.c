#define KL_LOG KL_ALL

#include <stdc.h>
#include <ktest.h>
#define _KLOG_PRIVATE
#include <klog.h>
#include <thread.h>
#include <sched.h>

static int logging_with_custom_mask() {
  kprintf("Testing klog with custom mask.\n");
  klog_mask(KL_NONE, "Testing custom mask %d", KL_NONE);
  assert(klog.first == klog.last);
  if (klog.mask) {
    klog_mask(KL_ALL, "Testing custom mask %d", KL_ALL);
    assert((klog.first + 1) % KL_SIZE == klog.last);
  }

  klog_clear();
  return 0;
}

/* Testing logging when klog don't accept logs (mask = KL-NONE) */
static int logging_klog_zero_mask() {
  kprintf("Testing klog while acceptance mask is %d.\n", klog.mask);
  klog("Testing klog while acceptance mask is %d", klog.mask);
  assert(klog.first == klog.last);
  klog_clear();

  return 0;
}

static int test_log_messages(const int number_of_logs) {
  kprintf("Testing loging with %d messages in array length %d.\n",
          number_of_logs, KL_SIZE);

  for (int i = 0; i < number_of_logs; ++i)
    klog("Testing logger; Message %d, of %d", i, number_of_logs);

  int saved_logs = (number_of_logs < KL_SIZE) ? number_of_logs : KL_SIZE - 1;
  assert(((klog.first + saved_logs) % KL_SIZE) == klog.last);

  klog_clear();
  assert(klog.first == klog.last);

  return 0;
}

static int testing_different_number_of_parametars() {
  /* Logging without parameters don't works. */
  /* klog("Message with zero parameters."); */
  klog("Message with %d parameter.", 1);
  klog("Message with %d, %d, %d, %d, %d parameters.", 5, 5, 5, 5, 5);
  klog("Message with %d, %d, %d, %d, %d, %d parameters.", 6, 6, 6, 6, 6, 6);
  klog_dump();

  return 0;
}

const int number_of_logs_per_thread = 200;

static void thread_logs(void *p) {
  int thread_number = thread_self()->td_tid;
  for (int i = 0; i < number_of_logs_per_thread; i++)
    klog("Message number %d of thread %d.", i, thread_number);

  thread_exit(0);
}

static int multithreads_test(const int number_of_threads) {
  kprintf("Testing logging with %d threads.\n", number_of_threads);
  thread_t *threads[number_of_threads];
  for (int i = 0; i < number_of_threads; i++) {
    threads[i] = thread_create("Thread", thread_logs, NULL);
    sched_add(threads[i]);
  }
  for (int i = 0; i < number_of_threads; i++)
    thread_join(threads[i]);

  int saved_logs = (number_of_logs_per_thread * number_of_threads < KL_SIZE)
                     ? number_of_logs_per_thread * number_of_threads
                     : KL_SIZE - 1;
  assert(((klog.first + saved_logs) % KL_SIZE) == klog.last);

  klog_clear();

  return 0;
}

static int test_klog() {
  kprintf("Testing klog with mask %x while acceptance mask is %x\n", KL_LOG,
          klog.mask);
  /* Deleting logs if there are some old left. */
  klog_clear();

  if (klog.mask) {
    /* Testing logging with different number of messages. */
    assert(test_log_messages(1) == 0);
    assert(test_log_messages(KL_SIZE - 1) == 0);
    assert(test_log_messages(KL_SIZE) == 0);
    assert(test_log_messages(KL_SIZE + 1) == 0);
    assert(test_log_messages(3 * KL_SIZE + 1) == 0);
    /* Testing logging messages with multiple threads. */
    assert(multithreads_test(1) == 0);
    assert(multithreads_test(5) == 0);
    assert(multithreads_test(10) == 0);
  } else {
    /* Testing logging while klog don't accept any logs. */
    assert(logging_klog_zero_mask() == 0);
  }

  assert(logging_with_custom_mask() == 0);
  assert(testing_different_number_of_parametars() == 0);

  kprintf("Testing klog. Succesful!\n");

  return KTEST_SUCCESS;
}

KTEST_ADD(klog, test_klog, 0);
