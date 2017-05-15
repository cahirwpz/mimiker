#define KL_LOG KL_SYSCALL
#include <klog.h>
#include <sysent.h>
#include <stdc.h>
#include <errno.h>
#include <thread.h>
#include <vm_map.h>
#include <vm_pager.h>
#include <mmap.h>
#include <vfs.h>
#include <fork.h>
#include <sbrk.h>
#include <signal.h>
#include <proc.h>
#include <systm.h>

/* Empty syscall handler, for unimplemented and deprecated syscall numbers. */
static int sys_nosys(thread_t *td, syscall_args_t *args) {
  klog("unimplemented system call %ld", args->code);
  return -ENOSYS;
};

static int sys_sbrk(thread_t *td, syscall_args_t *args) {
  intptr_t increment = (size_t)args->args[0];

  klog("sbrk(%zu)", increment);

  /* TODO: Shrinking sbrk is impossible, because it requires unmapping pages,
   * which is not yet implemented! */
  if (increment < 0) {
    klog("WARNING: sbrk called with a negative argument!");
    return -ENOMEM;
  }

  assert(td->td_proc);

  return sbrk_resize(td->td_proc->p_uspace, increment);
}

/* This is just a stub. A full implementation of this syscall will probably
   deserve a separate file. */
static int sys_exit(thread_t *td, syscall_args_t *args) {
  int status = args->args[0];

  klog("exit(%d)", status);

  thread_exit(status);
  __unreachable();
}

static int sys_fork(thread_t *td, syscall_args_t *args) {
  klog("fork()");
  return do_fork();
}

static int sys_getpid(thread_t *td, syscall_args_t *args) {
  klog("getpid()");
  return td->td_proc->p_pid;
}

static int sys_kill(thread_t *td, syscall_args_t *args) {
  pid_t pid = args->args[0];
  signo_t sig = args->args[1];
  klog("kill(%lu, %d)", pid, sig);
  return do_kill(pid, sig);
}

static int sys_sigaction(thread_t *td, syscall_args_t *args) {
  int signo = args->args[0];
  char *p_newact = (char *)args->args[1];
  char *p_oldact = (char *)args->args[2];

  klog("sigaction(%d, %p, %p)", signo, p_newact, p_oldact);

  sigaction_t newact;
  sigaction_t oldact;
  copyin(p_newact, &newact, sizeof(sigaction_t));

  int res = do_sigaction(signo, &newact, &oldact);
  if (res < 0)
    return res;

  if (p_oldact != NULL)
    copyout(&oldact, p_oldact, sizeof(sigaction_t));

  return res;
}

static int sys_sigreturn(thread_t *td, syscall_args_t *args) {
  klog("sigreturn()");
  return do_sigreturn();
}

sysent_t sysent[] = {[SYS_EXIT] = {sys_exit},
                     [SYS_OPEN] = {sys_open},
                     [SYS_CLOSE] = {sys_close},
                     [SYS_READ] = {sys_read},
                     [SYS_WRITE] = {sys_write},
                     [SYS_LSEEK] = {sys_lseek},
                     [SYS_UNLINK] = {sys_nosys},
                     [SYS_GETPID] = {sys_getpid},
                     [SYS_KILL] = {sys_kill},
                     [SYS_FSTAT] = {sys_fstat},
                     [SYS_SBRK] = {sys_sbrk},
                     [SYS_MMAP] = {sys_mmap},
                     [SYS_FORK] = {sys_fork},
                     [SYS_MOUNT] = {sys_mount},
                     [SYS_GETDENTS] = {sys_getdirentries},
                     [SYS_SIGACTION] = {sys_sigaction},
                     [SYS_SIGRETURN] = {sys_sigreturn}};
