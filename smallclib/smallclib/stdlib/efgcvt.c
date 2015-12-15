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
*              file : $RCSfile: efgcvt.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

/*
FUNCTION
<<ecvt>>, <<ecvtf>>, <<fcvt>>, <<fcvtf>>---double or float to string

INDEX
>-------ecvt
INDEX
>-------ecvtf
INDEX
>-------fcvt
INDEX
>-------fcvtf

ANSI_SYNOPSIS
>-------#include <stdlib.h>

>-------char *ecvt(double <[val]>, int <[chars]>, int *<[decpt]>, int *<[sgn]>);
>-------char *ecvtf(float <[val]>, int <[chars]>, int *<[decpt]>, int *<[sgn]>);

>-------char *fcvt(double <[val]>, int <[decimals]>,~
                   int *<[decpt]>, int *<[sgn]>);
>-------char *fcvtf(float <[val]>, int <[decimals]>,~
                    int *<[decpt]>, int *<[sgn]>);

TRAD_SYNOPSIS
>-------#include <stdlib.h>

>-------char *ecvt(<[val]>, <[chars]>, <[decpt]>, <[sgn]>);
>-------double <[val]>;
>-------int <[chars]>;
>-------int *<[decpt]>;
>-------int *<[sgn]>;
>-------char *ecvtf(<[val]>, <[chars]>, <[decpt]>, <[sgn]>);
>-------float <[val]>;
>-------int <[chars]>;
>-------int *<[decpt]>;
>-------int *<[sgn]>;

>-------char *fcvt(<[val]>, <[decimals]>, <[decpt]>, <[sgn]>);
>-------double <[val]>;
>-------int <[decimals]>;
>-------int *<[decpt]>;
>-------int *<[sgn]>;
>-------char *fcvtf(<[val]>, <[decimals]>, <[decpt]>, <[sgn]>);
>-------float <[val]>;
>-------int <[decimals]>;
>-------int *<[decpt]>;
>-------int *<[sgn]>;

DESCRIPTION
<<ecvt>> and <<fcvt>> produce (null-terminated) strings of digits
representating the <<double>> number <[val]>.
<<ecvtf>> and <<fcvtf>> produce the corresponding character
representations of <<float>> numbers.

(The <<stdlib>> functions <<ecvtbuf>> and <<fcvtbuf>> are reentrant
versions of <<ecvt>> and <<fcvt>>.)

The only difference between <<ecvt>> and <<fcvt>> is the
interpretation of the second argument (<[chars]> or <[decimals]>).
For <<ecvt>>, the second argument <[chars]> specifies the total number
of characters to write (which is also the number of significant digits
in the formatted string, since these two functions write only digits).
For <<fcvt>>, the second argument <[decimals]> specifies the number of
characters to write after the decimal point; all digits for the integer
part of <[val]> are always included.

Since <<ecvt>> and <<fcvt>> write only digits in the output string,
they record the location of the decimal point in <<*<[decpt]>>>, and
the sign of the number in <<*<[sgn]>>>.  After formatting a number,
<<*<[decpt]>>> contains the number of digits to the left of the
decimal point.  <<*<[sgn]>>> contains <<0>> if the number is positive,
and <<1>> if it is negative.

RETURNS
All four functions return a pointer to the new string containing a
character representation of <[val]>.

PORTABILITY
None of these functions are ANSI C.

Supporting OS subroutines required: <<close>>, <<fstat>>, <<isatty>>,
<<lseek>>, <<read>>, <<sbrk>>, <<write>>.

NEWPAGE
FUNCTION
<<gvcvt>>, <<gcvtf>>---format double or float as string

INDEX
>-------gcvt
INDEX
>-------gcvtf

ANSI_SYNOPSIS
>-------#include <stdlib.h>

>-------char *gcvt(double <[val]>, int <[precision]>, char *<[buf]>);
>-------char *gcvtf(float <[val]>, int <[precision]>, char *<[buf]>);

TRAD_SYNOPSIS
>-------#include <stdlib.h>

>-------char *gcvt(<[val]>, <[precision]>, <[buf]>);
>-------double <[val]>;
>-------int <[precision]>;
>-------char *<[buf]>;
>-------char *gcvtf(<[val]>, <[precision]>, <[buf]>);
>-------float <[val]>;
>-------int <[precision]>;
>-------char *<[buf]>;

DESCRIPTION
<<gcvt>> writes a fully formatted number as a null-terminated
string in the buffer <<*<[buf]>>>.  <<gdvtf>> produces corresponding
character representations of <<float>> numbers.

<<gcvt>> uses the same rules as the <<printf>> format
`<<%.<[precision]>g>>'---only negative values are signed (with
`<<->>'), and either exponential or ordinary decimal-fraction format
is chosen depending on the number of significant digits (specified by
<[precision]>).

RETURNS
The result is a pointer to the formatted representation of <[val]>
(the same as the argument <[buf]>).

PORTABILITY
Neither function is ANSI C.

Supporting OS subroutines required: <<close>>, <<fstat>>, <<isatty>>,
<<lseek>>, <<read>>, <<sbrk>>, <<write>>.
*/

/* same as newlib 2.0 */
#include <stdlib.h>
#include <low/_stdlib.h>

char *
fcvt (double d, int ndigit, int *decpt, int *sign)
{
  return fcvtbuf (d, ndigit, decpt, sign, NULL);
}

char *
fcvtf (float d, int ndigit, int *decpt, int *sign)
{
  return fcvt ((double) d, ndigit, decpt, sign);
}


char *
gcvtf (float d, int ndigit, char *buf)
{
  double asd = d;
  return gcvt (asd, ndigit, buf);
}

char *
ecvt (double d, int ndigit, int *decpt, int *sign)
{
  return ecvtbuf (d, ndigit, decpt, sign, NULL);
}

char *
ecvtf (float d, int ndigit, int *decpt, int *sign)
{
  return ecvt ((double) d, ndigit, decpt, sign);
}

char *
gcvt (double d, int ndigit, char *buf)
{
  char *tbuf = buf;
  if (d < 0) {
    *buf = '-';
    buf++;
    ndigit--;
  }
  return (_gcvt (d, ndigit, buf, 'g', 0) ? tbuf : 0);
}
