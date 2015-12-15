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
*              file : $RCSfile: atof.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

/*
FUNCTION
   <<atof>>, <<atoff>>---string to double or float

INDEX
>-------atof
INDEX
>-------atoff

ANSI_SYNOPSIS
>-------#include <stdlib.h>
        double atof(const char *<[s]>);
        float atoff(const char *<[s]>);

TRAD_SYNOPSIS
>-------#include <stdlib.h>
        double atof(<[s]>)
        char *<[s]>;

        float atoff(<[s]>)
        char *<[s]>;

DESCRIPTION
<<atof>> converts the initial portion of a string to a <<double>>.
<<atoff>> converts the initial portion of a string to a <<float>>.

The functions parse the character string <[s]>,
locating a substring which can be converted to a floating-point
value. The substring must match the format:
. [+|-]<[digits]>[.][<[digits]>][(e|E)[+|-]<[digits]>]
The substring converted is the longest initial
fragment of <[s]> that has the expected format, beginning with
the first non-whitespace character.  The substring
is empty if <<str>> is empty, consists entirely
of whitespace, or if the first non-whitespace character is
something other than <<+>>, <<->>, <<.>>, or a digit.

<<atof(<[s]>)>> is implemented as <<strtod(<[s]>, NULL)>>.
<<atoff(<[s]>)>> is implemented as <<strtof(<[s]>, NULL)>>.

RETURNS
<<atof>> returns the converted substring value, if any, as a
<<double>>; or <<0.0>>,  if no conversion could be performed.
If the correct value is out of the range of representable values, plus
or minus <<HUGE_VAL>> is returned, and <<ERANGE>> is stored in
<<errno>>.
If the correct value would cause underflow, <<0.0>> is returned
and <<ERANGE>> is stored in <<errno>>.

<<atoff>> obeys the same rules as <<atof>>, except that it
returns a <<float>>.
PORTABILITY
<<atof>> is ANSI C. <<atof>>, <<atoi>>, and <<atol>> are subsumed by <<strod>>
and <<strol>>, but are used extensively in existing code. These functions are
less reliable, but may be faster if the argument is verified to be in a valid
range.

Supporting OS subroutines required: <<close>>, <<fstat>>, <<isatty>>,
<<lseek>>, <<read>>, <<sbrk>>, <<write>>.
*/

/* same as newlib 2.0 */

#include <stdlib.h>

double
atof (const char *s)
{
  return strtod (s, NULL);
}
