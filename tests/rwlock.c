#include <stdc.h>
#include <sched.h>
#include <thread.h>
#include <rwlock.h>
#include <ktest.h>

const char * const test_rwlock_name = "TEST_RWLOCK_NAME";

static void read_lock_test(bool recursive) {
  rwlock_t rw;
  rw_init(&rw, test_rwlock_name, recursive);
  rw_assert(&rw, RW_UNLOCKED);
  rw_enter(&rw, RW_READER);
  rw_assert(&rw, RW_RLOCKED);
  rw_leave(&rw);
  rw_assert(&rw, RW_UNLOCKED);
  rw_destroy(&rw);
}

static void multiple_read_lock_test(bool recursive) {
  rwlock_t rw;
  rw_init(&rw, test_rwlock_name, recursive);
  rw_assert(&rw, RW_UNLOCKED);
  for(int i=0;i<10;i++) {
    rw_enter(&rw, RW_READER);
    rw_assert(&rw, RW_RLOCKED);
  }
  for(int i=0;i<9;i++){
    rw_leave(&rw);
    rw_assert(&rw, RW_RLOCKED);
  }
  rw_leave(&rw);
  rw_assert(&rw, RW_UNLOCKED);
  rw_destroy(&rw);
}

static void write_lock_test(void) {
  rwlock_t rw;
  rw_init(&rw, test_rwlock_name, false);
  rw_assert(&rw, RW_UNLOCKED);
  rw_enter(&rw, RW_WRITER);
  rw_assert(&rw, RW_WLOCKED);
  rw_leave(&rw);
  rw_assert(&rw, RW_UNLOCKED);
  rw_destroy(&rw);
}

static void recursive_write_lock_test(void) {
  rwlock_t rw;
  rw_init(&rw, test_rwlock_name, true);
  rw_assert(&rw, RW_UNLOCKED);
  for(int i=0;i<10;i++) {
    rw_enter(&rw, RW_WRITER);
    rw_assert(&rw, RW_WLOCKED);
  }
  for(int i=0;i<9;i++) {
    rw_leave(&rw);
    rw_assert(&rw, RW_WLOCKED);
  } 
  rw_leave(&rw);
  rw_assert(&rw, RW_UNLOCKED);
  rw_destroy(&rw);
}

static void upgrade_test(void) {
  rwlock_t rw;
  rw_init(&rw, test_rwlock_name, false);
  rw_assert(&rw, RW_UNLOCKED);
  rw_enter(&rw, RW_READER);
  rw_assert(&rw, RW_RLOCKED);
  assert(rw_try_upgrade(&rw));
  rw_assert(&rw, RW_WLOCKED);
  rw_leave(&rw);
  rw_assert(&rw, RW_UNLOCKED);
  rw_destroy(&rw);
}

static void downgrade_test(void) {
  rwlock_t rw;
  rw_init(&rw, test_rwlock_name, false);
  rw_assert(&rw, RW_UNLOCKED);
  rw_enter(&rw, RW_WRITER);
  rw_assert(&rw, RW_WLOCKED);
  rw_downgrade(&rw);
  rw_assert(&rw, RW_RLOCKED);
  rw_leave(&rw);
  rw_assert(&rw, RW_UNLOCKED);
  rw_destroy(&rw);
}

static int test_rwlock(void) {
  read_lock_test(false);
  log("read_lock_test passed");
  multiple_read_lock_test(false);
  log("multiple_read_lock_test passed");
  read_lock_test(true);
  log("read_lock_test passed");
  multiple_read_lock_test(true);
  log("multiple_read_lock_test passed");
  write_lock_test();
  log("write_lock_test passed");
  recursive_write_lock_test();
  log("recursive_write_lock_test passed");
  upgrade_test();
  log("upgrade_test passed");
  downgrade_test();
  log("downgrade_test passed");
  return KTEST_SUCCESS;
}

KTEST_ADD(rwlock, test_rwlock, 0);
