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

#include "sysent.h"

/* Empty syscall handler, for unimplemented and deprecated syscall numbers. */
int sys_syscall(proc_t *p, syscall_args_t *args, register_t *res) {
  klog("unimplemented system call %ld", args->number);
  return ENOSYS;
};

static int sys_sbrk(proc_t *p, sbrk_args_t *args, register_t *res) {
  klog("sbrk(%d)", args->increment);

  return sbrk_resize(p, args->increment, (vaddr_t *)res);
}

/* https://pubs.opengroup.org/onlinepubs/9699919799/functions/exit.html */
static int sys_exit(proc_t *p, exit_args_t *args, register_t *res) {
  klog("exit(%d)", args->rval);
  proc_exit(MAKE_STATUS_EXIT(args->rval));
  __unreachable();
}

/* https://pubs.opengroup.org/onlinepubs/9699919799/functions/fork.html */
static int sys_fork(proc_t *p, void *args, register_t *res) {
  int error;
  pid_t pid;

  klog("fork()");

  if ((error = do_fork(&pid)))
    return error;

  *res = pid;
  return 0;
}

/* https://pubs.opengroup.org/onlinepubs/9699919799/functions/getpid.html */
static int sys_getpid(proc_t *p, void *args, register_t *res) {
  klog("getpid()");
  *res = p->p_pid;
  return 0;
}

/* https://pubs.opengroup.org/onlinepubs/9699919799/functions/getppid.html */
static int sys_getppid(proc_t *p, void *args, register_t *res) {
  klog("getppid()");
  *res = p->p_parent ? p->p_parent->p_pid : 0;
  return 0;
}

/* Moves the process to the process group with ID specified by pgid.
 * If pid is zero, then the PID of the calling process is used.
 * If pgid is zero, then the process is moved to process group
 * with ID equal to the PID of the process.
 *
 * https://pubs.opengroup.org/onlinepubs/9699919799/functions/setpgid.html */
static int sys_setpgid(proc_t *p, setpgid_args_t *args, register_t *res) {
  pid_t pid = args->pid;
  pgid_t pgid = args->pgid;

  klog("setpgid(%d, %d)", pid, pgid);

  if (pgid < 0)
    return EINVAL;

  if (pid == 0)
    pid = p->p_pid;
  if (pgid == 0)
    pgid = pid;

  /* TODO Allow process to call setpgid on its children.
   * TODO Make setpgid accepts pgid equal to ID of any existing process group */
  if (pid != p->p_pid || pgid != p->p_pid)
    return ENOTSUP;

  return pgrp_enter(p, pgid);
}

/* Gets process group ID of the process with ID specified by pid.
 * If pid equals zero then process ID of the calling process is used.
 *
 * https://pubs.opengroup.org/onlinepubs/9699919799/functions/getpgid.html */
static int sys_getpgid(proc_t *p, getpgid_args_t *args, register_t *res) {
  pid_t pid = args->pid;

  if (pid < 0)
    return EINVAL;

  if (pid == 0)
    pid = p->p_pid;

  return proc_getpgid(pid, res);
}

/* https://pubs.opengroup.org/onlinepubs/9699919799/functions/kill.html */
static int sys_kill(proc_t *p, kill_args_t *args, register_t *res) {
  klog("kill(%lu, %d)", args->pid, args->sig);
  return proc_sendsig(args->pid, args->sig);
}

/* Sends signal sig to process group with ID equal to pgid.
 *
 * https://pubs.opengroup.org/onlinepubs/9699919799/functions/killpg.html */
static int sys_killpg(proc_t *p, killpg_args_t *args, register_t *res) {
  pgid_t pgid = args->pgrp;
  int sig = args->sig;
  klog("killpg(%lu, %d)", pgid, sig);

  if (pgid == 1 || pgid < 0)
    return EINVAL;

  /* pgid == 0 => sends signal to our process group
   * pgid  > 1 => sends signal to the process group with ID equal pgid */
  return proc_sendsig(-pgid, sig);
}

/* https://pubs.opengroup.org/onlinepubs/9699919799/functions/sigaction.html */
static int sys_sigaction(proc_t *p, sigaction_args_t *args, register_t *res) {
  int signo = args->signum;
  const void *p_newact = args->nsa;
  void *p_oldact = args->osa;

  klog("sigaction(%d, %p, %p)", signo, p_newact, p_oldact);

  sigaction_t newact;
  sigaction_t oldact;
  int error;

  if ((error = copyin_s(p_newact, newact)))
    return error;

  if ((error = do_sigaction(signo, &newact, &oldact)))
    return error;

  if (p_oldact != NULL)
    if ((error = copyout_s(oldact, p_oldact)))
      return error;

  return error;
}

/* TODO: handle sigcontext argument */
static int sys_sigreturn(proc_t *p, sigreturn_args_t *args, register_t *res) {
  klog("sigreturn()");
  return do_sigreturn();
}

static int sys_mmap(proc_t *p, mmap_args_t *args, register_t *res) {
  vaddr_t va = (vaddr_t)args->addr;
  size_t length = args->len;
  vm_prot_t prot = args->prot;
  int flags = args->flags;

  klog("mmap(%p, %u, %d, %d)", args->addr, length, prot, flags);

  int error;
  if ((error = do_mmap(&va, length, prot, flags)))
    return error;

  *res = va;
  return 0;
}

static int sys_munmap(proc_t *p, munmap_args_t *args, register_t *res) {
  klog("munmap(%p, %u)", args->addr, args->len);
  return do_munmap((vaddr_t)args->addr, args->len);
}

/* TODO: implement it !!! */
static int sys_mprotect(proc_t *p, mprotect_args_t *args, register_t *res) {
  klog("mprotect(%p, %u, %u)", args->addr, args->len, args->prot);
  return ENOTSUP;
}

static int sys_open(proc_t *p, open_args_t *args, register_t *res) {
  const char *u_path = args->path;
  int flags = args->flags;
  mode_t mode = args->mode;

  char *path = kmalloc(M_TEMP, PATH_MAX, 0);
  size_t n = 0;
  int fd, error;

  /* Copyout path. */
  if ((error = copyinstr(u_path, path, PATH_MAX, &n)))
    goto end;

  klog("open(\"%s\", %d, %d)", path, flags, mode);

  if ((error = do_open(p, path, flags, mode, &fd)))
    goto end;

  *res = fd;

end:
  kfree(M_TEMP, path);
  return error;
}

static int sys_close(proc_t *p, close_args_t *args, register_t *res) {
  klog("close(%d)", args->fd);

  return do_close(p, args->fd);
}

static int sys_read(proc_t *p, read_args_t *args, register_t *res) {
  int fd = args->fd;
  void *u_buf = args->buf;
  size_t nbyte = args->nbyte;
  int error;

  klog("read(%d, %p, %u)", fd, u_buf, nbyte);

  uio_t uio = UIO_SINGLE_USER(UIO_READ, 0, u_buf, nbyte);
  if ((error = do_read(p, fd, &uio)))
    return error;

  *res = nbyte - uio.uio_resid;
  return 0;
}

static int sys_write(proc_t *p, write_args_t *args, register_t *res) {
  int fd = args->fd;
  const char *u_buf = args->buf;
  size_t nbyte = args->nbyte;
  int error;

  klog("write(%d, %p, %u)", fd, u_buf, nbyte);

  uio_t uio = UIO_SINGLE_USER(UIO_WRITE, 0, u_buf, nbyte);
  if ((error = do_write(p, fd, &uio)))
    return error;

  *res = nbyte - uio.uio_resid;
  return 0;
}

static int sys_lseek(proc_t *p, lseek_args_t *args, register_t *res) {
  off_t newoff;
  int error;

  klog("lseek(%d, %ld, %d)", args->fd, args->offset, args->whence);
  if ((error = do_lseek(p, args->fd, args->offset, args->whence, &newoff)))
    return error;

  *res = newoff;
  return 0;
}

static int sys_fstat(proc_t *p, fstat_args_t *args, register_t *res) {
  int fd = args->fd;
  stat_t *u_sb = args->sb;
  stat_t sb;
  int error;

  klog("fstat(%d, %p)", fd, u_sb);

  if ((error = do_fstat(p, fd, &sb)))
    return error;
  if ((error = copyout_s(sb, u_sb)))
    return error;

  return 0;
}

static int sys_stat(proc_t *p, stat_args_t *args, register_t *res) {
  const char *u_path = args->path;
  stat_t *u_sb = args->sb;

  char *path = kmalloc(M_TEMP, PATH_MAX, 0);
  size_t path_len = 0;
  stat_t sb;
  int error;

  if ((error = copyinstr(u_path, path, PATH_MAX, &path_len)))
    goto end;

  klog("stat(\"%s\", %p)", path, u_sb);

  if ((error = do_stat(p, path, &sb)))
    goto end;
  if ((error = copyout_s(sb, u_sb)))
    goto end;

end:
  kfree(M_TEMP, path);
  return error;
}

/* TODO: not implemented */
static int sys_chdir(proc_t *p, chdir_args_t *args, register_t *res) {
  const char *u_path = args->path;
  char *path = kmalloc(M_TEMP, PATH_MAX, 0);
  size_t len = 0;
  int error;

  if ((error = copyinstr(u_path, path, PATH_MAX, &len)))
    goto end;

  klog("chdir(\"%s\")", path);
  error = ENOTSUP;

end:
  kfree(M_TEMP, path);
  return error;
}

static int sys_getcwd(proc_t *p, getcwd_args_t *args, register_t *res) {
  __unused char *u_buf = args->buf;
  __unused size_t len = args->len;

  return ENOTSUP;
}

static int sys_mount(proc_t *p, mount_args_t *args, register_t *res) {
  const char *u_type = args->type;
  const char *u_path = args->path;

  char *type = kmalloc(M_TEMP, PATH_MAX, 0);
  char *path = kmalloc(M_TEMP, PATH_MAX, 0);
  size_t n = 0;
  int error;

  /* Copyout fsysname. */
  if ((error = copyinstr(u_type, type, PATH_MAX, &n)))
    goto end;
  n = 0;
  /* Copyout pathname. */
  if ((error = copyinstr(u_path, path, PATH_MAX, &n)))
    goto end;

  klog("mount(\"%s\", \"%s\")", path, type);

  error = do_mount(type, path);
end:
  kfree(M_TEMP, type);
  kfree(M_TEMP, path);
  return error;
}

static int sys_getdirentries(proc_t *p, getdirentries_args_t *args,
                             register_t *res) {
  int fd = args->fd;
  void *u_buf = args->buf;
  size_t len = args->len;
  off_t *u_basep = (off_t *)args->basep;
  off_t base;
  int error;

  klog("getdirentries(%d, %p, %u, %p)", fd, u_buf, len, u_basep);

  uio_t uio = UIO_SINGLE_USER(UIO_READ, 0, u_buf, len);
  if ((error = do_getdirentries(p, fd, &uio, &base)))
    return error;

  if (u_basep != NULL)
    if ((error = copyout_s(base, u_basep)))
      return error;

  return 0;
}

static int sys_dup(proc_t *p, dup_args_t *args, register_t *res) {
  klog("dup(%d)", args->fd);
  return do_dup(p, args->fd, res);
}

static int sys_dup2(proc_t *p, dup2_args_t *args, register_t *res) {
  klog("dup2(%d, %d)", args->from, args->to);

  *res = args->to;

  return do_dup2(p, args->from, args->to);
}

static int sys_waitpid(proc_t *p, waitpid_args_t *args, register_t *res) {
  pid_t pid = args->pid;
  int *u_wstatus = args->wstatus;
  int options = args->options;
  int status = 0;
  int error;

  klog("waitpid(%d, %x, %d)", pid, u_wstatus, options);

  if ((error = do_waitpid(pid, &status, options, res)))
    return error;

  if (u_wstatus != NULL)
    if ((error = copyout_s(status, u_wstatus)))
      return error;

  return 0;
}

static int sys_pipe(proc_t *p, pipe_args_t *args, register_t *res) {
  int *u_fdp = args->fdp;
  int fds[2];
  int error;

  klog("pipe(%x)", u_fdp);

  if ((error = do_pipe(p, fds)))
    return error;

  return copyout(fds, u_fdp, 2 * sizeof(int));
}

static int sys_unlink(proc_t *p, unlink_args_t *args, register_t *res) {
  const char *u_path = args->path;

  char *path = kmalloc(M_TEMP, PATH_MAX, 0);
  size_t n = 0;
  int error;

  /* Copyout pathname. */
  if ((error = copyinstr(u_path, path, PATH_MAX, &n)))
    goto end;

  klog("unlink(%s)", path);

  error = do_unlink(p, path);

end:
  kfree(M_TEMP, path);
  return error;
}

static int sys_mkdir(proc_t *p, mkdir_args_t *args, register_t *res) {
  const char *u_path = args->path;
  mode_t mode = args->mode;

  char *path = kmalloc(M_TEMP, PATH_MAX, 0);
  size_t n = 0;
  int error;

  if ((error = copyinstr(u_path, path, PATH_MAX, &n)))
    goto end;

  klog("mkdir(%s, %d)", u_path, mode);

  error = do_mkdir(p, path, mode);

end:
  kfree(M_TEMP, path);
  return error;
}

static int sys_rmdir(proc_t *p, rmdir_args_t *args, register_t *res) {
  const char *u_path = args->path;

  char *path = kmalloc(M_TEMP, PATH_MAX, 0);
  size_t n = 0;
  int error;

  if ((error = copyinstr(u_path, path, PATH_MAX, &n)))
    goto end;

  klog("rmdir(%s)", path);

  error = do_rmdir(p, path);

end:
  kfree(M_TEMP, path);
  return error;
}

static int sys_execve(proc_t *p, execve_args_t *args, register_t *res) {
  const char *u_path = args->path;
  char *const *u_argp = args->argp;
  char *const *u_envp = args->envp;

  /* do_execve handles copying data from user-space */
  return do_execve(u_path, u_argp, u_envp);
}

static int sys_access(proc_t *p, access_args_t *args, register_t *res) {
  const char *u_path = args->path;
  mode_t mode = args->amode;

  char *path = kmalloc(M_TEMP, PATH_MAX, 0);
  int error;

  if ((error = copyinstr(u_path, path, PATH_MAX, NULL)))
    goto end;

  klog("access(\"%s\", %d)", path, mode);

  error = do_access(p, path, mode);

end:
  kfree(M_TEMP, path);
  return error;
}

static int sys_clock_gettime(proc_t *p, clock_gettime_args_t *args,
                             register_t *res) {
  clockid_t clock_id = args->clock_id;
  timespec_t *u_ts = args->tsp;
  timespec_t ts;
  int error;

  if ((error = do_clock_gettime(clock_id, &ts)))
    return error;

  return copyout_s(ts, u_ts);
}

static int sys_clock_nanosleep(proc_t *p, clock_nanosleep_args_t *args,
                               register_t *res) {
  clockid_t clock_id = args->clock_id;
  int flags = args->flags;
  const timespec_t *u_rqtp = args->rqtp;
  timespec_t rqtp;
  int error;

  if ((error = copyin_s(u_rqtp, rqtp)))
    return error;

  return do_clock_nanosleep(clock_id, flags, &rqtp, NULL);
}
