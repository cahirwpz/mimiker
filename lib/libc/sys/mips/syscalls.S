#include <mips/asm.h>
#include <mips/regdef.h>
#include <sys/errno.h>
#include <sys/syscall.h>

        .set noreorder


AENT(link)
AENT(symlink)
AENT(readlink)
AENT(isatty)
AENT(times)
AENT(lstat)
AENT(fcntl)
AENT(getrusage)
AENT(rename)
AENT(ioctl)
AENT(ftruncate)
AENT(getpgrp)
AENT(umask)
AENT(getrlimit)
AENT(setegid)
AENT(seteuid)
AENT(setgid)
AENT(setuid)
AENT(sigprocmask)
AENT(sigsuspend)
AENT(fchmod)
AENT(fchdir)
AENT(fchown)
AENT(fstatvfs)
AENT(statvfs)
AENT(setitimer)
AENT(setrlimit)
AENT(getdents)
AENT(socketpair)
AENT(sigaltstack)
AENT(setcontext)

LEAF(no_syscall)
        b       set_errno
        li      v0, -ENOSYS
END(no_syscall)

SNESTED(set_errno, CALLFRAME_SIZ, ra)
        PTR_SUBU sp, sp, CALLFRAME_SIZ
        .mask	0x80000000, (CALLFRAME_RA - CALLFRAME_SIZ)
        sw      ra, CALLFRAME_RA(sp)
        sw      v0, 16(sp)
        jal     __errno
        nop
        lw      v1, 16(sp)
        negu    v1, v1
        sw      v1, 0(v0)
        li      v0, -1
        lw      ra, CALLFRAME_RA(sp)
        jr      ra
        PTR_ADDU sp, sp, CALLFRAME_SIZ
END(set_errno)

/* This implementation uses newlib-specific errno processing. */
#define DO_SYSCALL(num)         \
        li      v0, num;        \
        syscall;                \
        bltz    v0, set_errno;  \
        nop;                    \
        jr      ra;             \
        nop;

#define SYSCALL(name, num)      \
        LEAF(name)              \
        DO_SYSCALL(num)         \
        END(name)

SYSCALL(_exit, SYS_exit)
SYSCALL(open, SYS_open)
SYSCALL(close, SYS_close)
SYSCALL(read, SYS_read)
SYSCALL(write, SYS_write)
SYSCALL(lseek, SYS_lseek)
SYSCALL(unlink, SYS_unlink)
SYSCALL(getpid, SYS_getpid)
SYSCALL(kill, SYS_kill)
SYSCALL(fstat, SYS_fstat)
SYSCALL(sbrk, SYS_sbrk)
SYSCALL(fork, SYS_fork)
SYSCALL(mmap, SYS_mmap)
SYSCALL(dup, SYS_dup)
SYSCALL(dup2, SYS_dup2)
SYSCALL(getdirentries, SYS_getdirentries)
SYSCALL(sigaction, SYS_sigaction)
SYSCALL(sigreturn, SYS_sigreturn)
SYSCALL(waitpid, SYS_waitpid)
SYSCALL(pipe, SYS_pipe)
SYSCALL(mkdir, SYS_mkdir)
SYSCALL(rmdir, SYS_rmdir)
SYSCALL(access, SYS_access)
SYSCALL(execve, SYS_execve)
SYSCALL(stat, SYS_stat)
SYSCALL(clock_gettime, SYS_clock_gettime)
SYSCALL(clock_nanosleep, SYS_clock_nanosleep)
SYSCALL(getppid, SYS_getppid)
SYSCALL(getpgid, SYS_getpgid)
SYSCALL(setpgid, SYS_setpgid)
SYSCALL(killpg, SYS_killpg)
SYSCALL(munmap, SYS_munmap)
SYSCALL(mprotect, SYS_mprotect)
SYSCALL(chdir, SYS_chdir)
SYSCALL(getcwd, SYS_getcwd)

# vim: sw=8 ts=8 et
