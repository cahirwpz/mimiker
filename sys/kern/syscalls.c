#define KL_LOG KL_SYSCALL
#include <sys/klog.h>
#include <sys/sysent.h>
#include <sys/mimiker.h>
#include <sys/errno.h>
#include <sys/mman.h>
#include <sys/vfs.h>
#include <sys/uio.h>
#include <sys/file.h>
#include <sys/sbrk.h>
#include <sys/signal.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/exec.h>
#include <sys/time.h>
#include <sys/ioccom.h>
#include <sys/pipe.h>
#include <sys/malloc.h>
#include <sys/libkern.h>
#include <sys/syslimits.h>
#include <sys/context.h>
#include <sys/thread.h>

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
  proc_lock(p);
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
  pgid_t pgid;

  if (pid < 0)
    return EINVAL;

  if (pid == 0)
    pid = p->p_pid;

  int error = proc_getpgid(pid, &pgid);
  *res = pgid;
  return error;
}

/* https://pubs.opengroup.org/onlinepubs/9699919799/functions/kill.html */
static int sys_kill(proc_t *p, kill_args_t *args, register_t *res) {
  klog("kill(%lu, %d)", args->pid, args->sig);
  return proc_sendsig(args->pid, args->sig);
}

/* Set and get the file mode creation mask.
 *
 * https://pubs.opengroup.org/onlinepubs/9699919799/functions/umask.html */
static int sys_umask(proc_t *p, umask_args_t *args, register_t *res) {
  mode_t newmask = args->newmask;
  klog("umask(%x)", args->newmask);
  return do_umask(p, newmask, res);
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

static int sys_openat(proc_t *p, openat_args_t *args, register_t *res) {
  int fdat = args->fd;
  const char *u_path = args->path;
  int flags = args->flags;
  mode_t mode = args->mode;

  char *path = kmalloc(M_TEMP, PATH_MAX, 0);
  size_t n = 0;
  int fd, error;

  /* Copyout path. */
  if ((error = copyinstr(u_path, path, PATH_MAX, &n)))
    goto end;

  klog("openat(%d, \"%s\", %d, %d)", fdat, path, flags, mode);

  if ((error = do_openat(p, fdat, path, flags, mode, &fd)))
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
  return copyout_s(sb, u_sb);
}

static int sys_chdir(proc_t *p, chdir_args_t *args, register_t *res) {
  const char *u_path = args->path;
  char *path = kmalloc(M_TEMP, PATH_MAX, 0);
  size_t len = 0;
  int error = 0;

  if ((error = copyinstr(u_path, path, PATH_MAX, &len)))
    goto end;

  error = do_chdir(p, path);

end:
  kfree(M_TEMP, path);
  return error;
}

static int sys_fchdir(proc_t *p, fchdir_args_t *args, register_t *res) {
  klog("fchdir(%d)", args->fd);
  return do_fchdir(p, args->fd);
}

static int sys_getcwd(proc_t *p, getcwd_args_t *args, register_t *res) {
  char *u_buf = args->buf;
  size_t len = args->len;
  int error;

  if (len == 0)
    return EINVAL;

  char *path = kmalloc(M_TEMP, PATH_MAX, 0);
  size_t last = PATH_MAX;

  /* We're going to construct the path backwards! */
  if ((error = do_getcwd(p, path, &last)))
    goto end;

  size_t used = PATH_MAX - last;
  if (used > len) {
    error = ERANGE;
    goto end;
  }

  error = copyout(&path[last], u_buf, used);

end:
  kfree(M_TEMP, path);
  return error;
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

static int sys_getdents(proc_t *p, getdents_args_t *args, register_t *res) {
  int fd = args->fd;
  void *u_buf = args->buf;
  size_t len = args->len;
  int error;

  klog("getdents(%d, %p, %u)", fd, u_buf, len);

  uio_t uio = UIO_SINGLE_USER(UIO_READ, 0, u_buf, len);
  if ((error = do_getdents(p, fd, &uio)))
    return error;

  *res = len - uio.uio_resid;
  return 0;
}

static int sys_dup(proc_t *p, dup_args_t *args, register_t *res) {
  int error, fd;
  klog("dup(%d)", args->fd);
  error = do_dup(p, args->fd, &fd);
  *res = fd;
  return error;
}

static int sys_dup2(proc_t *p, dup2_args_t *args, register_t *res) {
  klog("dup2(%d, %d)", args->from, args->to);

  *res = args->to;

  return do_dup2(p, args->from, args->to);
}

static int sys_fcntl(proc_t *p, fcntl_args_t *args, register_t *res) {
  int error, value;
  klog("fcntl(%d, %d, %ld)", args->fd, args->cmd, (long)args->arg);
  error = do_fcntl(p, args->fd, args->cmd, (long)args->arg, &value);
  *res = value;
  return error;
}

static int sys_wait4(proc_t *p, wait4_args_t *args, register_t *res) {
  pid_t pid = args->pid;
  int *u_status = args->status;
  int options = args->options;
  struct rusage *u_rusage = args->rusage;
  int status = 0;
  int error;

  klog("wait4(%d, %x, %d, %p)", pid, u_status, options, u_rusage);

  if (u_rusage)
    klog("sys_wait4: acquiring rusage not implemented!");

  pid_t cld;
  if ((error = do_waitpid(pid, &status, options, &cld)))
    return error;

  if (u_status != NULL)
    if ((error = copyout_s(status, u_status)))
      return error;

  *res = cld;
  return 0;
}

static int sys_pipe2(proc_t *p, pipe2_args_t *args, register_t *res) {
  int *u_fdp = args->fdp;
  int flags = args->flags;
  int fds[2];
  int error;

  klog("pipe2(%x, %d)", u_fdp, flags);

  if (flags)
    klog("sys_pipe2: non-zero flags not handled!");

  if ((error = do_pipe(p, fds)))
    return error;

  return copyout(fds, u_fdp, 2 * sizeof(int));
}

static int sys_unlinkat(proc_t *p, unlinkat_args_t *args, register_t *res) {
  int fd = args->fd;
  const char *u_path = args->path;
  int flag = args->flag;

  char *path = kmalloc(M_TEMP, PATH_MAX, 0);
  size_t n = 0;
  int error;

  /* Copyout pathname. */
  if ((error = copyinstr(u_path, path, PATH_MAX, &n)))
    goto end;

  klog("unlinkat(%d, \"%s\", %d)", fd, path, flag);

  error = do_unlinkat(p, fd, path, flag);

end:
  kfree(M_TEMP, path);
  return error;
}

static int sys_mkdirat(proc_t *p, mkdirat_args_t *args, register_t *res) {
  int fd = args->fd;
  const char *u_path = args->path;
  mode_t mode = args->mode;

  char *path = kmalloc(M_TEMP, PATH_MAX, 0);
  size_t n = 0;
  int error;

  if ((error = copyinstr(u_path, path, PATH_MAX, &n)))
    goto end;

  klog("mkdirat(%d, %s, %d)", fd, u_path, mode);

  error = do_mkdirat(p, fd, path, mode);

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

static int sys_faccessat(proc_t *p, faccessat_args_t *args, register_t *res) {
  int fd = args->fd;
  const char *u_path = args->path;
  mode_t mode = args->mode;
  int flags = args->flags;

  char *path = kmalloc(M_TEMP, PATH_MAX, 0);
  int error;

  if ((error = copyinstr(u_path, path, PATH_MAX, NULL)))
    goto end;

  klog("faccessat(%d, \"%s\", %d, %d)", fd, path, mode, flags);

  error = do_faccessat(p, fd, path, mode, flags);

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

static int sys_sigaltstack(proc_t *p, sigaltstack_args_t *args,
                           register_t *res) {
  const stack_t *ss = args->ss;
  stack_t *old_ss = args->old_ss;
  int error;

  klog("sigaltstack(%p, %p)", ss, old_ss);

  /* TODO: Setting alternate signal stack is not implemented yet. */
  assert(ss == NULL);

  if (old_ss != NULL) {
    /* TODO: Currently we don't support alternate signal stack in the kernel,
       so this syscall always returns SS_DISABLE. */
    stack_t result = {.ss_sp = 0, .ss_size = 0, .ss_flags = SS_DISABLE};
    error = copyout_s(result, old_ss);
    if (error)
      return error;
  }

  return 0;
}

static int sys_sigprocmask(proc_t *p, sigprocmask_args_t *args,
                           register_t *res) {
  int how = args->how;
  const sigset_t *set = args->set;
  sigset_t *user_oset = args->oset;
  sigset_t oset;
  int error;

  klog("sigprocmask(%d, %p, %p)", how, set, user_oset);

  proc_lock(p);
  error = do_sigprocmask(how, set, &oset);
  proc_unlock(p);

  if (error)
    return error;

  if (user_oset)
    error = copyout_s(oset, user_oset);
  return error;
}

static int sys_setcontext(proc_t *p, setcontext_args_t *args, register_t *res) {
  const ucontext_t *ucp = args->ucp;
  klog("setcontext(%p)", ucp);

  ucontext_t uc;
  copyin_s(ucp, uc);

  return do_setcontext(p->p_thread, &uc);
}

static int sys_ioctl(proc_t *p, ioctl_args_t *args, register_t *res) {
  int fd = args->fd;
  u_long cmd = args->cmd;
  void *u_data = args->data;
  int error;

  klog("ioctl(%d, %lx, %p)", fd, cmd, u_data);

  /* For cmd format look at <sys/ioccom.h> header file. */
  unsigned len = IOCPARM_LEN(cmd);
  unsigned dir = cmd & IOC_DIRMASK;
  void *data = NULL;

  /* Allocate a kernel buffer if ioctl needs/returns information. */
  if (dir)
    data = kmalloc(M_TEMP, len, 0);

  if ((dir & IOC_IN) && (error = copyin(u_data, data, len)))
    goto fail;

  if ((error = do_ioctl(p, fd, cmd, data)))
    goto fail;

  if ((dir & IOC_OUT) && (error = copyout(data, u_data, len)))
    goto fail;

fail:
  if (dir)
    kfree(M_TEMP, data);

  return error;
}

/* TODO: not implemented */
static int sys_getuid(proc_t *p, void *args, register_t *res) {
  klog("getuid()");
  *res = 0;
  return 0;
}

/* TODO: not implemented */
static int sys_geteuid(proc_t *p, void *args, register_t *res) {
  klog("geteuid()");
  *res = 0;
  return 0;
}

/* TODO: not implemented */
static int sys_getgid(proc_t *p, void *args, register_t *res) {
  klog("getgid()");
  *res = 0;
  return 0;
}

/* TODO: not implemented */
static int sys_getegid(proc_t *p, void *args, register_t *res) {
  klog("getegid()");
  *res = 0;
  return 0;
}

/* TODO: not implemented */
static int sys_issetugid(proc_t *p, void *args, register_t *res) {
  klog("issetugid()");
  *res = 0;
  return 0;
}

static int sys_truncate(proc_t *p, truncate_args_t *args, register_t *res) {
  const char *u_path = args->path;
  off_t length = args->length;

  char *path = kmalloc(M_TEMP, PATH_MAX, 0);
  int error;

  if ((error = copyinstr(u_path, path, PATH_MAX, NULL)))
    goto end;

  klog("truncate(\"%s\", %d)", path, length);

  error = do_truncate(p, path, length);

end:
  kfree(M_TEMP, path);
  return error;
}

static int sys_ftruncate(proc_t *p, ftruncate_args_t *args, register_t *res) {
  int fd = args->fd;
  off_t length = args->length;
  klog("ftruncate(%d, %d)", fd, length);
  return do_ftruncate(p, fd, length);
}

static int sys_fstatat(proc_t *p, fstatat_args_t *args, register_t *res) {
  int fd = args->fd;
  const char *u_path = args->path;
  stat_t *u_sb = args->sb;
  int flag = args->flag;
  stat_t sb;
  int error;

  char *path = kmalloc(M_TEMP, PATH_MAX, 0);

  if ((error = copyinstr(u_path, path, PATH_MAX, NULL)))
    goto end;

  klog("fstatat(%d, \"%s\", %p, %d)", fd, path, u_sb, flag);

  if (!(error = do_fstatat(p, fd, path, &sb, flag)))
    error = copyout_s(sb, u_sb);

end:
  kfree(M_TEMP, path);
  return error;
}

static int sys_readlinkat(proc_t *p, readlinkat_args_t *args, register_t *res) {
  int fd = args->fd;
  const char *u_path = args->path;
  char *u_buf = args->buf;
  size_t bufsiz = args->bufsiz;
  int error;

  char *path = kmalloc(M_TEMP, PATH_MAX, 0);

  if ((error = copyinstr(u_path, path, PATH_MAX, NULL)))
    goto end;

  klog("readlinkat(%d, \"%s\", %p, %u)", fd, path, u_buf, bufsiz);

  uio_t uio = UIO_SINGLE_USER(UIO_READ, 0, u_buf, bufsiz);
  if (!(error = do_readlinkat(p, fd, path, &uio)))
    *res = bufsiz - uio.uio_resid;

end:
  kfree(M_TEMP, path);
  return error;
}

static int sys_symlinkat(proc_t *p, symlinkat_args_t *args, register_t *res) {
  const char *u_target = args->target;
  int newdirfd = args->newdirfd;
  const char *u_linkpath = args->linkpath;
  int error;

  char *target = kmalloc(M_TEMP, PATH_MAX, 0);
  char *linkpath = kmalloc(M_TEMP, PATH_MAX, 0);

  if ((error = copyinstr(u_target, target, PATH_MAX, NULL)))
    goto end;
  if ((error = copyinstr(u_linkpath, linkpath, PATH_MAX, NULL)))
    goto end;

  klog("symlinkat(\"%s\", %d, \"%s\")", target, newdirfd, linkpath);

  error = do_symlinkat(p, target, newdirfd, linkpath);

end:
  kfree(M_TEMP, target);
  kfree(M_TEMP, linkpath);
  return error;
}

static int sys_linkat(proc_t *p, linkat_args_t *args, register_t *res) {
  int fd1 = args->fd1;
  const char *u_name1 = args->name1;
  int fd2 = args->fd2;
  const char *u_name2 = args->name2;
  int flags = args->flags;
  int error;

  char *name1 = kmalloc(M_TEMP, PATH_MAX, 0);
  char *name2 = kmalloc(M_TEMP, PATH_MAX, 0);

  if ((error = copyinstr(u_name1, name1, PATH_MAX, NULL)))
    goto end;
  if ((error = copyinstr(u_name2, name2, PATH_MAX, NULL)))
    goto end;

  klog("linkat(%d, \"%s\", %d, \"%s\", %d)", fd1, u_name1, fd2, u_name2, flags);

  error = do_linkat(p, fd1, name1, fd2, name2, flags);

end:
  kfree(M_TEMP, name1);
  kfree(M_TEMP, name2);
  return error;
}

static int sys_fchmod(proc_t *p, fchmod_args_t *args, register_t *res) {
  int fd = args->fd;
  mode_t mode = args->mode;

  klog("fchmod(%d, %d)", fd, mode);

  return do_fchmod(p, fd, mode);
}

static int sys_fchmodat(proc_t *p, fchmodat_args_t *args, register_t *res) {
  int fd = args->fd;
  const char *u_path = args->path;
  mode_t mode = args->mode;
  int flag = args->flag;
  int error;

  char *path = kmalloc(M_TEMP, PATH_MAX, 0);

  if ((error = copyinstr(u_path, path, PATH_MAX, NULL)))
    goto end;

  klog("fchmodat(%d, \"%s\", %d, %d)", fd, path, mode, flag);

  error = do_fchmodat(p, fd, path, mode, flag);

end:
  kfree(M_TEMP, path);
  return error;
}

static int sys_sched_yield(proc_t *p, void *args, register_t *res) {
  klog("sched_yield()");
  thread_yield();
  return 0;
}
