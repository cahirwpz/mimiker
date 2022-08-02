#include <aarch64/abi.h>
#include <aarch64/asm.h>

#define INIT_STACK_SIZE 512

/* Initial boot environment is described in detail 
 * at Documentation/arm64/booting.rst from Linux source tree. */

#define IMAGE_OFFSET 0 /* Image offset from start of 2 MiB page */
#define IMAGE_SIZE __kernel_size
#define IMAGE_FLAGS 2 /* Kernel is little endian, page size is 4KiB */

        .section .boot,"ax",@progbits

_ENTRY(_start)
        /* Based on locore.S from FreeBSD. */
        b       1f
        .long   0
        .quad   IMAGE_OFFSET
        .quad   IMAGE_SIZE
        .quad   IMAGE_FLAGS
        .quad   0
        .quad   0
        .quad   0
        .long   0x644d5241 /* Magic "ARM\x64" */
        .long   0
1:
        /* Get CPU number. */
        mrs     x3, mpidr_el1
        and     x3, x3, #3
        cmp     x3, #0
        bne     .

        /* Setup initial stack. */
        adr     x3, _init_stack_end
        mov     sp, x3

        b       aarch64_init
_END(_start)

        .section .boot.bss,"aw",@nobits

        .align  STACK_ALIGN
_init_stack:
        .space  INIT_STACK_SIZE
_init_stack_end:

# vim: sw=8 ts=8 et
