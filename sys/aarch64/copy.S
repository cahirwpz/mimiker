#include <sys/errno.h>
#include <aarch64/asm.h>
#include <aarch64/vm_param.h>

#include "assym.h"

.macro  set_td_onfault tmp td func
        mov     \tmp, \func
        mrs     \td, tpidr_el1
        ldr     \td, [\td, #PCPU_CURTHREAD]
        str     \tmp, [\td, #TD_ONFAULT]
.endm

.macro  unset_td_onfault td
        str     xzr, [\td, #TD_ONFAULT]
.endm

ASENTRY(copyerr)
        mov     x0, #EFAULT
        ret
ASEND(copyerr)

/*
 * int copycommon(void *from, void *dest, size_t len, thread_t *td)
 *
 * Copy specified amount of data and clear td_onfault.
 */
ASENTRY(copycommon)
        cbz     x2, 1f
        ldrb    w9, [x0]
        strb    w9, [x1]
        sub     x2, x2, #0x1
        add     x0, x0, #0x1
        add     x1, x1, #0x1
        b       copycommon
1:
        unset_td_onfault x3
        mov     x0, #0x0
        ret
ASEND(copycommon)

ASENTRY(copystr)
        b       .
ASEND(copystr)

ASENTRY(copyinstr)
        b       .
ASEND(copyinstr)

/*
 * int copyin(const void *udaddr, void *kaddr, size_t len)
 *
 * Copy specified amount of data from user space into the kernel.
 */
ASENTRY(copyin)
        cbz     x2, 1f

        adds    x9, x0, x2
        # fault on overflow
        b.cs    copy_fault

        adds    x11, x1, x2
        # fault on overflow
        b.cs    copyerr

        ldr     x10, =USER_SPACE_END 

        # user data must be in user space
        cmp     x9, x10
        b.hi    copyerr

        # kernel data must be in kernel space 
        cmp     x10, x1
        b.hi    copyerr

        set_td_onfault x12, x3, copyerr

        b       copycommon

1:      mov     x0, #0x0
        ret
ASEND(copyin)

/*
 * int copyout(const void *kaddr, void *udaddr, size_t len)
 *
 * Copy specified amount of data from kernel to the user space.
 */
ASENTRY(copyout)
        cbz     x2, 1f

        adds    x9, x0, x2
        # fault on overflow
        b.cs    copy_fault

        adds    x11, x1, x2
        # fault on overflow
        b.cs    copyerr

        ldr     x10, =USER_SPACE_END 

        # user data must be in user space
        cmp     x9, x10
        b.hi    copyerr

        # kernel data must be in kernel space 
        cmp     x10, x1
        b.hi    copyerr
        
        set_td_onfault x12, x3, copyerr

        # swap registers
        mov     x9, x1
        mov     x1, x0
        mov     x0, x9
        
        b       copycommon

1:      mov     x0, #0x0
        ret
ASEND(copyout)

/* bool try_load_word(unsigned *ptr, unsigned *val_p) */
ASENTRY(try_load_word)
        set_td_onfault x9 x10 fault
        ldr     w9, [x0]
        unset_td_onfault x10
        str     w9, [x1]
        mov     x0, #0x1
        ret
ASEND(try_load_word)

/* bool try_store_word(unsigned *ptr, unsigned val) */
ASENTRY(try_store_word)
        set_td_onfault x9 x10 fault
        str     w1, [x0]
        unset_td_onfault x10
        mov     x0, #0x1
        ret
ASEND(try_store_word)

ASENTRY(fault)
        mov     x0, #0x0
        ret
ASEND(fault)
