#include <mips/asm.h>

        .set noreorder

#
# void syscall(unsigned num)
#
LEAF(syscall)
        move    $v0, $a0
        syscall
        j       $ra
        nop
END(syscall)

# vim: sw=8 ts=8 et
