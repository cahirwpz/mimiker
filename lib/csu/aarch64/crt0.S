#include <aarch64/asm.h>

	.global _start

# This is the starting procedude for user ELF images, used as their
# entry point. It takes care of preparing argc and argc for main().
# For details about environment in which this procedure starts, refer
# to prepare_program_stack in exec.c.

ENTRY(_start)
        # Grab argc from stack.
        ldr     w0, [sp, #0]

        # Prepare argv.
        add     x1, sp, #8

        # Prepare envp, it starts at argv + argc + 1
        lsl     x2, x0, #3
        add     x2, x1, x2
        add     x2, x2, #8

        # Jump to start in crt0-common.c
        # void ___start(int argc, char **argv, char **envp)
        b       ___start
        nop
END(_start)

# vim: sw=8 ts=8 et
