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
#include <stat.h>
#include <systm.h>
#include <wait.h>
#include <exec.h>
#include <time.h>
#include <proc.h>
#include <pipe.h>
#include <malloc.h>
#include <syslimits.h>

/* Empty syscall handler, for unimplemented and deprecated syscall numbers. */
int sys_nosys(thread_t *td, syscall_args_t *args) {
  klog("unimplemented system call %ld", args->code);
  return -ENOSYS;
};

static int sys_sbrk(thread_t *td, syscall_args_t *args) {
  intptr_t increment = (size_t)args->args[0];

  klog("sbrk(%d)", increment);

  return sbrk_resize(td->td_proc, increment);
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

static int sys_getppid(thread_t *td, syscall_args_t *args) {
  klog("getppid()");
  assert(td->td_proc->p_parent);
  return td->td_proc->p_parent->p_pid;
}

/* Moves the process to the process group with ID specified by pgid.
 * If pid is zero, then the PID of the calling process is used.
 * If pgid is zero, then the process is moved to process group
 * with ID equal to the PID of the process. */
static int sys_setpgid(thread_t *td, syscall_args_t *args) {
  pid_t pid = args->args[0];
  pgid_t pgid = args->args[1];

  klog("setpgid(%d, %d)", pid, pgid);

  proc_t *p = td->td_proc;

  if (pgid < 0)
    return -EINVAL;

  if (pid == 0)
    pid = p->p_pid;
  if (pgid == 0)
    pgid = pid;

  /* TODO Allow process to call setpgid on its children.
   * TODO Make setpgid accepts pgid equal to ID of any existing process group */
  if (pid != p->p_pid || pgid != p->p_pid)
    return -ENOTSUP;

  return pgrp_enter(p, pgid);
}

/* Gets process group ID of the process with ID specified by pid.
 * If pid equals zero then process ID of the calling process is used. */
static int sys_getpgid(thread_t *td, syscall_args_t *args) {
  pid_t pid = args->args[0];
  proc_t *p = td->td_proc;

  if (pid < 0)
    return -EINVAL;

  if (pid == 0)
    pid = p->p_pid;

  return proc_getpgid(pid);
}

static int sys_kill(thread_t *td, syscall_args_t *args) {
  pid_t pid = args->args[0];
  signo_t sig = args->args[1];
  klog("kill(%lu, %d)", pid, sig);
  return proc_sendsig(pid, sig);
}

/* Sends signal sig to process group with ID equal to pgid. */
static int sys_killpg(thread_t *td, syscall_args_t *args) {
  pgid_t pgid = args->args[0];
  signo_t sig = args->args[1];
  klog("killpg(%lu, %d)", pgid, sig);

  if (pgid == 1 || pgid < 0)
    return -EINVAL;

  /* pgid == 0 => sends signal to our process group
   * pgid  > 1 => sends signal to the process group with ID equal pgid */
  return proc_sendsig(-pgid, sig);
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
  vaddr_t addr = args->args[0];
  size_t length = args->args[1];
  vm_prot_t prot = args->args[2];
  int flags = args->args[3];

  klog("mmap(%p, %u, %d, %d)", (void *)addr, length, prot, flags);

  int error = do_mmap(&addr, length, prot, flags);
  if (error < 0)
    return error;
  return addr;
}

static int sys_munmap(thread_t *td, syscall_args_t *args) {
  vaddr_t addr = args->args[0];
  size_t length = args->args[1];
  klog("munmap(%p, %u)", (void *)addr, length);
  return do_munmap(addr, length);
}

static int sys_mprotect(thread_t *td, syscall_args_t *args) {
  vaddr_t addr = args->args[0];
  size_t length = args->args[1];
  vm_prot_t prot = args->args[2];
  klog("mprotect(%p, %u, %u)", (void *)addr, length, prot);
  return -ENOTSUP;
}

static int sys_open(thread_t *td, syscall_args_t *args) {
  char *user_pathname = (char *)args->args[0];
  int flags = args->args[1];
  mode_t mode = args->args[2];

  int result = 0;
  char *pathname = kmalloc(M_TEMP, PATH_MAX, 0); /* TODO: with statement? */
  size_t n = 0;

  /* Copyout pathname. */
  result = copyinstr(user_pathname, pathname, PATH_MAX, &n);
  if (result < 0)
    goto end;

  klog("open(\"%s\", %d, %d)", pathname, flags, mode);

  int fd;
  result = do_open(td, pathname, flags, mode, &fd);
  if (result == 0)
    result = fd;

end:
  kfree(M_TEMP, pathname);
  return result;
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

  klog("read(%d, %p, %u)", fd, ubuf, count);

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

  klog("write(%d, %p, %u)", fd, ubuf, count);

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

  klog("lseek(%d, %ld, %d)", fd, offset, whence);

  return do_lseek(td, fd, offset, whence);
}

static int sys_fstat(thread_t *td, syscall_args_t *args) {
  int fd = args->args[0];
  stat_t *statbuf_p = (stat_t *)args->args[1];

  klog("fstat(%d, %p)", fd, statbuf_p);

  stat_t statbuf;
  int error = do_fstat(td, fd, &statbuf);
  if (error)
    return error;
  error = copyout_s(statbuf, statbuf_p);
  if (error < 0)
    return error;
  return 0;
}

static int sys_stat(thread_t *td, syscall_args_t *args) {
  char *user_path = (char *)args->args[0];
  stat_t *statbuf_p = (stat_t *)args->args[1];

  char *path = kmalloc(M_TEMP, PATH_MAX, 0);
  size_t path_len = 0;
  int result;

  result = copyinstr(user_path, path, PATH_MAX, &path_len);
  if (result < 0)
    goto end;

  klog("stat(\"%s\", %p)", path, statbuf_p);

  stat_t statbuf;
  if ((result = do_stat(td, path, &statbuf)))
    goto end;
  result = copyout_s(statbuf, statbuf_p);
  if (result < 0)
    goto end;
  result = 0;

end:
  kfree(M_TEMP, path);
  return result;
}

static int sys_chdir(thread_t *td, syscall_args_t *args) {
  char *user_path = (char *)args->args[0];

  char *path = kmalloc(M_TEMP, PATH_MAX, 0);
  size_t len = 0;
  int result;

  result = copyinstr(user_path, path, PATH_MAX, &len);
  if (result < 0)
    goto end;

  klog("chdir(\"%s\")", path);
  result = -ENOTSUP;

end:
  kfree(M_TEMP, path);
  return result;
}

static int sys_getcwd(thread_t *td, syscall_args_t *args) {
  __unused char *user_buf = (char *)args->args[0];
  __unused size_t size = (size_t)args->args[1];
  return -ENOTSUP;
}

static int sys_mount(thread_t *td, syscall_args_t *args) {
  char *user_fsysname = (char *)args->args[0];
  char *user_pathname = (char *)args->args[1];

  int error = 0;
  char *fsysname = kmalloc(M_TEMP, PATH_MAX, 0);
  char *pathname = kmalloc(M_TEMP, PATH_MAX, 0);
  size_t n = 0;

  /* Copyout fsysname. */
  error = copyinstr(user_fsysname, fsysname, PATH_MAX, &n);
  if (error < 0)
    goto end;
  n = 0;
  /* Copyout pathname. */
  error = copyinstr(user_pathname, pathname, PATH_MAX, &n);
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

  klog("getdirentries(%d, %p, %u, %p)", fd, ubuf, count, base_p);

  uio_t uio = UIO_SINGLE_USER(UIO_READ, 0, ubuf, count);
  int res = do_getdirentries(td, fd, &uio, &base);
  if (res < 0)
    return res;
  if (base_p != NULL) {
    int error = copyout_s(base, base_p);
    if (error)
      return error;
  }
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
  if (status_p != NULL) {
    int error = copyout_s(status, status_p);
    if (error)
      return error;
  }
  return res;
}

static int sys_pipe(thread_t *td, syscall_args_t *args) {
  int *fds_p = (void *)args->args[0];
  int fds[2];

  klog("pipe(%x)", fds_p);

  int error = do_pipe(td, fds);
  if (error)
    return error;
  return copyout(fds, fds_p, 2 * sizeof(int));
}

static int sys_unlink(thread_t *td, syscall_args_t *args) {
  char *user_pathname = (char *)args->args[0];
  char *pathname = kmalloc(M_TEMP, PATH_MAX, 0);
  size_t n = 0;
  int result = 0;

  /* Copyout pathname. */
  result = copyinstr(user_pathname, pathname, PATH_MAX, &n);
  if (result < 0)
    goto end;

  klog("unlink(%s)", pathname);

  result = do_unlink(td, pathname);

end:
  kfree(M_TEMP, pathname);
  return result;
}

static int sys_mkdir(thread_t *td, syscall_args_t *args) {
  char *user_pathname = (char *)args->args[0];
  mode_t mode = (int)args->args[1];
  char *pathname = kmalloc(M_TEMP, PATH_MAX, 0);
  size_t n = 0;
  int result = 0;

  /* Copyout pathname. */
  result = copyinstr(user_pathname, pathname, PATH_MAX, &n);
  if (result < 0)
    goto end;

  klog("mkdir(%s, %d)", user_pathname, mode);

  result = do_mkdir(td, pathname, mode);

end:
  kfree(M_TEMP, pathname);
  return result;
}

static int sys_rmdir(thread_t *td, syscall_args_t *args) {
  char *user_pathname = (char *)args->args[0];
  char *pathname = kmalloc(M_TEMP, PATH_MAX, 0);
  size_t n = 0;
  int result = 0;

  /* Copyout pathname. */
  result = copyinstr(user_pathname, pathname, PATH_MAX, &n);
  if (result < 0)
    goto end;

  klog("rmdir(%s)", pathname);

  result = do_rmdir(td, pathname);

end:
  kfree(M_TEMP, pathname);
  return result;
}

static int sys_execve(thread_t *td, syscall_args_t *args) {
  char *user_path = (char *)args->args[0];
  char **user_argv = (char **)args->args[1];
  char **user_envp = (char **)args->args[2];

  /* do_execve handles copying data from user-space */
  return do_execve(user_path, user_argv, user_envp);
}

static int sys_access(thread_t *td, syscall_args_t *args) {
  char *user_pathname = (char *)args->args[0];
  mode_t mode = args->args[1];

  int result = 0;
  char *pathname = kmalloc(M_TEMP, PATH_MAX, 0);

  result = copyinstr(user_pathname, pathname, PATH_MAX, NULL);
  if (result < 0)
    goto end;

  klog("access(\"%s\", %d)", pathname, mode);

  result = do_access(td, pathname, mode);

end:
  kfree(M_TEMP, pathname);
  return result;
}

static int sys_clock_gettime(thread_t *td, syscall_args_t *args) {
  clockid_t clk = (clockid_t)args->args[0];
  timespec_t *uts = (timespec_t *)args->args[1];
  timespec_t kts;
  int result = do_clock_gettime(clk, &kts);
  if (result != 0)
    return result;
  return copyout_s(kts, uts);
}

static int sys_clock_nanosleep(thread_t *td, syscall_args_t *args) {
  clockid_t clk = (clockid_t)args->args[0];
  int flags = (int)args->args[1];
  timespec_t *urqtp = (timespec_t *)args->args[2];
  timespec_t krqtp;
  int result = copyin_s(urqtp, krqtp);
  if (result != 0)
    return result;
  return do_clock_nanosleep(clk, flags, &krqtp, NULL);
}

/* clang-format hates long arrays. */
sysent_t sysent[] = {
  [SYS_EXIT] = {sys_exit},
  [SYS_OPEN] = {sys_open},
  [SYS_CLOSE] = {sys_close},
  [SYS_READ] = {sys_read},
  [SYS_WRITE] = {sys_write},
  [SYS_LSEEK] = {sys_lseek},
  [SYS_UNLINK] = {sys_unlink},
  [SYS_GETPID] = {sys_getpid},
  [SYS_GETPPID] = {sys_getppid},
  [SYS_SETPGID] = {sys_setpgid},
  [SYS_GETPGID] = {sys_getpgid},
  [SYS_KILL] = {sys_kill},
  [SYS_FSTAT] = {sys_fstat},
  [SYS_STAT] = {sys_stat},
  [SYS_SBRK] = {sys_sbrk},
  [SYS_MMAP] = {sys_mmap},
  [SYS_FORK] = {sys_fork},
  [SYS_MOUNT] = {sys_mount},
  [SYS_GETDENTS] = {sys_getdirentries},
  [SYS_SIGACTION] = {sys_sigaction},
  [SYS_SIGRETURN] = {sys_sigreturn},
  [SYS_DUP] = {sys_dup},
  [SYS_DUP2] = {sys_dup2},
  [SYS_WAITPID] = {sys_waitpid},
  [SYS_MKDIR] = {sys_mkdir},
  [SYS_RMDIR] = {sys_rmdir},
  [SYS_ACCESS] = {sys_access},
  [SYS_PIPE] = {sys_pipe},
  [SYS_CLOCKGETTIME] = {sys_clock_gettime},
  [SYS_CLOCKNANOSLEEP] = {sys_clock_nanosleep},
  [SYS_EXECVE] = {sys_execve},
  [SYS_KILLPG] = {sys_killpg},
  [SYS_MUNMAP] = {sys_munmap},
  [SYS_MPROTECT] = {sys_mprotect},
  [SYS_CHDIR] = {sys_chdir},
  [SYS_GETCWD] = {sys_getcwd},
};
