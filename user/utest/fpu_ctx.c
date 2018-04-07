/*----------------------------------------------------------------------------
 *
 * Floating Point Unit context preservation tests:
 * - test_fpu_fcsr                  - needs context switch. sleep(0)?
 * - test_fpu_gpr_preservation      - needs context switch. sleep(0)?
 * - test_fpu_cpy_ctx_on_fork       - needs another process using FPU
 * - test_fpu_ctx_signals
 *
 * Checking 32 FPU FPR's and FPU FCSR register.
 * FCCR, FEXR, FENR need not to be saved, because they are only better
 * structured views of FCSR.
 *
 *----------------------------------------------------------------------------
 */

#include "utest.h"
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/wait.h>
#include <sys/signal.h>

#define TEST_TIME 1000000
#define PROCESSES 10

#define P_TEST_TIME 100000
#define P_PROCESSES 10

#define MTC1(var, freg) asm volatile("mtc1 %0, " #freg : : "r"(var))
#define MFC1(var, freg) asm volatile("mfc1 %0, " #freg : "=r"(var))

#define MFC1_ASSERT_CMP(var, freg, cmp)                                        \
  do {                                                                         \
    MFC1(var, freg);                                                           \
    assert(var == cmp);                                                        \
  } while (0)

static inline void check_fpu_all_gpr(int expected_value) {
  int current_value;

  MFC1_ASSERT_CMP(current_value, $f0, expected_value);
  MFC1_ASSERT_CMP(current_value, $f1, expected_value);
  MFC1_ASSERT_CMP(current_value, $f2, expected_value);
  MFC1_ASSERT_CMP(current_value, $f3, expected_value);
  MFC1_ASSERT_CMP(current_value, $f4, expected_value);
  MFC1_ASSERT_CMP(current_value, $f5, expected_value);
  MFC1_ASSERT_CMP(current_value, $f6, expected_value);
  MFC1_ASSERT_CMP(current_value, $f7, expected_value);
  MFC1_ASSERT_CMP(current_value, $f8, expected_value);
  MFC1_ASSERT_CMP(current_value, $f9, expected_value);
  MFC1_ASSERT_CMP(current_value, $f10, expected_value);
  MFC1_ASSERT_CMP(current_value, $f11, expected_value);
  MFC1_ASSERT_CMP(current_value, $f12, expected_value);
  MFC1_ASSERT_CMP(current_value, $f13, expected_value);
  MFC1_ASSERT_CMP(current_value, $f14, expected_value);
  MFC1_ASSERT_CMP(current_value, $f15, expected_value);
  MFC1_ASSERT_CMP(current_value, $f16, expected_value);
  MFC1_ASSERT_CMP(current_value, $f17, expected_value);
  MFC1_ASSERT_CMP(current_value, $f18, expected_value);
  MFC1_ASSERT_CMP(current_value, $f19, expected_value);
  MFC1_ASSERT_CMP(current_value, $f20, expected_value);
  MFC1_ASSERT_CMP(current_value, $f21, expected_value);
  MFC1_ASSERT_CMP(current_value, $f22, expected_value);
  MFC1_ASSERT_CMP(current_value, $f23, expected_value);
  MFC1_ASSERT_CMP(current_value, $f24, expected_value);
  MFC1_ASSERT_CMP(current_value, $f25, expected_value);
  MFC1_ASSERT_CMP(current_value, $f26, expected_value);
  MFC1_ASSERT_CMP(current_value, $f27, expected_value);
  MFC1_ASSERT_CMP(current_value, $f28, expected_value);
  MFC1_ASSERT_CMP(current_value, $f29, expected_value);
  MFC1_ASSERT_CMP(current_value, $f30, expected_value);
  MFC1_ASSERT_CMP(current_value, $f31, expected_value);
}

static inline void MTC1_all_gpr(int value) {
  MTC1(value, $f0);
  MTC1(value, $f1);
  MTC1(value, $f2);
  MTC1(value, $f3);
  MTC1(value, $f4);
  MTC1(value, $f5);
  MTC1(value, $f6);
  MTC1(value, $f7);
  MTC1(value, $f8);
  MTC1(value, $f9);
  MTC1(value, $f10);
  MTC1(value, $f11);
  MTC1(value, $f12);
  MTC1(value, $f13);
  MTC1(value, $f14);
  MTC1(value, $f15);
  MTC1(value, $f16);
  MTC1(value, $f17);
  MTC1(value, $f18);
  MTC1(value, $f19);
  MTC1(value, $f20);
  MTC1(value, $f21);
  MTC1(value, $f22);
  MTC1(value, $f23);
  MTC1(value, $f24);
  MTC1(value, $f25);
  MTC1(value, $f26);
  MTC1(value, $f27);
  MTC1(value, $f28);
  MTC1(value, $f29);
  MTC1(value, $f30);
  MTC1(value, $f31);
}

static int spawn_process(int pproc, void (*proc_handler)(int)) {
  int pid;
  switch ((pid = fork())) {
    case -1:
      exit(EXIT_FAILURE);
    case 0:
      proc_handler(pproc);
      exit(EXIT_SUCCESS);
    default:
      return pid;
  }
}

static void check_fpu_ctx(int value) {
  MTC1_all_gpr(value);
  for (int i = 0; i < P_TEST_TIME; i++) {
    check_fpu_all_gpr(value);
  }
}

int test_fpu_gpr_preservation() {
  int seed = 0xbeefface;
  for (int i = 0; i < P_PROCESSES; i++) {
    spawn_process(seed + i, check_fpu_ctx);
  }

  int status;
  for (int i = 0; i < PROCESSES; i++) {
    wait(&status);
    assert(WIFEXITED(status));
    assert(EXIT_SUCCESS == WEXITSTATUS(status));
  }

  return 0;
}

int test_fpu_cpy_ctx_on_fork() {
  int value = 0xbeefface;
  MTC1_all_gpr(value);
  spawn_process(0xc0de, MTC1_all_gpr);
  spawn_process(value, check_fpu_all_gpr);

  for (int i = 0; i < 2; i++) {
    int status;
    wait(&status);
    assert(WIFEXITED(status));
    assert(EXIT_SUCCESS == WEXITSTATUS(status));
  }

  return 0;
}

static void check_fcsr(int expected_value) {
  MTC1(expected_value, $25);
  for (int i = 0; i < TEST_TIME; i++) {
    int current_value;
    MFC1_ASSERT_CMP(current_value, $25, expected_value);
  }
}

int test_fpu_fcsr() {
  for (int i = 0; i < PROCESSES; i++) {
    spawn_process(i, check_fcsr);
  }

  int status;
  for (int i = 0; i < PROCESSES; i++) {
    wait(&status);
    assert(WIFEXITED(status));
    assert(EXIT_SUCCESS == WEXITSTATUS(status));
  }
  return 0;
}

static void signal_handler_usr2(int signo) {
  MTC1_all_gpr(42);
}

static void signal_handler_usr1(int signo) {
  MTC1_all_gpr(1337);

  signal(SIGUSR2, signal_handler_usr2);
  raise(SIGUSR2);

  check_fpu_all_gpr(1337);
}

int test_fpu_ctx_signals() {
  MTC1_all_gpr(0xc0de);

  signal(SIGUSR1, signal_handler_usr1);
  raise(SIGUSR1);

  check_fpu_all_gpr(0xc0de);

  return 0;
}