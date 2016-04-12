#include <asm.h>
#include <mips/m32c0.h>

        .set noreorder

        .section .init

        .local copy_rom_to_ram
        .local clear_bss
        .local kernel_main_exit

# The .org 0x0 directive explicitly forces the routine to be placed
# exactly at the beginning of the .exception section. Not using this
# directive should yield the same result, but it is set explicitly in
# order to highlight the fact that offsets within .exception section
# shall not be arbitrary. This may save us from problems in case the
# assembler decided to place __reset_vector anywhere else but
# 0xbfc00000.

        .org 0x0
LEAF(_start)
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

        la      $sp, _estack    # Set stack pointer.
        la      $gp, _gp        # Prepare global pointer.

copy_rom_to_ram:
        # Copy .data from ROM to RAM.
        # .data is located in ROM just after .text, so it starts at _etext.
        # It should land at the beginning of RAM, which is pointed at by _data.
        # Copy words one by one, until _edata is reached.
        la      $t1, __data
        la      $t2, __edata
        la      $t3, __etext

1:      lw      $t0, ($t3)
        sw      $t0, ($t1)
        addiu   $t1, 4
        bne     $t1, $t2, 1b
        addiu   $t3, 4

clear_bss:
        # Clear .bss. It starts at _bss and ends at _ebss.
        la      $t1, __bss
        la      $t2, __ebss

1:      sw      $zero, ($t1)
        addiu   $t1, 4
        bne     $t1, $t2, 1b
        nop

        # StatusERL is set by the processor when a Reset, Soft Reset, NMI or
        # Cache Error exception are taken. When ERL is set:
        # - The processor is running in kernel mode
        # - Hardware and software interrupts are disabled
        # - The ERET instruction will use the return address held in ErrorEPC
        #   instead of EPC

        # Jump to kernel_main()
        la      $ra, kernel_main_exit # Set return address to kernel_main_exit
        la      $t0, kernel_main
        mtc0    $t0, C0_ERRPC         # Return from exception to kernel_main
        eret                          # Sets StatusERL to 0 

kernel_main_exit:
        # If for some reason kernel_main returned call a debugger.
        sdbbp
        nop

        END(_start)
