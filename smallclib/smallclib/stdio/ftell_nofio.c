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
* 		  file : $RCSfile:  $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/*
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/*
FUNCTION
<<ftell>>, <<ftello>>---return position in a stream or file

INDEX
	ftell
INDEX
	ftello

ANSI_SYNOPSIS
	#include <stdio.h>
	long ftell(FILE *<[fp]>);
	off_t ftello(FILE *<[fp]>);

TRAD_SYNOPSIS
	#include <stdio.h>
	long ftell(<[fp]>)
	FILE *<[fp]>;

	off_t ftello(<[fp]>)
	FILE *<[fp]>;

DESCRIPTION
Objects of type <<FILE>> can have a ``position'' that records how much
of the file your program has already read.  Many of the <<stdio>> functions
depend on this position, and many change it as a side effect.

The result of <<ftell>>/<<ftello>> is the current position for a file
identified by <[fp]>.  If you record this result, you can later
use it with <<fseek>>/<<fseeko>> to return the file to this
position.  The difference between <<ftell>> and <<ftello>> is that
<<ftell>> returns <<long>> and <<ftello>> returns <<off_t>>.

In the current implementation, <<ftell>>/<<ftello>> simply uses a character
count to represent the file position; this is the same number that
would be recorded by <<fgetpos>>.

RETURNS
<<ftell>>/<<ftello>> return the file position, if possible.  If they cannot do
this, they return <<-1L>>.  Failure occurs on streams that do not support
positioning; the global <<errno>> indicates this condition with the
value <<ESPIPE>>.

PORTABILITY
<<ftell>> is required by the ANSI C standard, but the meaning of its
result (when successful) is not specified beyond requiring that it be
acceptable as an argument to <<fseek>>.  In particular, other
conforming C implementations may return a different result from
<<ftell>> than what <<fgetpos>> records.

<<ftello>> is defined by the Single Unix specification.

No supporting OS subroutines are required.
*/

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "%W% (Berkeley) %G%";
#endif /* LIBC_SCCS and not lint */

/*
 * ftell: return current offset.
 */

#include <_ansi.h>
#include <stdio.h>
#include <errno.h>

long
_DEFUN(ftell_nofio, (fp),
       register FILE * fp)
{
  (void) fp;
  return 1L;
}