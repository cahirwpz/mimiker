/*
Copyright (c) 2015, Imagination Technologies Limited and/or its affiliated
group companies

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software without
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <asm.h>
#include <mips/regdef.h>
#include <mips/m32c0.h>

	.set	noreorder       // Don't allow the assembler to reorder instructions.
	.set	noat            // Don't allow the assembler to use r1(at) for synthetic instr.

LEAF(init_gpr)

        // Initialize the general purpose registers and any shadow register
        // sets. Although not necessary, register initialization may be useful
        // during boot, debug, and simulation when certain ways of initializing
        // registers may not work (xor rN, rN, rN for example.)

	// Initialize register sets
        li      $1, 0xdeadbeef      // (0xdeadbeef stands out, kseg2 mapped, odd.)

        // Determine how many shadow sets are implemented (in addition to the
        // base register set.) the first time thru the loop it will initialize
        // using $1 set above.
        // At the bottom og the loop, 1 is subtract from $30 and loop back to
        // next_shadow_set to start the next loop and the next lowest set
        // number.
	mfc0    $29, C0_SRSCTL
	ext	$30, $29, 26, 4		// extract HSS

next_shadow_set:
	// set PSS to shadow set to be initialized
	ins     $29, $30, 6, 4		// insert PSS
	mtc0	$29, C0_SRSCTL

	wrpgpr	$1, $1
	wrpgpr	$2, $1
	wrpgpr	$3, $1
	wrpgpr	$4, $1
	wrpgpr	$5, $1
	wrpgpr	$6, $1
	wrpgpr	$7, $1
	wrpgpr	$8, $1
	wrpgpr	$9, $1
	wrpgpr	$10, $1
	wrpgpr	$11, $1
	wrpgpr	$12, $1
	wrpgpr	$13, $1
	wrpgpr	$14, $1
	wrpgpr	$15, $1
	wrpgpr	$16, $1
	wrpgpr	$17, $1
	wrpgpr	$18, $1
	wrpgpr	$19, $1
	wrpgpr	$20, $1
	wrpgpr	$21, $1
	wrpgpr	$22, $1
	wrpgpr	$23, $1
	wrpgpr	$24, $1
	wrpgpr	$25, $1
	wrpgpr	$26, $1
	wrpgpr	$27, $1
	wrpgpr	$28, $1
	wrpgpr	$29, $1
        // early exit when we get to set 0 so we don't clobber return in $31
	beqz    $30, done_init_gpr
	nop
	wrpgpr	$30, $1
	wrpgpr	$31, $1
	b	next_shadow_set
        // Since the code started with the highest set number this decrements
        // to the next lower number
	add	$30, -1

done_init_gpr:
        jr      ra
        nop

END(init_gpr)
