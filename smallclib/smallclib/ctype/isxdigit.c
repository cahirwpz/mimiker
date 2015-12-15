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
* 		  file : $RCSfile: isxdigit.c,v $ 
*     		author : $Author  Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* From newlib-2.0, no change */

/*
FUNCTION
<<isxdigit>>---hexadecimal digit predicate

INDEX
isxdigit

ANSI_SYNOPSIS
#include <ctype.h>
int isxdigit(int <[c]>);

TRAD_SYNOPSIS
#include <ctype.h>
int isxdigit(int <[c]>);

DESCRIPTION
<<isxdigit>> is a macro which classifies ASCII integer values by table
lookup.  It is a predicate returning non-zero for hexadecimal digits,
and <<0>> for other characters.  It is defined only when
<<isascii>>(<[c]>) is true or <[c]> is EOF.

You can use a compiled subroutine instead of the macro definition by
undefining the macro using `<<#undef isxdigit>>'.

RETURNS
<<isxdigit>> returns non-zero if <[c]> is a hexadecimal digit
(<<0>>--<<9>>, <<a>>--<<f>>, or <<A>>--<<F>>).

PORTABILITY
<<isxdigit>> is ANSI C.

No supporting OS subroutines are required.
*/

#include <ctype.h>
#include <low/_ctype.h>

int isxdigit(int c)
{
	return isdigit(c) || ((unsigned)c|32)-'a' < 6;
}
