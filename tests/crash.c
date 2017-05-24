#include <stdc.h>
#include <thread.h>
#include <sched.h>
#include <mips/mips.h>
#include <ktest.h>

__attribute__((noinline)) static int load(intptr_t ptr) {
  return *(volatile int *)ptr;
}

__attribute__((noinline)) static void store(intptr_t ptr, int value) {
  *(volatile int *)ptr = value;
}

static void unaligned_load(void *arg) {
   (void)arg;
  load(0xDEADC0DE);
}

static void unaligned_store(void *arg) {
   (void)arg;
  store(0xDEADC0DE, 666);
}

static void unmapped_load(void *arg) {
   (void)arg;
  load(0x10000000);
}

static void unmapped_store(void *arg) {
   (void)arg;
  store(0x10000000, 666);
}

#define NEW_THREAD(fn, arg) thread_create(#fn, fn, arg)

static int test_crash() {
  sched_add(NEW_THREAD(unaligned_load, NULL));
  sched_add(NEW_THREAD(unaligned_store, NULL));
  sched_add(NEW_THREAD(unmapped_load, NULL));
  sched_add(NEW_THREAD(unmapped_store, NULL));

  sched_run();

  return KTEST_FAILURE;
}

KTEST_ADD(crash, test_crash, KTEST_FLAG_NORETURN);
