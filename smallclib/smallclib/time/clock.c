/*******************************************************************************
 *
 * Copyright 2014-2015, Imagination Technologies Limited and/or its
 *                      affiliated group companies.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

/******************************************************************************
*              file : $RCSfile: clock.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

/* From newlib 2.0 */

/*
 * clock.c
 * Original Author Imagination Technologies Ltd
 *
 * Determines the processor time used by the program since invocation. The time
 * in seconds is the value returned divided by the value of the macro CLK_TCK.
 * If the processor time used is not available, (clock_t) -1 is returned.
 */

/*
FUNCTION
<<clock>>---cumulative processor time

INDEX
	clock

ANSI_SYNOPSIS
	#include <time.h>
	clock_t clock(void);

TRAD_SYNOPSIS
	#include <time.h>
	clock_t clock();

DESCRIPTION
Calculates the best available approximation of the cumulative amount
of time used by your program since it started.  To convert the result
into seconds, divide by the macro <<CLOCKS_PER_SEC>>.

RETURNS
The amount of processor time used so far by your program, in units
defined by the machine-dependent macro <<CLOCKS_PER_SEC>>.  If no
measurement is available, the result is (clock_t)<<-1>>.

PORTABILITY
ANSI C requires <<clock>> and <<CLOCKS_PER_SEC>>.

Supporting OS subroutine required: <<times>>.
*/

#include <time.h>

clock_t clock (void)
{
  return 0;
}

