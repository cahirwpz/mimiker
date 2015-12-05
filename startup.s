	.section .exception
	.globl _start
	.type _start, function

/* Boot code at 0xbfc00000 */
_start:	la	$sp, _estack	# Set stack pointer.
	la	$gp, _gp	# Prepare global pointer.

	/* Copy .data from ROM to RAM.
	 * .data is located in ROM just after .text, so it starts at _etext.
	 * It should land at the beggining of RAM, which is pointed at by _data.
	 * Copy words one by one, until _edata is reached. */
	la	$t1, _data	# dest = _data
	la	$t2, _edata	# dest_end = _edata
	la	$t3, _etext	# src = _etext
copy_next_word:
	lw	$t0, 0($t3)	# x = *src
	sw	$t0, 0($t1)	# *dest = x
	addiu	$t1, 4		# dest++
	addiu	$t3, 4		# src++
	bne	$t1, $t2, copy_next_word
	  nop			# jump if dest != dest_end

	/*     Clean .bss. It starts at _bss and ends at _ebss. */
	la	$t1, _bss	# dest = _bss
	la	$t2, _ebss	# dest_end = _ebss
clear_next_word:
	/*     Everybody loves MIPS $zero register. */
	sw	$zero, 0($t1)	# *dest = 0
	addiu	$t1, 4		# dest++
	bne	$t1, $t2, clear_next_word
	  nop			# jump if dest != dest_end

	/* Jump to kernel_main(). */
	jal	kernel_main

	/* If for some reason kernel_main returned, loop forever. */
loop:
	j	loop

	.text

_estack = _end + 0x1000
