#define KL_LOG KL_TEST
#include <klog.h>
#include <common.h>
#include <exec.h>
#include <ktest.h>
#include <thread.h>
#include <sched.h>
#include <proc.h>
#include <wait.h>
#include <syslimits.h>
#include <malloc.h>
#include <stack.h>

static void utest_generic_thread(void *arg) {
  /* exec_args_t exec_args = {.prog_name = "/bin/utest", */
  /*                          .argc = 2, */
  /*                          .argv = (const char *[]){"utest", test_name}}; */

  /* run_program(&exec_args); */

  const char *test_name = arg;
  const char *argv[] = {"utest", test_name};

  size_t blob_size = roundup(roundup(sizeof(size_t) + 2 * sizeof(char *), 8) +
                               roundup(strlen("utest") + 1, 4) +
                               roundup(strlen(test_name) + 1, 4),
                             8);

  int8_t arg_blob[blob_size];

  /*  size_t blob_size = ARG_MAX; */

  /*  klog("@@@@@@@@@@@before kmalloc\n"); */

  /* int8_t* arg_blob = kmalloc(M_TEMP, blob_size, 0); */

  /*   klog("@@@@@@@@@@@after kmalloc\n"); */

  /*  klog("@@@@@@@@@@@arg_blob is %x", arg_blob); */
  size_t bytes_written;

  kspace_stack_image_setup(argv, arg_blob, blob_size, &bytes_written);

  assert(bytes_written == blob_size);

  exec_args_t exec_args = {.prog_name = "/bin/utest",
                           .stack_image = arg_blob,
                           .stack_byte_cnt = bytes_written};

  run_program(&exec_args);

  // kfree(M_TEMP, arg_blob);
}

/* This is the klog mask used with utests. */
#define KL_UTEST_MASK (KL_ALL & (~KL_MASK(KL_PMAP)) & (~KL_MASK(KL_VM)))

static int utest_generic(const char *name, int status_success) {
  unsigned old_klog_mask = klog_setmask(KL_UTEST_MASK);

  thread_t *utest_thread =
    thread_create(name, utest_generic_thread, (void *)name);
  proc_t *child = proc_create(utest_thread, proc_self());
  sched_add(utest_thread);

  int status;
  do_waitpid(child->p_pid, &status, 0);

  /* Restore previous klog mask */
  /* XXX: If we'll use klog_setmask heavily, maybe we should consider
     klog_{push,pop}_mask. */
  klog_setmask(old_klog_mask);

  klog("User test %s finished with status: %d, expected: %d", name, status,
       status_success);
  if (status == status_success)
    return KTEST_SUCCESS;
  else
    return KTEST_FAILURE;
}

/* Adds a new test executed by running /bin/utest. */
#define UTEST_ADD(name, status_success, flags)                                 \
  static int utest_test_##name(void) {                                         \
    return utest_generic(#name, status_success);                               \
  }                                                                            \
  KTEST_ADD(user_##name, utest_test_##name, flags | KTEST_FLAG_USERMODE);

#define UTEST_ADD_SIMPLE(name) UTEST_ADD(name, MAKE_STATUS_EXIT(0), 0)
#define UTEST_ADD_SIGNAL(name, sig)                                            \
  UTEST_ADD(name, MAKE_STATUS_SIG_TERM(sig), 0)

UTEST_ADD_SIMPLE(mmap);
UTEST_ADD_SIMPLE(sbrk);
UTEST_ADD_SIMPLE(misbehave);

UTEST_ADD_SIMPLE(fd_read);
UTEST_ADD_SIMPLE(fd_devnull);
UTEST_ADD_SIMPLE(fd_multidesc);
UTEST_ADD_SIMPLE(fd_readwrite);
UTEST_ADD_SIMPLE(fd_copy);
UTEST_ADD_SIMPLE(fd_bad_desc);
UTEST_ADD_SIMPLE(fd_open_path);
UTEST_ADD_SIMPLE(fd_dup);
UTEST_ADD_SIMPLE(fd_pipe);
UTEST_ADD_SIMPLE(fd_all);

UTEST_ADD_SIMPLE(signal_basic);
UTEST_ADD_SIMPLE(signal_send);
UTEST_ADD_SIGNAL(signal_abort, SIGABRT);
UTEST_ADD_SIGNAL(signal_segfault, SIGSEGV);

UTEST_ADD_SIMPLE(fork_wait);
UTEST_ADD_SIMPLE(fork_signal);
UTEST_ADD_SIMPLE(fork_sigchld_ignored);

UTEST_ADD_SIMPLE(lseek_basic);
UTEST_ADD_SIMPLE(lseek_errors);

UTEST_ADD_SIMPLE(access_basic);

UTEST_ADD_SIMPLE(execve_basic);
UTEST_ADD_SIMPLE(execve_errors);
UTEST_ADD_SIMPLE(stat);
UTEST_ADD_SIMPLE(fstat);

#if 0
UTEST_ADD_SIMPLE(fpu_fcsr);
UTEST_ADD_SIMPLE(fpu_gpr_preservation);
UTEST_ADD_SIMPLE(fpu_cpy_ctx_on_fork);
UTEST_ADD_SIMPLE(fpu_ctx_signals);
#endif

UTEST_ADD_SIGNAL(exc_cop_unusable, SIGILL);
UTEST_ADD_SIGNAL(exc_reserved_instruction, SIGILL);
UTEST_ADD_SIGNAL(exc_unaligned_access, SIGBUS);
UTEST_ADD_SIGNAL(exc_integer_overflow, SIGFPE);

UTEST_ADD_SIMPLE(exc_sigsys);
/* XXX UTEST_ADD_SIMPLE(syscall_in_bds); */
