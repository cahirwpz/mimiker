#include <sys/mimiker.h>
#include <sys/ktest.h>
#include <sys/klog.h>

#define report_error                                                           \
  do {                                                                         \
    kprintf("KUBSAN: %s\n", __func__);                                         \
  } while (0)

static void kubsan_fail(void) {
  if (ktest_test_running_flag)
    ktest_failure();
  else
    panic_fail();
}

void __ubsan_handle_type_mismatch_v1(void) {
  report_error;
  kubsan_fail();
}

void __ubsan_handle_pointer_overflow(void) {
  report_error;
  kubsan_fail();
}

void __ubsan_handle_out_of_bounds(void) {
  report_error;
  kubsan_fail();
}

void __ubsan_handle_add_overflow(void) {
  report_error;
  kubsan_fail();
}

void __ubsan_handle_sub_overflow(void) {
  report_error;
  kubsan_fail();
}

void __ubsan_handle_mul_overflow(void) {
  report_error;
  kubsan_fail();
}

void __ubsan_handle_divrem_overflow(void) {
  report_error;
  kubsan_fail();
}

void __ubsan_handle_shift_out_of_bounds(void) {
  report_error;
  kubsan_fail();
}

void __ubsan_handle_negate_overflow(void) {
  report_error;
  kubsan_fail();
}

void __ubsan_handle_load_invalid_value(void) {
  report_error;
  kubsan_fail();
}

void __ubsan_handle_invalid_builtin(void) {
  report_error;
  kubsan_fail();
}

void __ubsan_handle_vla_bound_not_positive(void) {
  report_error;
  kubsan_fail();
}

void __ubsan_handle_nonnull_arg(void) {
  report_error;
  kubsan_fail();
}
