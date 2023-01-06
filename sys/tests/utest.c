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

UTEST_ADD_SIMPLE(vmmap_text_access);
UTEST_ADD_SIMPLE(vmmap_data_access);
UTEST_ADD_SIMPLE(vmmap_rodata_access);

UTEST_ADD_SIMPLE(mmap);
UTEST_ADD_SIMPLE(munmap);
UTEST_ADD_SIMPLE(munmap_sigsegv);
UTEST_ADD_SIMPLE(munmap_many_1);
UTEST_ADD_SIMPLE(munmap_many_2);
UTEST_ADD_SIMPLE(mmap_prot_none);
UTEST_ADD_SIMPLE(mmap_prot_read);
UTEST_ADD_SIMPLE(mmap_fixed);
UTEST_ADD_SIMPLE(mmap_fixed_excl);
UTEST_ADD_SIMPLE(mmap_fixed_replace);
UTEST_ADD_SIMPLE(mmap_fixed_replace_many_1);
UTEST_ADD_SIMPLE(mmap_fixed_replace_many_2);
UTEST_ADD_SIMPLE(mprotect_simple);
UTEST_ADD_SIMPLE(sbrk);
UTEST_ADD_SIMPLE(sbrk_sigsegv);
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
UTEST_ADD_SIMPLE(fd_readv);
UTEST_ADD_SIMPLE(fd_writev);
UTEST_ADD_SIMPLE(fd_all);

UTEST_ADD_SIMPLE(signal_basic);
UTEST_ADD_SIMPLE(signal_send);
UTEST_ADD_SIMPLE(signal_abort);
UTEST_ADD_SIMPLE(signal_segfault);
UTEST_ADD_SIMPLE(signal_stop);
UTEST_ADD_SIMPLE(signal_cont_masked);
UTEST_ADD_SIMPLE(signal_mask);
UTEST_ADD_SIMPLE(signal_mask_nonmaskable);
UTEST_ADD_SIMPLE(signal_sigsuspend);
UTEST_ADD_SIMPLE(signal_sigsuspend_stop);
UTEST_ADD_SIMPLE(signal_handler_mask);

UTEST_ADD_SIMPLE(fork_wait);
UTEST_ADD_SIMPLE(fork_signal);
UTEST_ADD_SIMPLE(fork_sigchld_ignored);

UTEST_ADD_SIMPLE(lseek_basic);
UTEST_ADD_SIMPLE(lseek_errors);

UTEST_ADD_SIMPLE(access_basic);

UTEST_ADD_SIMPLE(stat);
UTEST_ADD_SIMPLE(fstat);

UTEST_ADD_SIMPLE(setjmp);

UTEST_ADD_SIMPLE(sigaction_with_setjmp);
UTEST_ADD_SIMPLE(sigaction_handler_returns);

UTEST_ADD_SIMPLE(vfs_dir);
UTEST_ADD_SIMPLE(vfs_relative_dir);
UTEST_ADD_SIMPLE(vfs_rw);
UTEST_ADD_SIMPLE(vfs_dot_dot_dir);
UTEST_ADD_SIMPLE(vfs_dot_dir);
UTEST_ADD_SIMPLE(vfs_dot_dot_across_fs);
UTEST_ADD_SIMPLE(vfs_trunc);
UTEST_ADD_SIMPLE(vfs_symlink);
UTEST_ADD_SIMPLE(vfs_link);
UTEST_ADD_SIMPLE(vfs_chmod);

UTEST_ADD_SIMPLE(wait_basic);
UTEST_ADD_SIMPLE(wait_nohang);

#if 0
UTEST_ADD_SIMPLE(fpu_fcsr);
UTEST_ADD_SIMPLE(fpu_gpr_preservation);
UTEST_ADD_SIMPLE(fpu_cpy_ctx_on_fork);
UTEST_ADD_SIMPLE(fpu_ctx_signals);
#endif

#ifdef __mips__
UTEST_ADD_SIMPLE(exc_cop_unusable);
UTEST_ADD_SIMPLE(exc_reserved_instruction);
UTEST_ADD_SIMPLE(exc_unaligned_access);
UTEST_ADD_SIMPLE(exc_integer_overflow);

UTEST_ADD_SIMPLE(exc_sigsys);
#endif

#ifdef __aarch64__
UTEST_ADD_SIMPLE(exc_unknown_instruction);
UTEST_ADD_SIMPLE(exc_msr_instruction);
UTEST_ADD_SIMPLE(exc_mrs_instruction);

UTEST_ADD_SIMPLE(exc_brk);
#endif

UTEST_ADD_SIMPLE(getcwd);
/* XXX UTEST_ADD_SIMPLE(syscall_in_bds); */

UTEST_ADD_SIMPLE(setpgid);
UTEST_ADD_SIMPLE(setpgid_leader);
UTEST_ADD_SIMPLE(setpgid_child);
UTEST_ADD_SIMPLE(kill);
UTEST_ADD_SIMPLE(killpg_same_group);
UTEST_ADD_SIMPLE(killpg_other_group);
UTEST_ADD_SIMPLE(pgrp_orphan);
UTEST_ADD_SIMPLE(session_basic);
UTEST_ADD_SIMPLE(session_login_name);

#ifdef __mips__
UTEST_ADD_SIMPLE(gettimeofday);
#endif
UTEST_ADD_SIMPLE(nanosleep);
UTEST_ADD_SIMPLE(itimer);

UTEST_ADD_SIMPLE(get_set_uid);
UTEST_ADD_SIMPLE(get_set_gid);
UTEST_ADD_SIMPLE(get_set_groups);

UTEST_ADD_SIMPLE(sharing_memory_simple);
UTEST_ADD_SIMPLE(sharing_memory_child_and_grandchild);

UTEST_ADD_SIMPLE(pty_simple);

UTEST_ADD_SIMPLE(tty_canon);
UTEST_ADD_SIMPLE(tty_echo);
UTEST_ADD_SIMPLE(tty_signals);

UTEST_ADD_SIMPLE(procstat);

UTEST_ADD_SIMPLE(pipe_parent_signaled);
UTEST_ADD_SIMPLE(pipe_child_signaled);
UTEST_ADD_SIMPLE(pipe_blocking_flag_manipulation);
UTEST_ADD_SIMPLE(pipe_write_interruptible_sleep);
UTEST_ADD_SIMPLE(pipe_write_errno_eagain);
UTEST_ADD_SIMPLE(pipe_read_interruptible_sleep);
UTEST_ADD_SIMPLE(pipe_read_errno_eagain);
UTEST_ADD_SIMPLE(pipe_read_return_zero);
