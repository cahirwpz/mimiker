#include <mips/asm.h>

    .set noreorder
    .global _start

# This is the starting procedude for user ELF images, used as their
# entry point. It takes care of preparing argc and argc for main().
# For details about environment in which this procedure starts, refer
# to prepare_program_stack in exec.c.

NESTED(_start, CALLFRAME_SIZE, $ra)
    # Adhere to calling convention wrt return address so that GDB can properly recognize stack frames.
    # However, store zero as RA, to mark stack bottom.
    addiu $sp,$sp,-8
    li $ra,0x0
    sw $ra,4($sp)
    sw $sp,0($sp)

    # Grab argc from below stack
    lw $a0,8($sp)
    # Prepare argv pointing at argument vector below stack
    addiu $a1,$sp,12

    # Jump to main
    jal main
    nop

    # Main returned. Normally we would syscall exit, but it's not implemented yet.
1:
    b 1b
    nop

END(_start)
