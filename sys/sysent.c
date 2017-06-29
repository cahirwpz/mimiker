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
#include <stdc.h>

#define PATH_MAX 1024

/* Empty syscall handler, for unimplemented and deprecated syscall numbers. */
int sys_nosys(thread_t *td, syscall_args_t *args) {
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

  klog("read(%d, %p, %zu)", fd, ubuf, count);

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

  klog("write(%d, %p, %zu)", fd, ubuf, count);

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

  klog("getdirentries(%d, %p, %zu, %p)", fd, ubuf, count, base_p);

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



/* typedef struct temporary_args temporary_args_t; */


/* struct temporary_args { */
/*   char * prog_name; */
/*   size_t argc; */
/*   char** argv_off; */
/*   void* data; */
/* }; */

  


static int sys_execve(thread_t *td, syscall_args_t *args) {

  const char *user_path = (const char*)args->args[0];
  const char **user_argv = (const char**)args->args[1];

  /*Unused as environments are not yet implemented*/
  /*const char **user_envp = (const char**)args->args[2];*/
  
  if ( (user_path == NULL) || (user_argv == NULL))
    return -EFAULT; /*TODO: Perhaps user_argv can be NULL?*/

  char *kern_path, **kern_argv;
  char *data;

  size_t argc = 0;

  int result;

  size_t foo;
  
  /*Assuming PATH_MAX means max size of path including the filename.*/
  /*Assuming kmalloc always succeeds.*/
  kern_path = kmalloc(M_TEMP, PATH_MAX, 0);
  if ((result = copyinstr(user_path, kern_path, PATH_MAX, &foo)) < 0) {
    goto path_copy_failure;
  }
  
  size_t i = 0;
  size_t isize;

  while ( (i < ARG_MAX) && (user_argv[i] != NULL))
    i++;
  
  if ( /*(i == ARG_MAX) &&*/ (user_argv[i] != NULL)) {
    result = -E2BIG;
    goto free_mem_and_fail;
  }
  
  argc = i;

  /* kprintf("ARGC is: %d\n", argc); */
  
 /* /\*Assuming argc > 0*\/ */
 /*  if (!argc) { */
 /*    result = -EFAULT; */
 /*    goto free_mem_and_fail; */
 /*  } */

 
  kern_argv = kmalloc(M_TEMP, (argc + 1)*sizeof(char*), 0);
  kern_argv[argc] = NULL;
  data = kmalloc(M_TEMP, ARG_MAX*sizeof(char), 0);
  
  size_t data_off = 0;
  i  = 0;

 /* kprintf("First is %s\n", user_argv[0]); */
 /* kprintf("Second is %s\n", user_argv[1]); */
   
  while ( (data_off < ARG_MAX) && (i < argc)) {


    if ( (result = copyinstr(user_argv[i], data + data_off,
			     ARG_MAX - data_off, &isize)) < 0) {
      goto argv_copy_failure;
    }

    int argumentTooLong = user_argv[i][isize - 1] != '\0';
    if ( argumentTooLong) {
      result = -E2BIG;
      goto argv_copy_failure;
    }
    
    /* isize = strnlen(user_argv[i], ARG_MAX - 1); */
    /* isize++; */
    /* kprintf("Isize is %d\n", isize); */

    /* kern_argv[i] = kmalloc(M_TEMP, isize, 0);*/
    /* kprintf("Isize is %d\n", isize); */

    /* int argumentTooLong = user_argv[i][isize - 1] != '\0'; */
    /* int tooManyBytesInArgv = nbytes > ARG_MAX -  isize;  */
    /* if ( argumentTooLong || tooManyBytesInArgv) { */
    /*   result = -E2BIG; */
    /*   goto argv_copy_failure; */
    /* } */

    /* if ( nbytes > ARG_MAX -  isize) { */
    /*   result = -E2BIG; */
    /*   goto argv_copy_failure; */
    /* } */
    
    kern_argv[i] = data + data_off;
   
    /* if ( (result = copyinstr(user_argv[i], */
    /* 			     data + nbytes, isize, 0)) < 0) { */
    /*   goto argv_copy_failure; */
    /* } */

    data_off += isize;
    i++;
  }
 
  /*WARNING: exec_args_t.argv type is probably incorrect. It is const char**, 
   should be char *const */
  
   const exec_args_t exec_args = {.prog_name = kern_path,
   				  .argv = (const char **)kern_argv,
   				  .argc = argc};
   result = do_exec(&exec_args);

   
  /* temporary_args_t exec_args = {.prog_name = kern_path, */
  /* 				.argv_off = kern_argv, */
  /* 				.argc = argc, */
  /* 				.data = data}; */

  
 argv_copy_failure:
 /*   kfree(M_TEMP, data); */
 /*   kfree(M_TEMP, kern_argv); */
   

 path_copy_failure:
 free_mem_and_fail:
 /*  kfree(M_TEMP, kern_path); */

 return result;
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
    [SYS_WAITPID] = {sys_waitpid},
    [SYS_MKDIR] = {sys_mkdir},
    [SYS_RMDIR] = {sys_rmdir},
    [SYS_EXECVE] = {sys_execve},
    [SYS_ACCESS] = {sys_access},
};
