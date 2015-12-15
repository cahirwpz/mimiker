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
*              file : $RCSfile: atexit.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

/*
 * Copyright (c) 1990 Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 */

/*
FUNCTION
<<atexit>>---request execution of functions at program exit

INDEX
>-------atexit

ANSI_SYNOPSIS
>-------#include <stdlib.h>
>-------int atexit (void (*<[function]>)(void));

TRAD_SYNOPSIS
>-------#include <stdlib.h>
>-------int atexit ((<[function]>)
>-------  void (*<[function]>)();

DESCRIPTION
You can use <<atexit>> to enroll functions in a list of functions that
will be called when your program terminates normally.  The argument is
a pointer to a user-defined function (which must not require arguments and
must not return a result).

The functions are kept in a LIFO stack; that is, the last function
enrolled by <<atexit>> will be the first to execute when your program
exits.

There is no built-in limit to the number of functions you can enroll
in this list; however, after every group of 32 functions is enrolled,
<<atexit>> will call <<malloc>> to get space for the next part of the
list.   The initial list of 32 functions is statically allocated, so
you can always count on at least that many slots available.

RETURNS
<<atexit>> returns <<0>> if it succeeds in enrolling your function,
<<-1>> if it fails (possible only if no space was available for
<<malloc>> to extend the list of functions).

PORTABILITY
<<atexit>> is required by the ANSI standard, which also specifies that
implementations must support enrolling at least 32 functions.

Supporting OS subroutines required: <<close>>, <<fstat>>, <<isatty>>,
<<lseek>>, <<read>>, <<sbrk>>, <<write>>.
*/

/* same as newlib 2.0 */

#include <stdlib.h>
#include <low/_atexit.h>
/*
 * Register a function to be performed at exit.
 */

int
atexit(void (*fn) (void))
{
	return __register_exitproc (__et_atexit, fn, NULL, NULL);
}
