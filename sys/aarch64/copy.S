#include <aarch64/asm.h>

#include "assym.h"

.macro  set_td_onfault tmp td
        mov     \tmp, fault
        mrs     \td, tpidr_el1
        ldr     \td, [\td, #PCPU_CURTHREAD]
        str     \tmp, [\td, #TD_ONFAULT]
.endm

.macro  unset_td_onfault td
        str     xzr, [\td, #TD_ONFAULT]
.endm

ASENTRY(copystr)
        b       .
ASEND(copystr)

ASENTRY(copyinstr)
        b       .
ASEND(copyinstr)

ASENTRY(copyin)
        b       .
ASEND(copyin)

ASENTRY(copyout)
        b       .
ASEND(copyout)

/* bool try_load_word(unsigned *ptr, unsigned *val_p) */
ASENTRY(try_load_word)
        set_td_onfault x9 x10
        ldr     w9, [x0]
        unset_td_onfault x10
        str     w9, [x1]
        mov     x0, #0x1
        ret
ASEND(try_load_word)

/* bool try_store_word(unsigned *ptr, unsigned val) */
ASENTRY(try_store_word)
        set_td_onfault x9 x10
        str     w1, [x0]
        unset_td_onfault x10
        mov     x0, #0x1
        ret
ASEND(try_store_word)

ASENTRY(fault)
        mov     x0, #0x0
        ret
ASEND(fault)
