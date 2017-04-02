#include <stdc.h>
#include <ktest.h>
#include <klog.h>


static int test_klog_shorter() {
    int number_of_logs = KL_SIZE - 1;
    kprintf("Testing %d logs in array size %d\n", number_of_logs, KL_SIZE);
    for (int i = 1; i <= number_of_logs; ++i)
        klog(KL_ALL, "Testing shorter; Message %d, of %d", i, number_of_logs);
    assert(((first_message + number_of_logs) & (KL_SIZE - 1)) == (last_message & (KL_SIZE - 1)));
    klog_dump_all();
    assert(first_message == last_message);
    kprintf("Testing %d logs in array size %d. Succesful.\n", number_of_logs, KL_SIZE);

    return 0;
}

static int test_klog_equal() {
    int number_of_logs = KL_SIZE;
    kprintf("Testing %d logs in array size %d\n", number_of_logs, KL_SIZE);
    for (int i = 1; i <= number_of_logs; ++i)
        klog(KL_ALL, "Testing longer; Message %d, of %d", i, number_of_logs);
    assert(((first_message + KL_SIZE - 1) & (KL_SIZE - 1)) == (last_message & (KL_SIZE - 1)));
    klog_dump_all();
    assert(first_message == last_message);
    kprintf("Testing %d logs in array size %d. Succesful.\n", number_of_logs, KL_SIZE);

    return 0;
}

static int test_klog_longer() {
    int number_of_logs = KL_SIZE + 1;
    kprintf("Testing %d logs in array size %d\n", number_of_logs, KL_SIZE);
    for (int i = 1; i <= number_of_logs; ++i)
        klog(KL_ALL, "Testing longer; Message %d, of %d", i, number_of_logs);
    assert(((first_message + KL_SIZE - 1) & (KL_SIZE - 1)) == (last_message & (KL_SIZE - 1)));
    klog_dump_all();
    assert(first_message == last_message);
    kprintf("Testing %d logs in array size %d. Succesful.\n", number_of_logs, KL_SIZE);

    return 0;
}

static int test_klog_long() {
    int number_of_logs = 55 * KL_SIZE + 1;
    kprintf("Testing %d logs in array size %d\n", number_of_logs, KL_SIZE);
    for (int i = 1; i <= number_of_logs; ++i)
        klog(KL_ALL, "Testing longer; Message %d, of %d", i, number_of_logs);
    assert(((first_message + KL_SIZE - 1) & (KL_SIZE - 1)) == (last_message & (KL_SIZE - 1)));
    klog_dump_all();
    assert(first_message == last_message);
    kprintf("Testing %d logs in array size %d. Succesful.\n", number_of_logs, KL_SIZE);

    return 0;
}

static int test_klog_clear() {
    int number_of_logs = 2 * KL_SIZE + 1;
    kprintf("Testing clear\n");
    for (int i = 1; i <= number_of_logs; ++i)
        klog(KL_ALL, "Testing clear; Message %d, of %d", i, number_of_logs);
    assert(((first_message + KL_SIZE - 1) & (KL_SIZE - 1)) == (last_message & (KL_SIZE - 1)));
    klog_clear();

    number_of_logs = KL_SIZE - 1;
    for (int i = 1; i <= number_of_logs; ++i)
        klog(KL_ALL, "Testing clear; Message %d, of %d", i, number_of_logs);
    assert(((first_message + number_of_logs) & (KL_SIZE - 1)) == (last_message & (KL_SIZE - 1)));
    assert((first_message + number_of_logs) == last_message);
    klog_dump_all();
    assert(first_message == last_message);
    kprintf("Testing clear. Succesful.\n");

    return 0;
}

static int test_klog_flags() {
    kprintf("Testing flags\n");
    int number_of_flags = 17;
    klog(KL_RUNQ, "Testing flag KL_RUNQ %d", KL_RUNQ);
    klog(KL_SLEEPQ, "Testing flag KL_SLEEPQ %d", KL_SLEEPQ);
    klog(KL_CALLOUT, "Testing flag KL_CALLOUT %d", KL_CALLOUT);
    klog(KL_PMAP, "Testing flag KL_PMAP %d", KL_PMAP);
    klog(KL_VM, "Testing flag KL_VM %d", KL_VM);
    klog(KL_MALLOC, "Testing flag KL_MALLOC %d", KL_MALLOC);
    klog(KL_POOL, "Testing flag KL_POOL %d", KL_POOL);
    klog(KL_LOCK, "Testing flag KL_LOCK %d", KL_LOCK);
    klog(KL_SCHED, "Testing flag KL_SCHED %d", KL_SCHED);
    klog(KL_THREAD, "Testing flag KL_THREAD %d", KL_THREAD);
    klog(KL_INTR, "Testing flag KL_INTR %d", KL_INTR);
    klog(KL_DEV, "Testing flag KL_DEV %d", KL_DEV);
    klog(KL_VFS, "Testing flag KL_VFS %d", KL_VFS);
    klog(KL_VNODE, "Testing flag KL_VNODE %d", KL_VNODE);
    klog(KL_PROC, "Testing flag KL_PROC %d", KL_PROC);
    klog(KL_SYSCALL, "Testing flag KL_SYSCALL %d", KL_SYSCALL);
    klog(KL_ALL, "Testing flag KL_ALL %d", KL_ALL);

    if(KL_MASK != KL_ALL)
        assert(((first_message + 2) & (KL_SIZE - 1)) == (last_message & (KL_SIZE - 1)));
    else if (KL_MASK == KL_ALL && KL_SIZE > number_of_flags)
        assert(((first_message + number_of_flags) & (KL_SIZE - 1)) == (last_message & (KL_SIZE - 1)));
    klog_dump_all();
    kprintf("Testing flags. Succesful.\n");

    return 0;
}

static int test_klog() {
  kprintf("Testing klog\n");
  assert(test_klog_shorter() == 0);
  assert(test_klog_equal() == 0);
  assert(test_klog_longer() == 0);
  assert(test_klog_long() == 0);
  assert(test_klog_clear() == 0);
  assert(test_klog_flags() == 0);
  kprintf("Testing klog. Succesful!\n");

  return 0;
}

KTEST_ADD(klog, test_klog);
