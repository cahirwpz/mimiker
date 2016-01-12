#include <mips/m32c0.h>

    .set noreorder      // Disable automatic instruction reordering.

/* Exception vector. */
.section .exception
.globl __reset_vector
/* The .org 0x0 directive explicitly forces the routine to be placed
 * exactly at the beginning of the .exception section. Not using this
 * directive should yield the same result, but it is set explicitly in
 * order to highlight the fact that offsets within .exception section
 * shall not be arbitrary. This may save us from problems in case the
 * assembler decided to place __reset_vector anywhere else but
 * 0xbfc00000.
 */
.org 0x0
__reset_vector:
        la      $k0, _start     // Jump to _start.
        jr      $k0
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

/* Startup routine. */
.section .init
.globl _start
.type _start, function
.ent _start
_start:
        la      $a0, init_gpr
        jalr    $a0
        nop

        la      $a0, init_cp0
        jalr    $a0
        nop

        la      $a0, init_tlb
        jalr    $a0
        nop

        la      $a0, init_icache
        jalr    $a0
        nop

        la      $a0, init_dcache
        jalr    $a0
        nop

        la      $sp, _estack    // Set stack pointer.
        la      $gp, _gp        // Prepare global pointer.

copy_rom_to_ram:
    /* Copy .data from ROM to RAM.
     * .data is located in ROM just after .text, so it starts at _etext.
     * It should land at the beggining of RAM, which is pointed at by _data.
     * Copy words one by one, until _edata is reached. */
#define t1_dest     $t1
#define t2_dest_end $t2
#define t3_src      $t3

        la      t1_dest, __data
        la      t2_dest_end, __edata
        la      t3_src, __etext

1:      lw      $t0, 0(t3_src)
        sw      $t0, 0(t1_dest)
        addiu   t1_dest, 4
        bne     t1_dest, t2_dest_end, 1b
        addiu   t3_src, 4

clear_bss:
        /* Clear .bss. It starts at _bss and ends at _ebss. */
        la      t1_dest, __bss
        la      t2_dest_end, __ebss

        /* Everybody loves MIPS $zero register. */
1:      sw      $zero, 0(t1_dest)
        addiu   t1_dest, 4
        bne     t1_dest, t2_dest_end, 1b
        nop

        /* Jump to kernel_main(). */
        la      $ra, kernel_main_exit   // Set return address for kernel_main
        la      $t0, kernel_main
        mtc0    $t0, C0_ERRPC           // Store kernel_main address as exception return address to ErrorEPC
        eret                            // Exception return.

kernel_main_exit:
        /* If for some reason kernel_main returned, loop forever. */
1:      b   1b
        nop

.end _start
