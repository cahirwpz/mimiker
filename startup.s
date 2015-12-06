	.section .exception
	.globl _start
	.type _start, function
	.set noreorder 	# Disable automatic instruction reordering.

.org 0x000
.ent __reset_vector
__reset_vector:
	la $a0, _start	# Jump to _start.
	jr $a0
	  nop
.end __reset_vector


/* Startup routine placed after exception vectors. */
.org 0x500
.ent _start
_start:
	la	$sp, _estack	# Set stack pointer.
	la	$gp, _gp	# Prepare global pointer.

copy_rom_to_ram:
	/* Copy .data from ROM to RAM.
	 * .data is located in ROM just after .text, so it starts at _etext.
	 * It should land at the beggining of RAM, which is pointed at by _data.
	 * Copy words one by one, until _edata is reached. */
	la	$t1, __data	# dest = _data
	la	$t2, __edata	# dest_end = _edata
	la	$t3, __etext	# src = _etext
.Lcopy_next_word:
	lw	$t0, 0($t3)	# x = *src
	sw	$t0, 0($t1)	# *dest = x
	addiu	$t1, 4		# dest++
	bne	$t1, $t2, .Lcopy_next_word # jump if dest != dest_end
	  addiu	$t3, 4		# src++

clear_bss:
	/*     Clear .bss. It starts at _bss and ends at _ebss. */
	la	$t1, __bss	# dest = _bss
	la	$t2, __ebss	# dest_end = _ebss
.Lclear_next_word:
	/*     Everybody loves MIPS $zero register. */
	sw	$zero, 0($t1)	# *dest = 0
	addiu	$t1, 4		# dest++
	bne	$t1, $t2, .Lclear_next_word
	  nop			# jump if dest != dest_end

	/* Jump to kernel_main(). */
	la $ra, kernel_main_exit # Set return address for kernel_main
    la      $t0, kernel_main
    mtc0    $t0, $30      # Store kernel_main address as exception return address to ErrorEPC
	eret				  # Exception return.

kernel_main_exit:
	/* If for some reason kernel_main returned, loop forever. */
.Lpost_exit_loop:
	j	.Lpost_exit_loop
	  nop

.end _start

_estack = _end + 0x1000
