#include <mips/asm.h>
#include <mips/regdef.h>
#include <mips/pcpu.h>

#include "assym.h"

        # Don't allow the assembler to reorder instructions.
        .set	noreorder

        .global try_load_word
        .global try_store_word  
        .local  error

/* bool try_load_word(unsigned *ptr, unsigned *val_p) */
LEAF(try_load_word)
        LOAD_PCPU(v1)
        PTR_L   v1, PCPU_CURTHREAD(v1)
        LA      v0, fault
        PTR_S   v0, TD_ONFAULT(v1)
        lw      v0, 0(a0)
        PTR_S   zero, TD_ONFAULT(v1)
        sw      v0, 0(a1)
        j       ra
        li      v0, 1
END(try_load_word)

/* bool try_store_word(unsigned *ptr, unsigned val) */
LEAF(try_store_word)
        LA      v0, fault
        LOAD_PCPU(v1)
        PTR_L   v1, PCPU_CURTHREAD(v1)
        PTR_S   v0, TD_ONFAULT(v1)
        sw      a1, 0(a0)
        PTR_S   zero, TD_ONFAULT(v1)
        j       ra
        li      v0, 1
END(try_store_word)

LEAF(fault)
        j       ra
        move    v0, zero
END(fault)

# vim: sw=8 ts=8 et
