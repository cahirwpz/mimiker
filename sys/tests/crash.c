#include <sys/libkern.h>
#include <sys/thread.h>
#include <sys/sched.h>
#include <sys/ktest.h>

__attribute__((noinline)) static int load(intptr_t ptr) {
  return *(volatile int *)ptr;
}

__attribute__((noinline)) static void store(intptr_t ptr, int value) {
  *(volatile int *)ptr = value;
}

static void unaligned_load(void *data) {
  load(0xDEADC0DE);
}

static void unaligned_store(void *data) {
  store(0xDEADC0DE, 666);
}

static void unmapped_load(void *data) {
  load(0x10000000);
}

static void unmapped_store(void *data) {
  store(0x10000000, 666);
}

#define NEW_THREAD(fn, arg)                                                    \
  thread_create("test-crash-" #fn, fn, arg, prio_kthread(0))

static int test_crash(void) {
  sched_add(NEW_THREAD(unaligned_load, NULL));
  sched_add(NEW_THREAD(unaligned_store, NULL));
  sched_add(NEW_THREAD(unmapped_load, NULL));
  sched_add(NEW_THREAD(unmapped_store, NULL));

  sched_run();

  return KTEST_FAILURE;
}

KTEST_ADD(crash, test_crash, KTEST_FLAG_BROKEN);
