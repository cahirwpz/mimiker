#include <aarch64/asm.h>

#include "aarch64/assym.h"

/* int sigsetjmp(jmp_buf buf, int savesigs) */
ENTRY(sigsetjmp)
        cmp     x1, xzr
        bne     1f
        str     x1, [x0, UC_FLAGS]
        b       _setjmp

1:      mov     x1, _UC_SIGMASK
        str     x1, [x0, UC_FLAGS]
        b       setjmp
END(sigsetjmp)

/* void siglongjmp(sigjmp_buf env, int val) */
ENTRY(siglongjmp)
        ldr     x2, [x0, UC_FLAGS]
        and     x2, x2, _UC_SIGMASK
        cmp     x2, _UC_SIGMASK
        beq     longjmp
        b       _longjmp
END(siglongjmp)
