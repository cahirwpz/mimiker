#include <sys/errno.h>
#include <aarch64/asm.h>
#include <aarch64/vm_param.h>

#include "assym.h"

.macro  set_onfault tmp td func
        # load current thread
        mrs     \td, tpidr_el1
        ldr     \td, [\td, #PCPU_CURTHREAD]
        # set td_onfault
        adr     \tmp, \func
        str     \tmp, [\td, #TD_ONFAULT]
.endm

.macro  clr_onfault td
        # load current thread
        mrs     \td, tpidr_el1
        ldr     \td, [\td, #PCPU_CURTHREAD]
        # clr td_onfault
        str     xzr, [\td, #TD_ONFAULT]
.endm

/*
 * int copyinstr(const void *udaddr, void *kaddr, size_t len, size_t *done)
 *
 * Copy a null terminated string from the user address space into
 * the kernel address space.
 */
ENTRY(copyinstr)
        # len > 0
        cbnz    x2, 1f
        mov     x0, xzr
        ret

        # (uintptr_t)us < (uintptr_t)(us + len)
1:      add     x4, x0, x2
        cmp     x0, x4
        b.hi    copyerr

        # (uintptr_t)(us + len) <= USER_SPACE_END
        ldr     x5, =USER_SPACE_END 
        cmp     x4, x5
        b.hi    copyerr

        set_onfault x4, x5, copyerr
        bl      copystr
        clr_onfault x4
        ret
END(copyinstr)

/*
 * int copyin(const void *udaddr, void *kaddr, size_t len)
 *
 * Copy specified amount of data from user space into the kernel.
 */
ENTRY(copyin)
        # len > 0
        cbz     x2, 1f

        # (uintptr_t)us < (uintptr_t)(us + len)
        add     x3, x0, x2
        cmp     x0, x3
        b.hi    copyerr

        # (uintptr_t)(us + len) <= USER_SPACE_END
        ldr     x4, =USER_SPACE_END 
        cmp     x3, x4
        b.hi    copyerr

        set_onfault x3, x4, copyerr
        bl      bcopy
        clr_onfault x3

1:      mov     x0, xzr
        ret
END(copyin)

/*
 * int copyout(const void *kaddr, void *udaddr, size_t len)
 *
 * Copy specified amount of data from kernel to the user space.
 */
ENTRY(copyout)
        # len > 0
        cbz     x2, 1f

        # (uintptr_t)us < (uintptr_t)(us + len)
        add     x3, x1, x2
        cmp     x1, x3
        b.hi    copyerr

        # (uintptr_t)(us + len) <= USER_SPACE_END
        ldr     x4, =USER_SPACE_END 
        cmp     x3, x4
        b.hi    copyerr

        set_onfault x3, x4, copyerr
        bl      bcopy
        clr_onfault x3

1:      mov     x0, xzr
        ret
END(copyout)

SENTRY(copyerr)
        # since we return from bcopy/copystr context we need to pop retaddr
        add     sp, sp, #8
        mov     x0, #EFAULT
        ret
END(copyerr)

/*
 * bool try_load_word(unsigned *ptr, unsigned *val_p)
 */
ENTRY(try_load_word)
        set_onfault x2 x3 fault
        ldr     w4, [x0]
        clr_onfault x2
        str     w4, [x1]
        mov     x0, #1
        ret
END(try_load_word)

/*
 * bool try_store_word(unsigned *ptr, unsigned val)
 */
ENTRY(try_store_word)
        set_onfault x2 x3 fault
        str     w1, [x0]
        clr_onfault x2
        mov     x0, #1
        ret
END(try_store_word)

SENTRY(fault)
        mov     x0, xzr
        ret
END(fault)

# vim: sw=8 ts=8 et
