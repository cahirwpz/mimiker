#ifndef _SYS_SYSCALL_H_
#define _SYS_SYSCALL_H_

#define SYS_exit 1
#define SYS_open 2
#define SYS_close 3
#define SYS_read 4
#define SYS_write 5
#define SYS_lseek 6
#define SYS_unlink 7
#define SYS_getpid 8
#define SYS_kill 9
#define SYS_fstat 10
#define SYS_sbrk 11
#define SYS_mmap 12
#define SYS_fork 13
#define SYS_mount 14
#define SYS_getdents 15
#define SYS_dup 16
#define SYS_dup2 17
#define SYS_sigaction 18
#define SYS_sigreturn 19
#define SYS_waitpid 20
#define SYS_mkdir 21
#define SYS_rmdir 22
#define SYS_access 23
#define SYS_stat 24
#define SYS_pipe 25
#define SYS_clockgettime 26
#define SYS_clocknanosleep 27
#define SYS_execve 28
#define SYS_getppid 29
#define SYS_setpgid 30
#define SYS_getpgid 31
#define SYS_killpg 32
#define SYS_munmap 33
#define SYS_mprotect 34
#define SYS_chdir 35
#define SYS_getcwd 36

#define SYS_MAXSYSCALL 37

#endif /* !_SYS_SYSCALL_H_ */
