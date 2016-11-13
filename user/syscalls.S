#include <mips/asm.h>
#include <errno.h>

    .set noreorder
    .set noat

/* This implementation uses newlib-specific errno processing. */
#define DO_SYSCALL(num)  \
    addiu $sp, $sp, -16; \
    sw $ra, 12($sp);     \
    li $v0, num;         \
    syscall;             \
    bgez $v0, 1f;        \
    nop;                 \
    sw $v0, 8($sp);      \
    jal __errno;         \
    nop;                 \
    lw $v1, 8($sp);      \
    negu $v1, $v1;       \
    sw $v1, 0($v0);      \
    li $v0, -1;          \
1:  lw $ra, 12($sp);     \
    addiu $sp, $sp, 16;  \
    jr $ra;              \
    nop;

#define DO_NO_SYSCALL() \
    addiu $sp, $sp, -8; \
    sw $ra, 4($sp);     \
    jal __errno;        \
    nop;                \
    li $v1, ENOSYS;     \
    sw $v1, 0($v0);     \
    li $v0, -1;         \
    lw $ra, 4($sp);     \
    addiu $sp, $sp, 8;  \
    jr $ra;             \
    nop;

#define SYSCALL(name, num) \
    LEAF(name)      \
    DO_SYSCALL(num) \
    END(name)
#define NO_SYSCALL(name) \
    LEAF(name)      \
    DO_NO_SYSCALL() \
    END(name)       \

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
NO_SYSCALL(sbrk)
NO_SYSCALL(execve)
NO_SYSCALL(link)
NO_SYSCALL(symlink)
NO_SYSCALL(readlink)
NO_SYSCALL(wait)
NO_SYSCALL(isatty)
NO_SYSCALL(times)
NO_SYSCALL(gettimeofday)
