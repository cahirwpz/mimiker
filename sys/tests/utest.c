#include <sys/klog.h>
#include <sys/mimiker.h>
#include <sys/exec.h>
#include <sys/kenv.h>
#include <sys/ktest.h>
#include <sys/thread.h>
#include <sys/proc.h>
#include <sys/wait.h>

#define UTEST_PATH "/bin/utest"

static __noreturn void utest_generic_thread(void *arg) {
  proc_t *p = proc_self();
  /* Run user tests in a separate session. */
  int error = session_enter(p);
  assert(error == 0);
  kern_execve(UTEST_PATH, (char *[]){UTEST_PATH, arg, NULL}, (char *[]){NULL});
}

/* This is the klog mask used with utests. */
#define KL_UTEST_MASK                                                          \
  (KL_ALL & (~(KL_MASK(KL_INTR) | KL_MASK(KL_PMAP) | KL_MASK(KL_PHYSMEM))))

static int utest_generic(const char *name, int status_success) {
  const char *utest_mask = kenv_get("klog-utest-mask");
  unsigned new_klog_mask =
    utest_mask ? (unsigned)strtol(utest_mask, NULL, 16) : KL_UTEST_MASK;
  unsigned old_klog_mask = klog_setmask(new_klog_mask);

  /* Prefix test thread's name with `utest-`. */
  char prefixed_name[TD_NAME_MAX];
  snprintf(prefixed_name, TD_NAME_MAX, "utest-%s", name);

  klog("User test '%s' started", name);

  pid_t cpid;
  if (do_fork(utest_generic_thread, (void *)name, &cpid))
    panic("Could not start test!");

  int status;
  pid_t pid;
  do_waitpid(cpid, &status, 0, &pid);
  assert(cpid == pid);

  /* Restore previous klog mask */
  /* XXX: If we'll use klog_setmask heavily, maybe we should consider
     klog_{push,pop}_mask. */
  klog_setmask(old_klog_mask);

  klog("User test '%s' finished with status: %d (expected: %d)", name, status,
       status_success);
  if (status == status_success)
    return KTEST_SUCCESS;
  else
    return KTEST_FAILURE;
}

/* Adds a new test executed by running /bin/utest. */
#define UTEST_ADD(name)                                                        \
  static int utest_test_##name(void) {                                         \
    return utest_generic(#name, MAKE_STATUS_EXIT(0));                          \
  }                                                                            \
  KTEST_ADD(user_##name, utest_test_##name, KTEST_FLAG_USERMODE)

UTEST_ADD(vmmap_text_access);
UTEST_ADD(vmmap_data_access);
UTEST_ADD(vmmap_rodata_access);

UTEST_ADD(mmap);
UTEST_ADD(munmap);
UTEST_ADD(mmap_private);
UTEST_ADD(munmap_sigsegv);
UTEST_ADD(munmap_many_1);
UTEST_ADD(munmap_many_2);
UTEST_ADD(mmap_prot_none);
UTEST_ADD(mmap_prot_read);
UTEST_ADD(mmap_fixed);
UTEST_ADD(mmap_fixed_excl);
UTEST_ADD(mmap_fixed_replace);
UTEST_ADD(mmap_fixed_replace_many_1);
UTEST_ADD(mmap_fixed_replace_many_2);
UTEST_ADD(mprotect_fail);
UTEST_ADD(mprotect1);
UTEST_ADD(mprotect2);
UTEST_ADD(mprotect3);
UTEST_ADD(sbrk);
UTEST_ADD(sbrk_sigsegv);
UTEST_ADD(misbehave);

UTEST_ADD(fd_read);
UTEST_ADD(fd_devnull);
UTEST_ADD(fd_multidesc);
UTEST_ADD(fd_readwrite);
UTEST_ADD(fd_copy);
UTEST_ADD(fd_bad_desc);
UTEST_ADD(fd_open_path);
UTEST_ADD(fd_dup);
UTEST_ADD(fd_pipe);
UTEST_ADD(fd_readv);
UTEST_ADD(fd_writev);
UTEST_ADD(fd_all);

UTEST_ADD(signal_basic);
UTEST_ADD(signal_send);
UTEST_ADD(signal_abort);
UTEST_ADD(signal_segfault);
UTEST_ADD(signal_stop);
UTEST_ADD(signal_cont_masked);
UTEST_ADD(signal_mask);
UTEST_ADD(signal_mask_nonmaskable);
UTEST_ADD(signal_sigtimedwait);
UTEST_ADD(signal_sigtimedwait_timeout);
UTEST_ADD(signal_sigtimedwait_intr);
UTEST_ADD(signal_sigsuspend);
UTEST_ADD(signal_sigsuspend_stop);
UTEST_ADD(signal_handler_mask);

UTEST_ADD(fork_wait);
UTEST_ADD(fork_signal);
UTEST_ADD(fork_sigchld_ignored);

UTEST_ADD(lseek_basic);
UTEST_ADD(lseek_errors);

UTEST_ADD(access_basic);

UTEST_ADD(stat);
UTEST_ADD(fstat);

UTEST_ADD(setjmp);

UTEST_ADD(sigaction_with_setjmp);
UTEST_ADD(sigaction_handler_returns);

UTEST_ADD(vfs_dir);
UTEST_ADD(vfs_relative_dir);
UTEST_ADD(vfs_rw);
UTEST_ADD(vfs_dot_dot_dir);
UTEST_ADD(vfs_dot_dir);
UTEST_ADD(vfs_dot_dot_across_fs);
UTEST_ADD(vfs_trunc);
UTEST_ADD(vfs_symlink);
UTEST_ADD(vfs_link);
UTEST_ADD(vfs_chmod);

UTEST_ADD(wait_basic);
UTEST_ADD(wait_nohang);

#if 0
UTEST_ADD(fpu_fcsr);
UTEST_ADD(fpu_gpr_preservation);
UTEST_ADD(fpu_cpy_ctx_on_fork);
UTEST_ADD(fpu_ctx_signals);
#endif

#ifdef __mips__
UTEST_ADD(exc_cop_unusable);
UTEST_ADD(exc_reserved_instruction);
UTEST_ADD(exc_unaligned_access);
UTEST_ADD(exc_integer_overflow);

UTEST_ADD(exc_sigsys);
#endif

#ifdef __aarch64__
UTEST_ADD(exc_unknown_instruction);
UTEST_ADD(exc_msr_instruction);
UTEST_ADD(exc_mrs_instruction);

UTEST_ADD(exc_brk);
#endif

UTEST_ADD(getcwd);
/* XXX UTEST_ADD(syscall_in_bds); */

UTEST_ADD(setpgid);
UTEST_ADD(setpgid_leader);
UTEST_ADD(setpgid_child);
UTEST_ADD(kill);
UTEST_ADD(killpg_same_group);
UTEST_ADD(killpg_other_group);
UTEST_ADD(pgrp_orphan);
UTEST_ADD(session_basic);
UTEST_ADD(session_login_name);

#ifdef __mips__
UTEST_ADD(gettimeofday);
#endif
// UTEST_ADD(nanosleep);
UTEST_ADD(itimer);

UTEST_ADD(get_set_uid);
UTEST_ADD(get_set_gid);
UTEST_ADD(get_set_groups);

UTEST_ADD(sharing_memory_simple);
UTEST_ADD(sharing_memory_child_and_grandchild);

UTEST_ADD(pty_simple);

UTEST_ADD(tty_canon);
UTEST_ADD(tty_echo);
UTEST_ADD(tty_signals);

UTEST_ADD(procstat);

UTEST_ADD(pipe_parent_signaled);
UTEST_ADD(pipe_child_signaled);
UTEST_ADD(pipe_blocking_flag_manipulation);
UTEST_ADD(pipe_write_interruptible_sleep);
UTEST_ADD(pipe_write_errno_eagain);
UTEST_ADD(pipe_read_interruptible_sleep);
UTEST_ADD(pipe_read_errno_eagain);
UTEST_ADD(pipe_read_return_zero);
