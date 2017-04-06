#define KL_LOG KL_ALL

#include <stdc.h>
#include <ktest.h>
#define _KLOG_PRIVATE
#include <klog.h>

static int test_klog_shorter() {
  int number_of_logs = KL_SIZE - 1;
  kprintf("Testing %d logs in array size %d\n", number_of_logs, KL_SIZE);
  for (int i = 1; i <= number_of_logs; ++i)
    klog("Testing shorter; Message %d, of %d", i, number_of_logs);
  assert(((klog.first + number_of_logs) & (KL_SIZE - 1)) ==
         (klog.last & (KL_SIZE - 1)));
  klog_dump();
  assert(klog.first == klog.last);
  kprintf("Testing %d logs in array size %d. Succesful.\n", number_of_logs,
          KL_SIZE);

  return 0;
}

static int test_klog_equal() {
  int number_of_logs = KL_SIZE;
  kprintf("Testing %d logs in array size %d\n", number_of_logs, KL_SIZE);
  for (int i = 1; i <= number_of_logs; ++i)
    klog("Testing longer; Message %d, of %d", i, number_of_logs);
  assert(((klog.first + KL_SIZE - 1) & (KL_SIZE - 1)) ==
         (klog.last & (KL_SIZE - 1)));
  klog_dump();
  assert(klog.first == klog.last);
  kprintf("Testing %d logs in array size %d. Succesful.\n", number_of_logs,
          KL_SIZE);

  return 0;
}

static int test_klog_longer() {
  int number_of_logs = KL_SIZE + 1;
  kprintf("Testing %d logs in array size %d\n", number_of_logs, KL_SIZE);
  for (int i = 1; i <= number_of_logs; ++i)
    klog("Testing longer; Message %d, of %d", i, number_of_logs);
  assert(((klog.first + KL_SIZE - 1) & (KL_SIZE - 1)) ==
         (klog.last & (KL_SIZE - 1)));
  klog_dump();
  assert(klog.first == klog.last);
  kprintf("Testing %d logs in array size %d. Succesful.\n", number_of_logs,
          KL_SIZE);

  return 0;
}

static int test_klog_long() {
  int number_of_logs = 55 * KL_SIZE + 1;
  kprintf("Testing %d logs in array size %d\n", number_of_logs, KL_SIZE);
  for (int i = 1; i <= number_of_logs; ++i)
    klog("Testing longer; Message %d, of %d", i, number_of_logs);
  assert(((klog.first + KL_SIZE - 1) & (KL_SIZE - 1)) ==
         (klog.last & (KL_SIZE - 1)));
  klog_dump();
  assert(klog.first == klog.last);
  kprintf("Testing %d logs in array size %d. Succesful.\n", number_of_logs,
          KL_SIZE);

  return 0;
}

static int test_klog_clear() {
  int number_of_logs = 2 * KL_SIZE + 1;
  kprintf("Testing clear\n");
  for (int i = 1; i <= number_of_logs; ++i)
    klog("Testing clear; Message %d, of %d", i, number_of_logs);
  assert(((klog.first + KL_SIZE - 1) & (KL_SIZE - 1)) ==
         (klog.last & (KL_SIZE - 1)));
  klog_clear();

  number_of_logs = KL_SIZE - 1;
  for (int i = 1; i <= number_of_logs; ++i)
    klog("Testing clear; Message %d, of %d", i, number_of_logs);
  assert(((klog.first + number_of_logs) & (KL_SIZE - 1)) ==
         (klog.last & (KL_SIZE - 1)));
  assert((klog.first + number_of_logs) == klog.last);
  klog_dump();
  assert(klog.first == klog.last);
  kprintf("Testing clear. Succesful.\n");

  return 0;
}

static int test_klog_no_log() {
  kprintf("Testing no log\n");
  int number_of_logs = 2 * KL_SIZE + 1;
  klog_clear();
  for (int i = 1; i <= number_of_logs; ++i)
    klog_mask(KL_NONE, "Testing no log; Message %d, of %d", i, number_of_logs);
  assert(klog.first == klog.last);
  klog_clear();
  kprintf("Testing no log. Succesful.\n");

  return 0;
}

static int test_klog_no_log2() {
  kprintf("Testing no log2\n");
  int number_of_logs = 2 * KL_SIZE + 1;
  klog_clear();
  // kprintf("klog.last = %d, klog.first = %d\n", klog.last,
  // klog.first);
  for (int i = 1; i <= number_of_logs; ++i)
    klog("Testing no log2; Message %d, of %d", i, number_of_logs);
  assert(klog.first == klog.last);
  klog_clear();
  kprintf("Testing no log2. Succesful.\n");

  return 0;
}

static int is_mask_zero() {
  int old = klog.last;
  klog_mask(KL_ALL, "Testing if we can put %d in log", 1);
  if (old == klog.last)
    return 1;
  klog_clear();
  return 0;
}

// static int test_zero_params() {
//     klog("Message with zero parameters.");
//     return 0;
// }

static int test_six_params() {
  klog("Message with %d, %d, %d, %d, %d, %d parameters.", 6, 6, 6, 6, 6, 6);
  return 0;
}

// static int test_seven_params() {
//     klog("Message with %d, %d, %d, %d, %d, %d, %d parameters.", 7, 7, 7, 7,
//     7, 7, 7, 7);
//     return 0;
// }

static int test_klog() {
  kprintf("Testing klog, %x\n", KL_LOG);
  assert(test_klog_no_log() == 0);

  if (is_mask_zero())
    assert(test_klog_no_log2() == 0);
  else {
    assert(test_klog_shorter() == 0);
    assert(test_klog_equal() == 0);
    assert(test_klog_longer() == 0);
    assert(test_klog_long() == 0);
    assert(test_klog_clear() == 0);
  }

  klog_clear();
  // test_zero_params();
  test_six_params();
  // test_seven_params();
  klog_dump();

  kprintf("Testing klog. Succesful!\n");

  return KTEST_SUCCESS;
}

KTEST_ADD(klog, test_klog, 0);
