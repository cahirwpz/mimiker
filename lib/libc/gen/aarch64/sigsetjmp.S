#include <aarch64/asm.h>
#include <machine/setjmp.h>

/* int sigsetjmp(jmp_buf buf, int savesigs) */
ENTRY(sigsetjmp)
        str     x1, [x0]
        cmp     x1, xzr
        bne     1f
        b       _setjmp
1:      b       setjmp
        ret
END(sigsetjmp)

/* void siglongjmp(sigjmp_buf env, int val) */
ENTRY(siglongjmp)
        ldr     x1, [x0]
        cmp     x1, xzr
        bne     1f
        b       _longjmp
1:      b       longjmp
        ret
END(siglongjmp)
