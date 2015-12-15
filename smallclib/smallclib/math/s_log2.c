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
* 		  file : $RCSfile: s_log2.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* From Newlib 2.0, no change */

/* @(#)s_log2.c 5.1 93/09/24 */
/* Modification from s_exp10.c Yaakov Selkowitz 2009.  */

/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

/*
FUNCTION
	<<log2>>, <<log2f>>--base 2 logarithm
INDEX
	log2
INDEX
	log2f

ANSI_SYNOPSIS
	#include <math.h>
	double log2(double <[x]>);
	float log2f(float <[x]>);

DESCRIPTION
The <<log2>> functions compute the base-2 logarithm of <[x]>.  A domain error
occurs if the argument is less than zero.  A range error occurs if the
argument is zero.

The Newlib implementations are not full, intrinisic calculations, but
rather are derivatives based on <<log>>.  (Accuracy might be slightly off from
a direct calculation.)  In addition to functions, they are also implemented as
macros defined in math.h:
. #define log2(x) (log (x) / _M_LN2)
. #define log2f(x) (logf (x) / (float) _M_LN2)
To use the functions instead, just undefine the macros first.

You can use the (non-ANSI) function <<matherr>> to specify error
handling for these functions, indirectly through the respective <<log>>
function. 

RETURNS
The <<log2>> functions return
@ifnottex
<<log base-2(<[x]>)>>
@end ifnottex
@tex
$log_2(x)$
@end tex
on success.
When <[x]> is zero, the
returned value is <<-HUGE_VAL>> and <<errno>> is set to <<ERANGE>>.
When <[x]> is negative, the returned value is NaN (not a number) and
<<errno>> is set to <<EDOM>>.  You can control the error behavior via
<<matherr>>.

PORTABILITY
C99, POSIX, System V Interface Definition (Issue 6).
*/

/*
 * wrapper log2(x)
 */

#include "low/_math.h"
#include <errno.h>
#include <math.h>
#undef log2

#ifndef _DOUBLE_IS_32BITS

#ifdef __STDC__
	double log2(double x)		/* wrapper log2 */
#else
	double log2(x)			/* wrapper log2 */
	double x;
#endif
{
  return (log(x) / M_LN2);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
