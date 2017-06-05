#include <mips/asm.h>
#include <mips/regdef.h>
#include <errno.h>
#include <sysent.h>

        .set noreorder

AENT(execve)
AENT(link)
AENT(symlink)
AENT(readlink)
AENT(isatty)
AENT(times)

SLEAF(no_syscall)
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
SEND(set_errno)

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

SYSCALL(_exit, SYS_EXIT)
SYSCALL(open, SYS_OPEN)
SYSCALL(close, SYS_CLOSE)
SYSCALL(read, SYS_READ)
SYSCALL(write, SYS_WRITE)
SYSCALL(lseek, SYS_LSEEK)
SYSCALL(unlink, SYS_UNLINK)
SYSCALL(getpid, SYS_GETPID)
SYSCALL(kill, SYS_KILL)
SYSCALL(fstat, SYS_FSTAT)
SYSCALL(sbrk, SYS_SBRK)
SYSCALL(fork, SYS_FORK)
SYSCALL(mmap, SYS_MMAP)
SYSCALL(dup, SYS_DUP)
SYSCALL(dup2, SYS_DUP2)	
SYSCALL(getdirentries, SYS_GETDENTS)
SYSCALL(sigaction, SYS_SIGACTION)
SYSCALL(sigreturn, SYS_SIGRETURN)
SYSCALL(waitpid, SYS_WAITPID)
SYSCALL(clock_gettime, SYS_CLOCKGETTIME)
SYSCALL(nanosleep, SYS_NANOSLEEP)
SYSCALL(gettimeofday, SYS_GETTIMEOFDAY)

# vim: sw=8 ts=8 et
