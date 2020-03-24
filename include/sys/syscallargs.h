/*
 * System call argument lists.
 *
 * DO NOT EDIT: this file is automatically generated.
 * created from; Mimiker system call name/number "master" file.
 */

#include <sys/time.h>
#include <sys/ucontext.h>
#include <sys/sigtypes.h>

typedef struct {
  int number;
} syscall_args_t;

typedef struct {
  int rval;
} exit_args_t;

typedef struct {
  int fd;
  void * buf;
  size_t nbyte;
} read_args_t;

typedef struct {
  int fd;
  const void * buf;
  size_t nbyte;
} write_args_t;

typedef struct {
  const char * path;
  int flags;
  mode_t mode;
} open_args_t;

typedef struct {
  int fd;
} close_args_t;

typedef struct {
  int fd;
  off_t offset;
  int whence;
} lseek_args_t;

typedef struct {
  const char * path;
} unlink_args_t;

typedef struct {
  pid_t pid;
  int sig;
} kill_args_t;

typedef struct {
  int fd;
  struct stat * sb;
} fstat_args_t;

typedef struct {
  intptr_t increment;
} sbrk_args_t;

typedef struct {
  void * addr;
  size_t len;
  int prot;
  int flags;
  int fd;
  off_t pos;
} mmap_args_t;

typedef struct {
  const char * type;
  const char * path;
} mount_args_t;

typedef struct {
  int fd;
  void * buf;
  size_t len;
} getdents_args_t;

typedef struct {
  int fd;
} dup_args_t;

typedef struct {
  int from;
  int to;
} dup2_args_t;

typedef struct {
  int signum;
  const struct sigaction * nsa;
  struct sigaction * osa;
} sigaction_args_t;

typedef struct {
  struct sigcontext * sigctx_p;
} sigreturn_args_t;

typedef struct {
  pid_t pid;
  int * status;
  int options;
  struct rusage * rusage;
} wait4_args_t;

typedef struct {
  int fd;
  const char * path;
  mode_t mode;
} mkdirat_args_t;

typedef struct {
  const char * path;
} rmdir_args_t;

typedef struct {
  const char * path;
  int amode;
} access_args_t;

typedef struct {
  int fd;
  const char * path;
  struct stat * sb;
  int flag;
} fstatat_args_t;

typedef struct {
  int * fdp;
  int flags;
} pipe2_args_t;

typedef struct {
  clockid_t clock_id;
  struct timespec * tsp;
} clock_gettime_args_t;

typedef struct {
  clockid_t clock_id;
  int flags;
  const struct timespec * rqtp;
  struct timespec * rmtp;
} clock_nanosleep_args_t;

typedef struct {
  const char * path;
  char *const * argp;
  char *const * envp;
} execve_args_t;

typedef struct {
  pid_t pid;
  pid_t pgid;
} setpgid_args_t;

typedef struct {
  pid_t pid;
} getpgid_args_t;

typedef struct {
  mode_t newmask;
} umask_args_t;

typedef struct {
  void * addr;
  size_t len;
} munmap_args_t;

typedef struct {
  void * addr;
  size_t len;
  int prot;
} mprotect_args_t;

typedef struct {
  const char * path;
} chdir_args_t;

typedef struct {
  char * buf;
  size_t len;
} getcwd_args_t;

typedef struct {
  const stack_t * ss;
  stack_t * old_ss;
} sigaltstack_args_t;

typedef struct {
  int how;
  const sigset_t * set;
  sigset_t * oset;
} sigprocmask_args_t;

typedef struct {
  const ucontext_t * ucp;
} setcontext_args_t;

typedef struct {
  int fd;
  u_long cmd;
  void * data;
} ioctl_args_t;

typedef struct {
  int fd;
  int cmd;
  void * arg;
} fcntl_args_t;

typedef struct {
  const char * path;
  off_t length;
} truncate_args_t;

typedef struct {
  int fd;
  off_t length;
} ftruncate_args_t;

typedef struct {
  int fd;
  const char * path;
  char * buf;
  size_t bufsiz;
} readlinkat_args_t;

typedef struct {
  int fd;
} fchdir_args_t;

typedef struct {
  const char * target;
  int newdirfd;
  const char * linkpath;
} symlinkat_args_t;
