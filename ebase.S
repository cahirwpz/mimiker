#include <mips/m32c0.h>

        .set noreorder

        .section .ebase

        .org 0x0
        sdbbp
        nop

        .org 0x180
        sdbbp
        nop

        .org 0x200
        la      $k0, _intr_handler
        jr      $k0
        nop

        .org 0x280
        sdbbp
        nop

        .org 0x300
        sdbbp
        nop

        .org 0x380
        sdbbp
        nop

        .org 0x480
        sdbbp
        nop
