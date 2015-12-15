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
*                 file : $RCSfile: swab.c,v $ 
*               author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/*
FUNCTION
	<<swab>>---swap adjacent bytes

ANSI_SYNOPSIS
	#include <unistd.h>
	void swab(const void *<[in]>, void *<[out]>, ssize_t <[n]>);

TRAD_SYNOPSIS
	void swab(<[in]>, <[out]>, <[n]>
	void *<[in]>;
	void *<[out]>;
	ssize_t <[n]>;

DESCRIPTION
	This function copies <[n]> bytes from the memory region
	pointed to by <[in]> to the memory region pointed to by
	<[out]>, exchanging adjacent even and odd bytes.

PORTABILITY
<<swab>> requires no supporting OS subroutines.
*/


/* same as newlib2.0 */

#include <unistd.h>

void
_DEFUN (swab, (b1, b2, length),
	_CONST void *b1 _AND
	void *b2 _AND
	ssize_t length)
{
  const char *from = b1;
  char *to = b2;
  ssize_t ptr;
  for (ptr = 1; ptr < length; ptr += 2)
    {
      char p = from[ptr];
      char q = from[ptr-1];
      to[ptr-1] = p;
      to[ptr  ] = q;
    }
  if (ptr == length) /* I.e., if length is odd, */
    to[ptr-1] = 0;   /* then pad with a NUL. */
}
