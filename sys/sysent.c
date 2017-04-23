#include <sysent.h>
#include <stdc.h>
#include <errno.h>
#include <thread.h>
#include <vm_map.h>
#include <vm_pager.h>
#include <mmap.h>
#include <vfs_syscalls.h>
#include <fork.h>
#include <sbrk.h>
#include <signal.h>

int sys_nosys(thread_t *td, syscall_args_t *args) {
  kprintf("[syscall] unimplemented system call %ld\n", args->code);
  return -ENOSYS;
};

int sys_sbrk(thread_t *td, syscall_args_t *args) {
  intptr_t increment = (size_t)args->args[0];

  kprintf("[syscall] sbrk(%zu)\n", increment);

  /* TODO: Shrinking sbrk is impossible, because it requires unmapping pages,
   * which is not yet implemented! */
  if (increment < 0) {
    log("WARNING: sbrk called with a negative argument!");
    return -ENOMEM;
  }

  assert(td->td_uspace);

  return sbrk_resize(td->td_uspace, increment);
}

/* This is just a stub. A full implementation of this syscall will probably
   deserve a separate file. */
int sys_exit(thread_t *td, syscall_args_t *args) {
  int status = args->args[0];

  kprintf("[syscall] exit(%d)\n", status);

  thread_exit(status);
  __builtin_unreachable();
}

int sys_fork(thread_t *td, syscall_args_t *args) {
  kprintf("[syscall] fork()\n");
  return do_fork();
}

int sys_getpid(thread_t *td, syscall_args_t *args) {
  kprintf("[syscall] getpid()\n");
  return td->td_tid;
}

int sys_kill(thread_t *td, syscall_args_t *args) {
  tid_t tid = args->args[0];
  signo_t sig = args->args[1];
  kprintf("[syscall] kill(%lu, %d)\n", tid, sig);

  return do_kill(tid, sig);
}

int sys_sigaction(thread_t *td, syscall_args_t *args) {
  int signo = args->args[0];
  char *p_newact = (char *)args->args[1];
  char *p_oldact = (char *)args->args[2];
  kprintf("[syscall] sigaction(%d, %p, %p)\n", signo, p_newact, p_oldact);

  sigaction_t newact;
  sigaction_t oldact;

  /* Copy in newact. */

  int res = do_sigaction(signo, &newact, &oldact);
  if (res < 0)
    return res;

  /* If p_oldact != null, copyout oldact. */

  return res;
}

/* clang-format hates long arrays. */
sysent_t sysent[] =
  {[SYS_EXIT] = {sys_exit},    [SYS_OPEN] = {sys_open},
   [SYS_CLOSE] = {sys_close},  [SYS_READ] = {sys_read},
   [SYS_WRITE] = {sys_write},  [SYS_LSEEK] = {sys_lseek},
   [SYS_UNLINK] = {sys_nosys}, [SYS_GETPID] = {sys_getpid},
   [SYS_KILL] = {sys_kill},    [SYS_FSTAT] = {sys_fstat},
   [SYS_SBRK] = {sys_sbrk},    [SYS_MMAP] = {sys_mmap},
   [SYS_FORK] = {sys_fork},    [SYS_SIGACTION] = {sys_sigaction}};
