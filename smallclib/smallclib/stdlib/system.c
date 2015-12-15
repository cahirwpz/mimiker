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
*                 file : $RCSfile: system.c,v $ 
*               author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/*
FUNCTION
<<system>>---execute command string

INDEX
	system
INDEX
	_system_r

ANSI_SYNOPSIS
	#include <stdlib.h>
	int system(char *<[s]>);

	int _system_r(void *<[reent]>, char *<[s]>);

TRAD_SYNOPSIS
	#include <stdlib.h>
	int system(<[s]>)
	char *<[s]>;

	int _system_r(<[reent]>, <[s]>)
	char *<[reent]>;
	char *<[s]>;

DESCRIPTION

Use <<system>> to pass a command string <<*<[s]>>> to <</bin/sh>> on
your system, and wait for it to finish executing.

Use ``<<system(NULL)>>'' to test whether your system has <</bin/sh>>
available.

The alternate function <<_system_r>> is a reentrant version.  The
extra argument <[reent]> is a pointer to a reentrancy structure.

RETURNS
<<system(NULL)>> returns a non-zero value if <</bin/sh>> is available, and
<<0>> if it is not.

With a command argument, the result of <<system>> is the exit status
returned by <</bin/sh>>.

PORTABILITY
ANSI C requires <<system>>, but leaves the nature and effects of a
command processor undefined.  ANSI C does, however, specify that
<<system(NULL)>> return zero or nonzero to report on the existence of
a command processor.

POSIX.2 requires <<system>>, and requires that it invoke a <<sh>>.
Where <<sh>> is found is left unspecified.

Supporting OS subroutines required: <<_exit>>, <<_execve>>, <<_fork_r>>,
<<_wait_r>>.
*/

#include <stdio.h>
#include <errno.h>

int system (const char *s)
{
  if (s == NULL)
    return 0;
  errno = ENOSYS;
  return -1;
}

