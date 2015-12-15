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
*                 file : $RCSfile: assert.c,v $ 
*               author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/*
FUNCTION
<<assert>>---macro for debugging diagnostics

INDEX
	assert

ANSI_SYNOPSIS
	#include <assert.h>
	void assert(int <[expression]>);

DESCRIPTION
	Use this macro to embed debuggging diagnostic statements in
	your programs.  The argument <[expression]> should be an
	expression which evaluates to true (nonzero) when your program
	is working as you intended.

	When <[expression]> evaluates to false (zero), <<assert>>
	calls <<abort>>, after first printing a message showing what
	failed and where:

. Assertion failed: <[expression]>, file <[filename]>, line <[lineno]>, function: <[func]>

	If the name of the current function is not known (for example,
	when using a C89 compiler that does not understand __func__),
	the function location is omitted.

	The macro is defined to permit you to turn off all uses of
	<<assert>> at compile time by defining <<NDEBUG>> as a
	preprocessor variable.   If you do this, the <<assert>> macro
	expands to

. (void(0))

RETURNS
	<<assert>> does not return a value.

PORTABILITY
	The <<assert>> macro is required by ANSI, as is the behavior
	when <<NDEBUG>> is defined.

Supporting OS subroutines required (only if enabled): <<close>>, <<fstat>>,
<<getpid>>, <<isatty>>, <<kill>>, <<lseek>>, <<read>>, <<sbrk>>, <<write>>.
*/

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef HAVE_ASSERT_FUNC
/* func can be NULL, in which case no function information is given.  */
void __assert_func (const char *file, int line, const char *func, const char *failedexpr)
{
  fiprintf(stderr,
	   "assertion \"%s\" failed: file \"%s\", line %d%s%s\n",
	   failedexpr, file, line,
	   func ? ", function: " : "", func ? func : "");
  abort();
  /* NOTREACHED */
}
#endif /* HAVE_ASSERT_FUNC */

void __assert (const char *file, int line, const char *failedexpr)
{
   __assert_func (file, line, NULL, failedexpr);
  /* NOTREACHED */
}
