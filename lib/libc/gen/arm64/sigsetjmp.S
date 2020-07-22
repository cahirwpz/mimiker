#include <arm64/asm.h>
#include <machine/setjmp.h>

ENTRY(sigsetjmp)
        ret
END(sigsetjmp)

ENTRY(siglongjmp)
        ret
END(siglongjmp)
