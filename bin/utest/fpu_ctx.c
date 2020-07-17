/*
 * Floating Point Unit context preservation tests:
 * - test_fpu_fcsr                  - needs context switch. sleep(0)?
 * - test_fpu_gpr_preservation      - needs context switch. sleep(0)?
 * - test_fpu_cpy_ctx_on_fork       - needs another process using FPU
 *                                    with a little bit of synchronization
 * - test_fpu_ctx_signals
 *
 * Checking 32 FPU FPR's and FPU FCSR register.
 * FCCR, FEXR, FENR need not to be saved, because they are only better
 * structured views of FCSR.
 */

#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "utest.h"

#define TEST_TIME 1000000
#define PROCESSES 10

#define P_TEST_TIME 100000
#define P_PROCESSES 10

#ifdef __mips__

#define MTC1(var, freg) asm volatile("mtc1 %0, " #freg : : "r"(var))
#define MFC1(var, freg) asm volatile("mfc1 %0, " #freg : "=r"(var))

#define MFC1_ASSERT_CMP(freg)                                                  \
  do {                                                                         \
    int value;                                                                 \
    MFC1(value, freg);                                                         \
    assert(value == expected);                                                 \
  } while (0)

static int check_fpu_all_gpr(void *arg) {
  int expected = (int)arg;

  MFC1_ASSERT_CMP($f0);
  MFC1_ASSERT_CMP($f1);
  MFC1_ASSERT_CMP($f2);
  MFC1_ASSERT_CMP($f3);
  MFC1_ASSERT_CMP($f4);
  MFC1_ASSERT_CMP($f5);
  MFC1_ASSERT_CMP($f6);
  MFC1_ASSERT_CMP($f7);
  MFC1_ASSERT_CMP($f8);
  MFC1_ASSERT_CMP($f9);
  MFC1_ASSERT_CMP($f10);
  MFC1_ASSERT_CMP($f11);
  MFC1_ASSERT_CMP($f12);
  MFC1_ASSERT_CMP($f13);
  MFC1_ASSERT_CMP($f14);
  MFC1_ASSERT_CMP($f15);
  MFC1_ASSERT_CMP($f16);
  MFC1_ASSERT_CMP($f17);
  MFC1_ASSERT_CMP($f18);
  MFC1_ASSERT_CMP($f19);
  MFC1_ASSERT_CMP($f20);
  MFC1_ASSERT_CMP($f21);
  MFC1_ASSERT_CMP($f22);
  MFC1_ASSERT_CMP($f23);
  MFC1_ASSERT_CMP($f24);
  MFC1_ASSERT_CMP($f25);
  MFC1_ASSERT_CMP($f26);
  MFC1_ASSERT_CMP($f27);
  MFC1_ASSERT_CMP($f28);
  MFC1_ASSERT_CMP($f29);
  MFC1_ASSERT_CMP($f30);
  MFC1_ASSERT_CMP($f31);

  return 0;
}

static int MTC1_all_gpr(void *_value) {
  int value = (int)_value;

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

  return 0;
}

static int check_fpu_ctx(void *value) {
  MTC1_all_gpr(value);

  for (int i = 0; i < P_TEST_TIME; i++)
    check_fpu_all_gpr(value);

  return 0;
}

int test_fpu_gpr_preservation(void) {
  int seed = 0xbeefface;

  for (int i = 0; i < P_PROCESSES; i++)
    utest_spawn(check_fpu_ctx, (void *)seed + i);

  for (int i = 0; i < PROCESSES; i++)
    utest_child_exited(EXIT_SUCCESS);

  return 0;
}

int test_fpu_cpy_ctx_on_fork(void) {
  void *value = (void *)0xbeefface;

  MTC1_all_gpr(value);
  utest_spawn(MTC1_all_gpr, (void *)0xc0de);
  utest_spawn(check_fpu_all_gpr, (void *)value);

  for (int i = 0; i < 2; i++)
    utest_child_exited(EXIT_SUCCESS);

  return 0;
}

static int check_fcsr(void *arg) {
  int expected = (int)arg;

  MTC1(expected, $25);
  for (int i = 0; i < TEST_TIME; i++)
    MFC1_ASSERT_CMP($25);

  return 0;
}

int test_fpu_fcsr() {
  for (int i = 0; i < PROCESSES; i++)
    utest_spawn(check_fcsr, (void *)i);

  for (int i = 0; i < PROCESSES; i++)
    utest_child_exited(EXIT_SUCCESS);

  return 0;
}

static void signal_handler_usr2(int signo) {
  MTC1_all_gpr((void *)42);
}

static void signal_handler_usr1(int signo) {
  MTC1_all_gpr((void *)1337);

  signal(SIGUSR2, signal_handler_usr2);
  raise(SIGUSR2);

  check_fpu_all_gpr((void *)1337);
}

int test_fpu_ctx_signals() {
  MTC1_all_gpr((void *)0xc0de);

  signal(SIGUSR1, signal_handler_usr1);
  raise(SIGUSR1);

  check_fpu_all_gpr((void *)0xc0de);

  return 0;
}

#endif /* !__mips__ */
