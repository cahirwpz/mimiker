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
* 		  file : $RCSfile: iswxdigit.c,v $ 
*     		author : $Author  Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* From newlib-2.0 */

/*
FUNCTION
	<<iswxdigit>>---hexadecimal digit wide character test

INDEX
	iswxdigit

ANSI_SYNOPSIS
	#include <wctype.h>
	int iswxdigit(wint_t <[c]>);

TRAD_SYNOPSIS
	#include <wctype.h>
	int iswxdigit(<[c]>)
	wint_t <[c]>;

DESCRIPTION
<<iswxdigit>> is a function which classifies wide character values that
are hexadecimal digits.

RETURNS
<<iswxdigit>> returns non-zero if <[c]> is a hexadecimal digit wide character.

PORTABILITY
<<iswxdigit>> is C99.

No supporting OS subroutines are required.
*/

#include <wctype.h>

int
iswxdigit(wint_t c)
{
  return ((c >= (wint_t)'0' && c <= (wint_t)'9') ||
	  (c >= (wint_t)'a' && c <= (wint_t)'f') ||
	  (c >= (wint_t)'A' && c <= (wint_t)'F'));
}

