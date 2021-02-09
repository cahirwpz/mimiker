#include <sys/klog.h>
#include <sys/libkern.h>
#include <sys/sched.h>
#include <sys/thread.h>
#include <sys/rwlock.h>
#include <sys/ktest.h>

const char *const test_rwlock_name = "TEST_RWLOCK_NAME";

static int read_lock(bool recursive) {
  rwlock_t rw;
  rw_init(&rw, test_rwlock_name, recursive);
  rw_assert(&rw, RW_UNLOCKED);
  rw_enter(&rw, RW_READER);
  rw_assert(&rw, RW_RLOCKED);
  rw_leave(&rw);
  rw_assert(&rw, RW_UNLOCKED);
  rw_destroy(&rw);
  return KTEST_SUCCESS;
}

static int rwlock_read_lock(void) {
  return read_lock(false);
}

static int recursive_rwlock_read_lock(void) {
  return read_lock(true);
}

static int multiple_read_locks(bool recursive) {
  rwlock_t rw;
  rw_init(&rw, test_rwlock_name, recursive);
  rw_assert(&rw, RW_UNLOCKED);
  for (int i = 0; i < 10; i++) {
    rw_enter(&rw, RW_READER);
    rw_assert(&rw, RW_RLOCKED);
  }
  for (int i = 0; i < 9; i++) {
    rw_leave(&rw);
    rw_assert(&rw, RW_RLOCKED);
  }
  rw_leave(&rw);
  rw_assert(&rw, RW_UNLOCKED);
  rw_destroy(&rw);
  return KTEST_SUCCESS;
}

static int rwlock_multiple_read_locks(void) {
  return multiple_read_locks(false);
}

static int recursive_rwlock_multiple_read_locks(void) {
  return multiple_read_locks(true);
}

static int rwlock_write_lock(void) {
  rwlock_t rw;
  rw_init(&rw, test_rwlock_name, false);
  rw_assert(&rw, RW_UNLOCKED);
  rw_enter(&rw, RW_WRITER);
  rw_assert(&rw, RW_WLOCKED);
  rw_leave(&rw);
  rw_assert(&rw, RW_UNLOCKED);
  rw_destroy(&rw);
  return KTEST_SUCCESS;
}

static int recursive_rwlock_write_locks(void) {
  rwlock_t rw;
  rw_init(&rw, test_rwlock_name, true);
  rw_assert(&rw, RW_UNLOCKED);
  for (int i = 0; i < 10; i++) {
    rw_enter(&rw, RW_WRITER);
    rw_assert(&rw, RW_WLOCKED);
  }
  for (int i = 0; i < 9; i++) {
    rw_leave(&rw);
    rw_assert(&rw, RW_WLOCKED);
  }
  rw_leave(&rw);
  rw_assert(&rw, RW_UNLOCKED);
  rw_destroy(&rw);
  return KTEST_SUCCESS;
}

static int upgrade(bool recursive) {
  rwlock_t rw;
  rw_init(&rw, test_rwlock_name, recursive);
  rw_assert(&rw, RW_UNLOCKED);
  rw_enter(&rw, RW_READER);
  rw_assert(&rw, RW_RLOCKED);
  assert(rw_try_upgrade(&rw));
  rw_assert(&rw, RW_WLOCKED);
  rw_leave(&rw);
  rw_assert(&rw, RW_UNLOCKED);
  rw_destroy(&rw);
  return KTEST_SUCCESS;
}

static int rwlock_upgrade(void) {
  return upgrade(false);
}

static int recursive_rwlock_upgrade(void) {
  return upgrade(true);
}

static int downgrade(bool recursive) {
  rwlock_t rw;
  rw_init(&rw, test_rwlock_name, recursive);
  rw_assert(&rw, RW_UNLOCKED);
  rw_enter(&rw, RW_WRITER);
  rw_assert(&rw, RW_WLOCKED);
  rw_downgrade(&rw);
  rw_assert(&rw, RW_RLOCKED);
  rw_leave(&rw);
  rw_assert(&rw, RW_UNLOCKED);
  rw_destroy(&rw);
  return KTEST_SUCCESS;
}

static int rwlock_downgrade(void) {
  return downgrade(false);
}

static int recursive_rwlock_downgrade(void) {
  return downgrade(true);
}

KTEST_ADD(rwlock_read_lock, rwlock_read_lock, 0);
KTEST_ADD(recursive_rwlock_read_lock, recursive_rwlock_read_lock, 0);
KTEST_ADD(rwlock_multiple_read_locks, rwlock_multiple_read_locks, 0);
KTEST_ADD(recursive_rwlock_multiple_read_locks,
          recursive_rwlock_multiple_read_locks, 0);
KTEST_ADD(rwlock_write_lock, rwlock_write_lock, 0);
KTEST_ADD(recursive_rwlock_write_locks, recursive_rwlock_write_locks, 0);
KTEST_ADD(rwlock_upgrade, rwlock_upgrade, 0);
KTEST_ADD(recursive_rwlock_upgrade, recursive_rwlock_upgrade, 0);
KTEST_ADD(rwlock_downgrade, rwlock_downgrade, 0);
KTEST_ADD(recursive_rwlock_downgrade, recursive_rwlock_downgrade, 0);
