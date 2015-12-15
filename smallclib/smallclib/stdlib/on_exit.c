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
*              file : $RCSfile: on_exit.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/
/*
 * Copyright (c) 1990 Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 *
 * This function is a modified version of atexit.c
 */

/*
FUNCTION
<<on_exit>>---request execution of function with argument at program exit

INDEX
>-------on_exit

ANSI_SYNOPSIS
>-------#include <stdlib.h>
>-------int on_exit (void (*<[function]>)(int, void *), void *<[arg]>);

TRAD_SYNOPSIS
>-------#include <stdlib.h>
>-------int on_exit ((<[function]>, <[arg]>)
>-------  void (*<[function]>)(int, void *);
>-------  void *<[arg]>;

DESCRIPTION
You can use <<on_exit>> to enroll functions in a list of functions that
will be called when your program terminates normally.  The argument is
a pointer to a user-defined function which takes two arguments.  The
first is the status code passed to exit and the second argument is of type
pointer to void.  The function must not return a result.  The value
of <[arg]> is registered and passed as the argument to <[function]>.

The functions are kept in a LIFO stack; that is, the last function
enrolled by <<atexit>> or <<on_exit>> will be the first to execute when~
your program exits.  You can intermix functions using <<atexit>> and
<<on_exit>>.

There is no built-in limit to the number of functions you can enroll
in this list; however, after every group of 32 functions is enrolled,
<<atexit>>/<<on_exit>> will call <<malloc>> to get space for the next part~
of the list.   The initial list of 32 functions is statically allocated, so
you can always count on at least that many slots available.

RETURNS
<<on_exit>> returns <<0>> if it succeeds in enrolling your function,
<<-1>> if it fails (possible only if no space was available for
<<malloc>> to extend the list of functions).

PORTABILITY
<<on_exit>> is a non-standard glibc extension

Supporting OS subroutines required: None
*/


#include <stdlib.h>
#include <low/_atexit.h>

/*
 * Register a function to be performed at exit.
 */

int
on_exit (void (*fn)(int status, void *arg), void *arg)
{
	return __register_exitproc (__et_onexit, (void (*)(void)) fn, arg, NULL);
}
