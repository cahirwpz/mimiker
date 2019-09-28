#define KL_LOG KL_SYSCALL
#include <sys/klog.h>
#include <sys/sysent.h>
#include <sys/mimiker.h>
#include <sys/errno.h>
#include <sys/thread.h>
#include <sys/mman.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/sbrk.h>
#include <sys/signal.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/exec.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/pipe.h>
#include <sys/malloc.h>
#include <sys/syslimits.h>

/* Empty syscall handler, for unimplemented and deprecated syscall numbers. */
int sys_syscall(thread_t *td, void *v) {
  sys_syscall_args_t *args = v;
  klog("unimplemented system call %ld", args->number);
  return -ENOSYS;
};

static int sys_sbrk(thread_t *td, void *v) {
  sys_sbrk_args_t *args = v;

  klog("sbrk(%d)", args->increment);

  return sbrk_resize(td->td_proc, args->increment);
}

static int sys_exit(thread_t *td, void *v) {
  sys_exit_args_t *args = v;

  klog("exit(%d)", args->rval);
  proc_exit(MAKE_STATUS_EXIT(args->rval));
  __unreachable();
}

static int sys_fork(thread_t *td, void *v) {
  klog("fork()");
  return do_fork();
}

static int sys_getpid(thread_t *td, void *v) {
  klog("getpid()");
  return td->td_proc->p_pid;
}

static int sys_getppid(thread_t *td, void *v) {
  proc_t *p = td->td_proc;
  klog("getppid()");
  return p->p_parent ? p->p_parent->p_pid : 0;
}

/* Moves the process to the process group with ID specified by pgid.
 * If pid is zero, then the PID of the calling process is used.
 * If pgid is zero, then the process is moved to process group
 * with ID equal to the PID of the process. */
static int sys_setpgid(thread_t *td, void *v) {
  sys_setpgid_args_t *args = v;

  pid_t pid = args->pid;
  pgid_t pgid = args->pgid;

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
static int sys_getpgid(thread_t *td, void *v) {
  sys_getpgid_args_t *args = v;

  pid_t pid = args->pid;
  proc_t *p = td->td_proc;

  if (pid < 0)
    return -EINVAL;

  if (pid == 0)
    pid = p->p_pid;

  return proc_getpgid(pid);
}

static int sys_kill(thread_t *td, void *v) {
  sys_kill_args_t *args = v;

  klog("kill(%lu, %d)", args->pid, args->signum);
  return proc_sendsig(args->pid, args->signum);
}

/* Sends signal sig to process group with ID equal to pgid. */
static int sys_killpg(thread_t *td, void *v) {
  sys_killpg_args_t *args = v;

  pgid_t pgid = args->pgid;
  signo_t sig = args->signum;
  klog("killpg(%lu, %d)", pgid, sig);

  if (pgid == 1 || pgid < 0)
    return -EINVAL;

  /* pgid == 0 => sends signal to our process group
   * pgid  > 1 => sends signal to the process group with ID equal pgid */
  return proc_sendsig(-pgid, sig);
}

static int sys_sigaction(thread_t *td, void *v) {
  sys_sigaction_args_t *args = v;

  int signo = args->signum;
  const void *p_newact = args->nsa;
  void *p_oldact = args->osa;

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

static int sys_sigreturn(thread_t *td, void *v) {
  klog("sigreturn()");
  return do_sigreturn();
}

static int sys_mmap(thread_t *td, void *v) {
  sys_mmap_args_t *args = v;

  vaddr_t va = (vaddr_t)args->addr;
  size_t length = args->len;
  vm_prot_t prot = args->prot;
  int flags = args->flags;

  klog("mmap(%p, %u, %d, %d)", args->addr, length, prot, flags);

  int error = do_mmap(&va, length, prot, flags);
  if (error < 0)
    return error;
  return va;
}

static int sys_munmap(thread_t *td, void *v) {
  sys_munmap_args_t *args = v;

  klog("munmap(%p, %u)", args->addr, args->len);
  return do_munmap((vaddr_t)args->addr, args->len);
}

static int sys_mprotect(thread_t *td, void *v) {
  sys_mprotect_args_t *args = v;
  klog("mprotect(%p, %u, %u)", args->addr, args->len, args->prot);
  return -ENOTSUP;
}

static int sys_open(thread_t *td, void *v) {
  sys_open_args_t *args = v;

  const char *u_path = args->path;
  int flags = args->flags;
  mode_t mode = args->mode;

  int result = 0;
  char *path = kmalloc(M_TEMP, PATH_MAX, 0);
  size_t n = 0;

  /* Copyout path. */
  result = copyinstr(u_path, path, PATH_MAX, &n);
  if (result < 0)
    goto end;

  klog("open(\"%s\", %d, %d)", path, flags, mode);

  int fd;
  result = do_open(td, path, flags, mode, &fd);
  if (result == 0)
    result = fd;

end:
  kfree(M_TEMP, path);
  return result;
}

static int sys_close(thread_t *td, void *v) {
  sys_close_args_t *args = v;

  klog("close(%d)", args->fd);

  return do_close(td, args->fd);
}

static int sys_read(thread_t *td, void *v) {
  sys_read_args_t *args = v;

  int fd = args->fd;
  void *u_buf = args->buf;
  size_t nbyte = args->nbyte;

  klog("read(%d, %p, %u)", fd, u_buf, nbyte);

  uio_t uio;
  uio = UIO_SINGLE_USER(UIO_READ, 0, u_buf, nbyte);
  int error = do_read(td, fd, &uio);
  if (error)
    return error;
  return nbyte - uio.uio_resid;
}

static int sys_write(thread_t *td, void *v) {
  sys_write_args_t *args = v;

  int fd = args->fd;
  const char *u_buf = args->buf;
  size_t nbyte = args->nbyte;

  klog("write(%d, %p, %u)", fd, u_buf, nbyte);

  uio_t uio;
  uio = UIO_SINGLE_USER(UIO_WRITE, 0, u_buf, nbyte);
  int error = do_write(td, fd, &uio);
  if (error)
    return error;
  return nbyte - uio.uio_resid;
}

static int sys_lseek(thread_t *td, void *v) {
  sys_lseek_args_t *args = v;

  klog("lseek(%d, %ld, %d)", args->fd, args->offset, args->whence);
  return do_lseek(td, args->fd, args->offset, args->whence);
}

static int sys_fstat(thread_t *td, void *v) {
  sys_fstat_args_t *args = v;

  int fd = args->fd;
  stat_t *u_sb = args->sb;

  klog("fstat(%d, %p)", fd, u_sb);

  stat_t sb;
  int error = do_fstat(td, fd, &sb);
  if (error)
    return error;
  error = copyout_s(sb, u_sb);
  if (error < 0)
    return error;
  return 0;
}

static int sys_stat(thread_t *td, void *v) {
  sys_stat_args_t *args = v;

  const char *u_path = args->path;
  stat_t *u_sb = args->sb;

  char *path = kmalloc(M_TEMP, PATH_MAX, 0);
  size_t path_len = 0;
  int result;

  result = copyinstr(u_path, path, PATH_MAX, &path_len);
  if (result < 0)
    goto end;

  klog("stat(\"%s\", %p)", path, u_sb);

  stat_t sb;
  if ((result = do_stat(td, path, &sb)))
    goto end;
  result = copyout_s(sb, u_sb);
  if (result < 0)
    goto end;
  result = 0;

end:
  kfree(M_TEMP, path);
  return result;
}

static int sys_chdir(thread_t *td, void *v) {
  sys_chdir_args_t *args = v;

  const char *u_path = args->path;
  char *path = kmalloc(M_TEMP, PATH_MAX, 0);
  size_t len = 0;
  int result;

  result = copyinstr(u_path, path, PATH_MAX, &len);
  if (result < 0)
    goto end;

  klog("chdir(\"%s\")", path);
  result = -ENOTSUP;

end:
  kfree(M_TEMP, path);
  return result;
}

static int sys_getcwd(thread_t *td, void *v) {
  sys_getcwd_args_t *args = v;

  __unused char *u_buf = args->buf;
  __unused size_t len = args->len;
  return -ENOTSUP;
}

static int sys_mount(thread_t *td, void *v) {
  sys_mount_args_t *args = v;

  const char *u_type = args->type;
  const char *u_path = args->path;

  int error = 0;
  char *type = kmalloc(M_TEMP, PATH_MAX, 0);
  char *path = kmalloc(M_TEMP, PATH_MAX, 0);
  size_t n = 0;

  /* Copyout fsysname. */
  error = copyinstr(u_type, type, PATH_MAX, &n);
  if (error < 0)
    goto end;
  n = 0;
  /* Copyout pathname. */
  error = copyinstr(u_path, path, PATH_MAX, &n);
  if (error < 0)
    goto end;

  klog("mount(\"%s\", \"%s\")", path, type);

  error = do_mount(td, type, path);
end:
  kfree(M_TEMP, type);
  kfree(M_TEMP, path);
  return error;
}

static int sys_getdirentries(thread_t *td, void *v) {
  sys_getdirentries_args_t *args = v;

  int fd = args->fd;
  void *u_buf = args->buf;
  size_t len = args->len;
  off_t *u_basep = (off_t *)args->basep;
  off_t base;

  klog("getdirentries(%d, %p, %u, %p)", fd, u_buf, len, u_basep);

  uio_t uio = UIO_SINGLE_USER(UIO_READ, 0, u_buf, len);
  int res = do_getdirentries(td, fd, &uio, &base);
  if (res < 0)
    return res;
  if (u_basep != NULL) {
    int error = copyout_s(base, u_basep);
    if (error)
      return error;
  }
  return res;
}

static int sys_dup(thread_t *td, void *v) {
  sys_dup_args_t *args = v;

  klog("dup(%d)", args->fd);
  return do_dup(td, args->fd);
}

static int sys_dup2(thread_t *td, void *v) {
  sys_dup2_args_t *args = v;

  klog("dup2(%d, %d)", args->from, args->to);
  return do_dup2(td, args->from, args->to);
}

static int sys_waitpid(thread_t *td, void *v) {
  sys_waitpid_args_t *args = v;

  pid_t pid = args->pid;
  int *u_wstatus = args->wstatus;
  int options = args->options;
  int status = 0;

  klog("waitpid(%d, %x, %d)", pid, u_wstatus, options);

  int res = do_waitpid(pid, &status, options);
  if (res < 0)
    return res;
  if (u_wstatus != NULL) {
    int error = copyout_s(status, u_wstatus);
    if (error)
      return error;
  }
  return res;
}

static int sys_pipe(thread_t *td, void *v) {
  sys_pipe_args_t *args = v;

  int *u_fdp = args->fdp;
  int fds[2];

  klog("pipe(%x)", u_fdp);

  int error = do_pipe(td, fds);
  if (error)
    return error;
  return copyout(fds, u_fdp, 2 * sizeof(int));
}

static int sys_unlink(thread_t *td, void *v) {
  sys_unlink_args_t *args = v;

  const char *u_path = args->path;

  char *path = kmalloc(M_TEMP, PATH_MAX, 0);
  size_t n = 0;
  int result = 0;

  /* Copyout pathname. */
  result = copyinstr(u_path, path, PATH_MAX, &n);
  if (result < 0)
    goto end;

  klog("unlink(%s)", path);

  result = do_unlink(td, path);

end:
  kfree(M_TEMP, path);
  return result;
}

static int sys_mkdir(thread_t *td, void *v) {
  sys_mkdir_args_t *args = v;

  const char *u_path = args->path;
  mode_t mode = args->mode;

  char *path = kmalloc(M_TEMP, PATH_MAX, 0);
  size_t n = 0;
  int result = 0;

  /* Copyout pathname. */
  result = copyinstr(u_path, path, PATH_MAX, &n);
  if (result < 0)
    goto end;

  klog("mkdir(%s, %d)", u_path, mode);

  result = do_mkdir(td, path, mode);

end:
  kfree(M_TEMP, path);
  return result;
}

static int sys_rmdir(thread_t *td, void *v) {
  sys_rmdir_args_t *args = v;

  const char *u_path = args->path;

  char *path = kmalloc(M_TEMP, PATH_MAX, 0);
  size_t n = 0;
  int result = 0;

  /* Copyout pathname. */
  result = copyinstr(u_path, path, PATH_MAX, &n);
  if (result < 0)
    goto end;

  klog("rmdir(%s)", path);

  result = do_rmdir(td, path);

end:
  kfree(M_TEMP, path);
  return result;
}

static int sys_execve(thread_t *td, void *v) {
  sys_execve_args_t *args = v;

  const char *u_path = args->path;
  char *const *u_argp = args->argp;
  char *const *u_envp = args->envp;

  /* do_execve handles copying data from user-space */
  return do_execve(u_path, u_argp, u_envp);
}

static int sys_access(thread_t *td, void *v) {
  sys_access_args_t *args = v;

  const char *u_path = args->path;
  mode_t mode = args->amode;

  int result = 0;
  char *path = kmalloc(M_TEMP, PATH_MAX, 0);

  result = copyinstr(u_path, path, PATH_MAX, NULL);
  if (result < 0)
    goto end;

  klog("access(\"%s\", %d)", path, mode);

  result = do_access(td, path, mode);

end:
  kfree(M_TEMP, path);
  return result;
}

static int sys_clock_gettime(thread_t *td, void *v) {
  sys_clock_gettime_args_t *args = v;

  clockid_t clock_id = args->clock_id;
  timespec_t *u_ts = args->tsp;
  timespec_t ts;
  int result = do_clock_gettime(clock_id, &ts);
  if (result != 0)
    return result;
  return copyout_s(ts, u_ts);
}

static int sys_clock_nanosleep(thread_t *td, void *v) {
  sys_clock_nanosleep_args_t *args = v;

  clockid_t clock_id = args->clock_id;
  int flags = args->flags;
  const timespec_t *u_rqtp = args->rqtp;
  timespec_t rqtp;
  int result = copyin_s(u_rqtp, rqtp);
  if (result != 0)
    return result;
  return do_clock_nanosleep(clock_id, flags, &rqtp, NULL);
}

#include "sysent.h"
