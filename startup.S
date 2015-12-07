#include <mips/m32c0.h>
    .set noreorder      // Disable automatic instruction reordering.

/* Exception vector. */
.section .exception
.ent __reset_vector
__reset_vector:
    la  $a0, _start     // Jump to _start.
    jr  $a0
      nop
.end __reset_vector


/* Startup routine. */
.section .init
.globl _start
.type _start, function
.ent _start
_start:
    la  $sp, _estack    // Set stack pointer.
    la  $gp, _gp        // Prepare global pointer.

copy_rom_to_ram:
    /* Copy .data from ROM to RAM.
     * .data is located in ROM just after .text, so it starts at _etext.
     * It should land at the beggining of RAM, which is pointed at by _data.
     * Copy words one by one, until _edata is reached. */
#define t1_dest     $t1
#define t2_dest_end $t2
#define t3_src      $t3
    la      t1_dest,     __data
    la      t2_dest_end, __edata
    la      t3_src,      __etext
.Lcopy_next_word:
    lw      $t0, 0(t3_src)
    sw      $t0, 0(t1_dest)
    addiu   t1_dest, 4
    bne     t1_dest, t2_dest_end, .Lcopy_next_word
     addiu  t3_src,  4

clear_bss:
    /* Clear .bss. It starts at _bss and ends at _ebss. */
    la      t1_dest,     __bss
    la      t2_dest_end, __ebss
.Lclear_next_word:
    /* Everybody loves MIPS $zero register. */
    sw      $zero, 0(t1_dest)
    addiu   t1_dest, 4
    bne     t1_dest, t2_dest_end, .Lclear_next_word
     nop

    /* Jump to kernel_main(). */
    la      $ra, kernel_main_exit   // Set return address for kernel_main
    la      $t0, kernel_main
    mtc0    $t0, C0_ERRPC           // Store kernel_main address as exception return address to ErrorEPC
    eret                            // Exception return.

kernel_main_exit:
    /* If for some reason kernel_main returned, loop forever. */
.Lpost_exit_loop:
    j   .Lpost_exit_loop
     nop

.end _start

_estack = _end + 0x1000
