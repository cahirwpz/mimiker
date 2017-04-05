#define SYS_MASK KL_ALL

#include <stdc.h>
#include <ktest.h>
#include <klog.h>

static int test_klog_shorter() {
  int number_of_logs = kl_size - 1;
  kprintf("Testing %d logs in array size %d\n", number_of_logs, kl_size);
  for (int i = 1; i <= number_of_logs; ++i)
    klog("Testing shorter; Message %d, of %d", i, number_of_logs);
  assert(((first_message + number_of_logs) & (kl_size - 1)) ==
         (last_message & (kl_size - 1)));
  klog_dump_all();
  assert(first_message == last_message);
  kprintf("Testing %d logs in array size %d. Succesful.\n", number_of_logs,
          kl_size);

  return 0;
}

static int test_klog_equal() {
  int number_of_logs = kl_size;
  kprintf("Testing %d logs in array size %d\n", number_of_logs, kl_size);
  for (int i = 1; i <= number_of_logs; ++i)
    klog("Testing longer; Message %d, of %d", i, number_of_logs);
  assert(((first_message + kl_size - 1) & (kl_size - 1)) ==
         (last_message & (kl_size - 1)));
  klog_dump_all();
  assert(first_message == last_message);
  kprintf("Testing %d logs in array size %d. Succesful.\n", number_of_logs,
          kl_size);

  return 0;
}

static int test_klog_longer() {
  int number_of_logs = kl_size + 1;
  kprintf("Testing %d logs in array size %d\n", number_of_logs, kl_size);
  for (int i = 1; i <= number_of_logs; ++i)
    klog("Testing longer; Message %d, of %d", i, number_of_logs);
  assert(((first_message + kl_size - 1) & (kl_size - 1)) ==
         (last_message & (kl_size - 1)));
  klog_dump_all();
  assert(first_message == last_message);
  kprintf("Testing %d logs in array size %d. Succesful.\n", number_of_logs,
          kl_size);

  return 0;
}

static int test_klog_long() {
  int number_of_logs = 55 * kl_size + 1;
  kprintf("Testing %d logs in array size %d\n", number_of_logs, kl_size);
  for (int i = 1; i <= number_of_logs; ++i)
    klog("Testing longer; Message %d, of %d", i, number_of_logs);
  assert(((first_message + kl_size - 1) & (kl_size - 1)) ==
         (last_message & (kl_size - 1)));
  klog_dump_all();
  assert(first_message == last_message);
  kprintf("Testing %d logs in array size %d. Succesful.\n", number_of_logs,
          kl_size);

  return 0;
}

static int test_klog_clear() {
  int number_of_logs = 2 * kl_size + 1;
  kprintf("Testing clear\n");
  for (int i = 1; i <= number_of_logs; ++i)
    klog("Testing clear; Message %d, of %d", i, number_of_logs);
  assert(((first_message + kl_size - 1) & (kl_size - 1)) ==
         (last_message & (kl_size - 1)));
  klog_clear();

  number_of_logs = kl_size - 1;
  for (int i = 1; i <= number_of_logs; ++i)
    klog("Testing clear; Message %d, of %d", i, number_of_logs);
  assert(((first_message + number_of_logs) & (kl_size - 1)) ==
         (last_message & (kl_size - 1)));
  assert((first_message + number_of_logs) == last_message);
  klog_dump_all();
  assert(first_message == last_message);
  kprintf("Testing clear. Succesful.\n");

  return 0;
}

static int test_klog_no_log() {
  kprintf("Testing no log\n");
  int number_of_logs = 2 * kl_size + 1;
  klog_clear();
  for (int i = 1; i <= number_of_logs; ++i)
    klog_mask(KL_NONE, "Testing no log; Message %d, of %d", i, number_of_logs);
  assert(first_message == last_message);
  klog_clear();
  kprintf("Testing no log. Succesful.\n");

  return 0;
}

static int test_klog_no_log2() {
  kprintf("Testing no log2\n");
  int number_of_logs = 2 * kl_size + 1;
  klog_clear();
  // kprintf("last_message = %d, first_message = %d\n", last_message,
  // first_message);
  for (int i = 1; i <= number_of_logs; ++i)
    klog("Testing no log2; Message %d, of %d", i, number_of_logs);
  assert(first_message == last_message);
  klog_clear();
  kprintf("Testing no log2. Succesful.\n");

  return 0;
}

static int is_mask_zero() {
  int old = last_message;
  klog_mask(KL_ALL, "Testing if we can put %d in log", 1);
  if (old == last_message)
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
  kprintf("Testing klog, %d\n", (SYS_MASK));
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
  klog_dump_all();

  kprintf("Testing klog. Succesful!\n");

  return KTEST_SUCCESS;
}

KTEST_ADD(klog, test_klog, 0);
