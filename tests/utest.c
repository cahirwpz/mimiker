#define KL_LOG KL_TEST
#include <klog.h>
#include <common.h>
#include <exec.h>
#include <ktest.h>
#include <thread.h>
#include <sched.h>
#include <proc.h>
#include <wait.h>

static void utest_generic_thread(void *arg) {
  const char *test_name = arg;

  exec_args_t exec_args = {.prog_name = "/bin/utest",
                           .argc = 2,
                           .argv = (const char *[]){"utest", test_name}};

  run_program(&exec_args);
}

/* This is the klog mask used with utests. */
#define KL_UTEST_MASK (KL_ALL & (~KL_MASK(KL_PMAP)) & (~KL_MASK(KL_VM)))

static int utest_generic(const char *name, int status_success) {
  unsigned old_klog_mask = klog_setmask(KL_UTEST_MASK);

  thread_t *utest_thread =
    thread_create(name, utest_generic_thread, (void *)name);
  sched_add(utest_thread);

  /* NOTE: It looks like waitpid should be used here... but keep in mind, that
     only the parent process may wait for children, and that this thread does
     not belong to any thread at all. */
  /* XXX: This only works because, for now, a process may only have one
     thread. Normally we would need to wait for this process... but we don't
     know it's PID (and it doesn't have to be 1!) nor a pointer, and we don't
     know when exactly it will become ready. */
  thread_join(utest_thread);
  /* XXX: Normally the operating system has only one "first process", but when
     running tests we create multiple such. They don't have a parent, yet they
     exit or get killed, and we need to clean up afterwards. */
  int status;
  proc_reap(utest_thread->td_proc, &status);

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
UTEST_ADD_SIMPLE(fd_all);

/* XXX UTEST_ADD_SIMPLE(signal_basic); */
/* XXX UTEST_ADD_SIMPLE(signal_send); */
/* XXX UTEST_ADD_SIGNAL(signal_abort, SIGABRT); */
/* XXX UTEST_ADD_SIGNAL(signal_segfault, SIGSEGV); */

/* XXX UTEST_ADD_SIMPLE(fork_wait); */
/* TODO Why this test takes so long to execute? */
/* UTEST_ADD_SIMPLE(fork_signal); */
/* XXX UTEST_ADD_SIMPLE(fork_sigchld_ignored); */

UTEST_ADD_SIMPLE(lseek_basic);
UTEST_ADD_SIMPLE(lseek_errors);

UTEST_ADD_SIMPLE(access_basic);

UTEST_ADD_SIMPLE(execve_basic);
UTEST_ADD_SIMPLE(execve_errors);
UTEST_ADD_SIMPLE(stat);
UTEST_ADD_SIMPLE(fstat);
