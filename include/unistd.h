#ifndef _UNISTD_H_
#define _UNISTD_H_

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/unistd.h>

/* IEEE Std 1003.1-90 */
#define STDIN_FILENO 0  /* standard input file descriptor */
#define STDOUT_FILENO 1 /* standard output file descriptor */
#define STDERR_FILENO 2 /* standard error file descriptor */

__BEGIN_DECLS
__noreturn void _exit(int);
int access(const char *, int);
unsigned int alarm(unsigned int);
int chdir(const char *);
int chown(const char *, uid_t, gid_t);
int close(int);
size_t confstr(int, char *, size_t);
int dup(int);
int dup2(int, int);
int execl(const char *, const char *, ...);
int execle(const char *, const char *, ...);
int execlp(const char *, const char *, ...);
int execv(const char *, char *const *);
int execve(const char *, char *const *, char *const *);
int execvp(const char *, char *const *);
pid_t fork(void);
long fpathconf(int, int);
char *getcwd(char *, size_t);
gid_t getegid(void);
uid_t geteuid(void);
gid_t getgid(void);
int getgroups(int, gid_t[]);
char *getlogin(void);
int getlogin_r(char *, size_t);
pid_t getpgrp(void);
pid_t getpid(void);
pid_t getppid(void);
uid_t getuid(void);
int isatty(int);
int link(const char *, const char *);
long pathconf(const char *, int);
int pause(void);
int pipe(int *);
ssize_t read(int, void *, size_t);
int rmdir(const char *);
int setgid(gid_t);
int setpgid(pid_t, pid_t);
pid_t setsid(void);
int setuid(uid_t);
unsigned int sleep(unsigned int);
long sysconf(int);
pid_t tcgetpgrp(int);
int tcsetpgrp(int, pid_t);
const char *ttyname(int);
int unlink(const char *);
ssize_t write(int, const void *, size_t);

/* IEEE Std 1003.2-92, adopted in X/Open Portability Guide Issue 4 and later */
int getopt(int, char *const[], const char *);

/* getopt(3) external variables */
extern char *optarg;
extern int opterr;
extern int optind;
extern int optopt;

/* The Open Group Base Specifications, Issue 5; IEEE Std 1003.1-2001 (POSIX) */
ssize_t readlink(const char *__restrict, char *__restrict, size_t);
ssize_t readlinkat(int, const char *__restrict, char *__restrict, size_t);

/* The Open Group Base Specifications, Issue 6; IEEE Std 1003.1-2001 (POSIX) */
int setegid(gid_t);
int seteuid(uid_t);
off_t lseek(int, off_t, int);
int truncate(const char *, off_t);

/*
 * IEEE Std 1003.1b-93, also found in X/Open Portability Guide >= Issue 4
 * Version 2
 */
int ftruncate(int, off_t);

/*
 * X/Open Portability Guide, all issues
 */
int nice(int);

/*
 * X/Open Portability Guide >= Issue 4
 */
const char *crypt(const char *, const char *);
int encrypt(char *, int);
char *getpass(const char *);
pid_t getsid(pid_t);

/*
 * X/Open Portability Guide >= Issue 4 Version 2
 */

#define F_ULOCK 0
#define F_LOCK 1
#define F_TLOCK 2
#define F_TEST 3

int brk(void *);
int fchdir(int);
int fchown(int, uid_t, gid_t);
int getdtablesize(void);
long gethostid(void);
int gethostname(char *, size_t);
__pure int getpagesize(void);
pid_t getpgid(pid_t);
int lchown(const char *, uid_t, gid_t);
int lockf(int, int, off_t);
void *sbrk(intptr_t);
int setpgrp(pid_t, pid_t);
int setregid(gid_t, gid_t);
int setreuid(uid_t, uid_t);
void swab(const void *__restrict, void *__restrict, ssize_t);
int symlink(const char *, const char *);
void sync(void);
useconds_t ualarm(useconds_t, useconds_t);
int usleep(useconds_t);
pid_t vfork(void) __returns_twice;

/*
 * IEEE Std 1003.1b-93, adopted in X/Open CAE Specification Issue 5 Version 2
 */
int fdatasync(int);
int fsync(int);

/*
 * X/Open CAE Specification Issue 5 Version 2
 */
ssize_t pread(int, void *, size_t, off_t);
ssize_t pwrite(int, const void *, size_t, off_t);

/*
 * X/Open Extended API set 2 (a.k.a. C063)
 */
int linkat(int, const char *, int, const char *, int);
int symlinkat(const char *, int, const char *);
int unlinkat(int, const char *, int);
int faccessat(int, const char *, int, int);

/*
 * Implementation-defined extensions
 */
int issetugid(void);
int pipe2(int *, int);
void *setmode(const char *mode_str);
mode_t getmode(const void *set, mode_t mode);
void strmode(mode_t, char *);

__END_DECLS

#endif /* !_UNISTD_H_ */
