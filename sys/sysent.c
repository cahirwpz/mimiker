#define KL_LOG KL_SYSCALL
#include <klog.h>
#include <sysent.h>
#include <systm.h>
#include <errno.h>
#include <thread.h>
#include <mman.h>
#include <vfs.h>
#include <vnode.h>
#include <fork.h>
#include <sbrk.h>
#include <signal.h>
#include <proc.h>
#include <systm.h>
#include <wait.h>

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

static int sys_exit(thread_t *td, syscall_args_t *args) {
  int status = args->args[0];

  klog("exit(%d)", status);

  proc_exit(MAKE_STATUS_EXIT(status));
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
  int error;
  if ((error = copyin_s(p_newact, newact)))
    return error;

  int res = do_sigaction(signo, &newact, &oldact);
  if (res < 0)
    return res;

  if (p_oldact != NULL)
    if ((error = copyout_s(oldact, p_oldact)))
      return error;

  return res;
}

static int sys_sigreturn(thread_t *td, syscall_args_t *args) {
  klog("sigreturn()");
  return do_sigreturn();
}

static int sys_mmap(thread_t *td, syscall_args_t *args) {
  vm_addr_t addr = args->args[0];
  size_t length = args->args[1];
  vm_prot_t prot = args->args[2];
  int flags = args->args[3];

  klog("mmap(%p, %zu, %d, %d)", (void *)addr, length, prot, flags);

  int error = 0;
  vm_addr_t result = do_mmap(addr, length, prot, flags, &error);
  if (error < 0)
    return -error;
  return result;
}

static int sys_open(thread_t *td, syscall_args_t *args) {
  char *user_pathname = (char *)args->args[0];
  int flags = args->args[1];
  int mode = args->args[2];

  int error = 0;
  char pathname[256];
  size_t n = 0;

  /* Copyout pathname. */
  error = copyinstr(user_pathname, pathname, sizeof(pathname), &n);
  if (error < 0)
    return error;

  klog("open(\"%s\", %d, %d)", pathname, flags, mode);

  int fd;
  error = do_open(td, pathname, flags, mode, &fd);
  if (error)
    return error;
  return fd;
}

static int sys_close(thread_t *td, syscall_args_t *args) {
  int fd = args->args[0];

  klog("close(%d)", fd);

  return do_close(td, fd);
}

static int sys_read(thread_t *td, syscall_args_t *args) {
  int fd = args->args[0];
  char *ubuf = (char *)(uintptr_t)args->args[1];
  size_t count = args->args[2];

  klog("sys_read(%d, %p, %zu)", fd, ubuf, count);

  uio_t uio;
  uio = UIO_SINGLE_USER(UIO_READ, 0, ubuf, count);
  int error = do_read(td, fd, &uio);
  if (error)
    return error;
  return count - uio.uio_resid;
}

static int sys_write(thread_t *td, syscall_args_t *args) {
  int fd = args->args[0];
  char *ubuf = (char *)(uintptr_t)args->args[1];
  size_t count = args->args[2];

  klog("sys_write(%d, %p, %zu)", fd, ubuf, count);

  uio_t uio;
  uio = UIO_SINGLE_USER(UIO_WRITE, 0, ubuf, count);
  int error = do_write(td, fd, &uio);
  if (error)
    return error;
  return count - uio.uio_resid;
}

static int sys_lseek(thread_t *td, syscall_args_t *args) {
  int fd = args->args[0];
  off_t offset = args->args[1];
  int whence = args->args[2];

  klog("sys_lseek(%d, %ld, %d)", fd, offset, whence);

  return do_lseek(td, fd, offset, whence);
}

static int sys_fstat(thread_t *td, syscall_args_t *args) {
  int fd = args->args[0];
  char *buf = (char *)args->args[1];

  klog("sys_fstat(%d, %p)", fd, buf);

  vattr_t attr_buf;
  int error = do_fstat(td, fd, &attr_buf);
  if (error)
    return error;
  error = copyout_s(attr_buf, buf);
  if (error < 0)
    return error;
  return 0;
}

static int sys_mount(thread_t *td, syscall_args_t *args) {
  char *user_fsysname = (char *)args->args[0];
  char *user_pathname = (char *)args->args[1];

  int error = 0;
  const int PATHSIZE_MAX = 256;
  char *fsysname = kmalloc(M_TEMP, PATHSIZE_MAX, 0);
  char *pathname = kmalloc(M_TEMP, PATHSIZE_MAX, 0);
  size_t n = 0;

  /* Copyout fsysname. */
  error = copyinstr(user_fsysname, fsysname, sizeof(fsysname), &n);
  if (error < 0)
    goto end;
  n = 0;
  /* Copyout pathname. */
  error = copyinstr(user_pathname, pathname, sizeof(pathname), &n);
  if (error < 0)
    goto end;

  klog("mount(\"%s\", \"%s\")", pathname, fsysname);

  error = do_mount(td, fsysname, pathname);
end:
  kfree(M_TEMP, fsysname);
  kfree(M_TEMP, pathname);
  return error;
}

static int sys_getdirentries(thread_t *td, syscall_args_t *args) {
  int fd = args->args[0];
  char *ubuf = (char *)args->args[1];
  size_t count = args->args[2];
  off_t *base_p = (off_t *)args->args[3];
  off_t base;

  klog("getdirentries(%d, %p, %zu, %p)", fd, ubuf, count, base_p);

  uio_t uio = UIO_SINGLE_USER(UIO_READ, 0, ubuf, count);
  int res = do_getdirentries(td, fd, &uio, &base);
  if (res < 0)
    return res;
  if (base_p != NULL)
    res = copyout_s(base, base_p);
  return res;
}

static int sys_dup(thread_t *td, syscall_args_t *args) {
  int old = args->args[0];
  klog("dup(%d)", old);
  return do_dup(td, old);
}

static int sys_dup2(thread_t *td, syscall_args_t *args) {
  int old = args->args[0];
  int new = args->args[1];
  klog("dup2(%d, %d)", old, new);
  return do_dup2(td, old, new);
}

static int sys_waitpid(thread_t *td, syscall_args_t *args) {
  pid_t pid = args->args[0];
  int *status_p = (int *)args->args[1];
  int options = args->args[2];
  int status = 0;

  klog("waitpid(%d, %x, %d)", pid, status_p, options);

  int res = do_waitpid(pid, &status, options);
  if (res < 0)
    return res;
  if (status_p != NULL)
    res = copyout_s(status, status_p);
  return res;
}

/* clang-format hates long arrays. */
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
                     [SYS_SIGRETURN] = {sys_sigreturn},
                     [SYS_DUP] = {sys_dup},
                     [SYS_DUP2] = {sys_dup2},
                     [SYS_WAITPID] = {sys_waitpid}};
