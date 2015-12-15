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
* 		  file : $RCSfile: s_copysign.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* Softfloat from Newlib 2.0, FP optimized version from scratch */

/* @(#)s_copysign.c 5.1 93/09/24 */
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
<<copysign>>, <<copysignf>>---sign of <[y]>, magnitude of <[x]>

INDEX
	copysign
INDEX
	copysignf

ANSI_SYNOPSIS
	#include <math.h>
	double copysign (double <[x]>, double <[y]>);
	float copysignf (float <[x]>, float <[y]>);

TRAD_SYNOPSIS
	#include <math.h>
	double copysign (<[x]>, <[y]>)
	double <[x]>;
	double <[y]>;

	float copysignf (<[x]>, <[y]>)
	float <[x]>;
	float <[y]>;

DESCRIPTION
<<copysign>> constructs a number with the magnitude (absolute value)
of its first argument, <[x]>, and the sign of its second argument,
<[y]>.

<<copysignf>> does the same thing; the two functions differ only in
the type of their arguments and result.

RETURNS
<<copysign>> returns a <<double>> with the magnitude of
<[x]> and the sign of <[y]>.
<<copysignf>> returns a <<float>> with the magnitude of
<[x]> and the sign of <[y]>.

PORTABILITY
<<copysign>> is not required by either ANSI C or the System V Interface
Definition (Issue 2).

*/

/*
 * copysign(double x, double y)
 * copysign(x,y) returns a value with the magnitude of x and
 * with the sign bit of y.
 */

#include "low/_math.h"
#include "low/_fpuinst.h"

double copysign(double x, double y)
{
#ifdef __MATH_SOFT_FLOAT__ 
  __uint32_t hx,hy;
  GET_HIGH_WORD(hx,x); 
  GET_HIGH_WORD(hy,y);
  SET_HIGH_WORD(x,(hx&0x7fffffff)|(hy&0x80000000));
#else /* __MATH_SOFT_FLOAT__ */
  __inst_copysign_D (x,y);
#endif /* __MATH_SOFT_FLOAT__ */
  return x;
}
