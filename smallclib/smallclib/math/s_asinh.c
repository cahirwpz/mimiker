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
* 		  file : $RCSfile: s_asinh.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* From Newlib 2.0, optimized in place for size*/

/* @(#)s_asinh.c 5.1 93/09/24 */
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
	<<asinh>>, <<asinhf>>---inverse hyperbolic sine 

INDEX
	asinh
INDEX
	asinhf

ANSI_SYNOPSIS
	#include <math.h>
	double asinh(double <[x]>);
	float asinhf(float <[x]>);

TRAD_SYNOPSIS
	#include <math.h>
	double asinh(<[x]>)
	double <[x]>;

	float asinhf(<[x]>)
	float <[x]>;

DESCRIPTION
<<asinh>> calculates the inverse hyperbolic sine of <[x]>.
<<asinh>> is defined as 
@ifnottex
. sgn(<[x]>) * log(abs(<[x]>) + sqrt(1+<[x]>*<[x]>))
@end ifnottex
@tex
$$sign(x) \times ln\Bigl(|x| + \sqrt{1+x^2}\Bigr)$$
@end tex

<<asinhf>> is identical, other than taking and returning floats.

RETURNS
<<asinh>> and <<asinhf>> return the calculated value.

PORTABILITY
Neither <<asinh>> nor <<asinhf>> are ANSI C.

*/

/* asinh(x)
 * Method :
 *	Based on 
 *		asinh(x) = sign(x) * log [ |x| + sqrt(x*x+1) ]
 *	we have
 *	asinh(x) := x  if  1+x*x=1,
 *		 := sign(x)*(log(x)+ln2)) for large |x|, else
 *		 := sign(x)*log(2|x|+1/(|x|+sqrt(x*x+1))) if|x|>2, else
 *		 := sign(x)*log1p(|x| + x^2/(1 + sqrt(1+x^2)))  
 */

#include "low/_math.h"
#include "low/_fpuinst.h"

#ifndef _DOUBLE_IS_32BITS

#ifdef __STDC__
static const double 
#else
static double 
#endif
ln2 =  6.93147180559945286227e-01, /* 0x3FE62E42, 0xFEFA39EF */
one =  1.0,
huge=  1.00000000000000000000e+300; 

#ifdef __STDC__
	double asinh(double x)
#else
	double asinh(x)
	double x;
#endif
{	
	double t,w, rt;
	__int32_t hx,ix;
	GET_HIGH_WORD(hx,x);
	ix = hx&0x7fffffff;

#ifdef __MATH_NONFINITE__
	if(ix>=0x7ff00000) 	/* x is inf or NaN */
#ifdef __MATH_MATCH_NEWLIB_EXCEPTION__
	  return x+x;
#else /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
	  return x;
#endif /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
#endif /* __MATH_NONFINITE__ */

	if(ix< 0x3e300000) {	/* |x|<2**-28 */
#ifdef __MATH_MATCH_NEWLIB_EXCEPTION__
	    if(huge+x>one) 
#endif /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
	      return x;	/* return x inexact except 0 */
	}

	__inst_abs_D (x,x);
#ifdef __MATH_SOFT_FLOAT__
	rt=__ieee754_sqrt(x*x+one);
#else /* __MATH_SOFT_FLOAT__ */
	rt=(x*x+one);
	__inst_sqrt_D(rt,rt);
#endif  /* __MATH_SOFT_FLOAT__ */

	if(ix>0x41b00000) {	/* |x| > 2**28 */
	    w = __ieee754_log(x)+ln2;
	} else if (ix>0x40000000) {	/* 2**28 > |x| > 2.0 */
	    t = x;
	    w = __ieee754_log(2.0*t+one/(rt+t));
	} else {		/* 2.0 > |x| > 2**-28 */
	    t = x*x;
	    w =log1p(x+t/(one+rt));
	}

#ifdef __MATH_SOFT_FLOAT__
	{
	    __uint32_t hw;
	    GET_HIGH_WORD (hw,w);
	    hw=hw&0x7fffffff;
	    hx=hx&0x80000000;
	    SET_HIGH_WORD(w,hw|hx);
      return w;
	}
#else /* __MATH_SOFT_FLOAT__ */
	__inst_copysign_Dx (w,hx);
	return w;
#endif /* __MATH_SOFT_FLOAT__ */
}

#endif /* _DOUBLE_IS_32BITS */
