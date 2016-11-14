#include <mips/asm.h>
#include <errno.h>

AENT(sbrk)
AENT(execve)
AENT(link)
AENT(symlink)
AENT(readlink)
AENT(wait)
AENT(isatty)
AENT(times)
AENT(gettimeofday)

SLEAF(no_syscall)
        b       set_errno
        li      $v0, -ENOSYS
END(no_syscall)

SNESTED(set_errno, 24, $ra)
        addiu   $sp, $sp, -24
        sw      $ra, 20($sp)
        sw      $v0, 16($sp)
        jal     __errno
        nop
        lw      $v1, 16($sp)
        negu    $v1, $v1
        sw      $v1, 0($v0)
        li      $v0, -1
        lw      $ra, 20($sp)
        jr      $ra
        addiu   $sp, $sp, 24
SEND(set_errno)

/* This implementation uses newlib-specific errno processing. */
#define DO_SYSCALL(num)         \
        li      $v0, num;       \
        syscall;                \
        bltz    $v0, set_errno; \
        nop;                    \
        jr $ra;                 \
        nop;

#define SYSCALL(name, num)      \
        LEAF(name)              \
        DO_SYSCALL(num)         \
        END(name)

SYSCALL(_exit,1)
SYSCALL(open,2)
SYSCALL(close,3)
SYSCALL(read,4)
SYSCALL(write,5)
SYSCALL(lseek,6)
SYSCALL(unlink,7)
SYSCALL(getpid,8)
SYSCALL(kill,9)
SYSCALL(fstat,10)

# vim: sw=8 ts=8 et
