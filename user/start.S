#include <mips/asm.h>
    
    .set noreorder
    .global _start

LEAF(_start)
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
    
    jal main
    nop
    
    # Main returned. Normally we would syscall exit.
1:
    b 1b
    nop
    
END(_start)
