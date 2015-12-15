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
* 		  file : $RCSfile: e_acosh.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* From Newlib 2.0, optimized in place for size, merged wrapper function */

/* @(#)e_cosh.c 5.1 93/09/24 */
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

/* __ieee754_cosh(x)
 * Method : 
 * mathematically cosh(x) if defined to be (exp(x)+exp(-x))/2
 *	1. Replace x by |x| (cosh(x) = cosh(-x)). 
 *	2. 
 *		                                        [ exp(x) - 1 ]^2 
 *	    0        <= x <= ln2/2  :  cosh(x) := 1 + -------------------
 *			       			           2*exp(x)
 *
 *		                                  exp(x) +  1/exp(x)
 *	    ln2/2    <= x <= 22     :  cosh(x) := -------------------
 *			       			          2
 *	    22       <= x <= lnovft :  cosh(x) := exp(x)/2 
 *	    lnovft   <= x <= ln2ovft:  cosh(x) := exp(x/2)/2 * exp(x/2)
 *	    ln2ovft  <  x	    :  cosh(x) := huge*huge (overflow)
 *
 * Special cases:
 *	cosh(x) is |x| if x is +INF, -INF, or NaN.
 *	only cosh(0)=1 is exact for finite x.
 */

#include "low/_math.h"
#include "low/_fpuinst.h"
#include <errno.h>

#ifndef _DOUBLE_IS_32BITS

#ifdef __STDC__
static const double huge = 1.0e300;
#else
static double huge = 1.0e300;
#endif

#ifdef __STDC__
	double cosh(double x)
#else
	double cosh(x)
	double x;
#endif
{	
	double t,w;
	__int32_t ix;
	__uint32_t lx;
	double half;

    /* High word of |x|. */
	GET_HIGH_WORD(ix,x);
	ix &= 0x7fffffff;

#ifdef __MATH_NONFINITE__
    /* x is INF or NaN */
	if(ix>=0x7ff00000) 
#ifdef __MATH_MATCH_NEWLIB_EXCEPTION__
	  return x*x;	
#else /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
	  return x;
#endif /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
#endif /* __MATH_NONFINITE__ */

#ifdef __MATH_SOFT_FLOAT__
	  x=fabs(x);
#else /* __MATH_SOFT_FLOAT__ */
	  __inst_abs_D (x,x);
#endif /* __MATH_SOFT_FLOAT__ */

	  t = __ieee754_exp(x);
#ifndef __MATH_SIZE_OVER_ULP__
    /* |x| in [0,0.5*ln2], return 1+expm1(|x|)^2/(2*exp(|x|)) */
	if(ix<0x3fd62e43) {
	    double one;
#ifdef __MATH_CONST_FROM_MEMORY__
	one=1.0;
#else /* __MATH_CONST_FROM_MEMORY__ */
	__inst_ldi_D_W(one, 1);
#endif /* __MATH_CONST_FROM_MEMORY__ */

	    t = t-one;
	    w = one+t;
	    if (ix<0x3c800000) return w;	/* cosh(tiny) = 1 */
	    return one+(t*t)/(w+w);
	}
#endif /* __MATH_SIZE_OVER_ULP__ */

#ifdef __MATH_CONST_FROM_MEMORY__
	half=0.5;
#else /* __MATH_CONST_FROM_MEMORY__ */
	__inst_ldi_D_H(half, 0x3fe00000);
#endif /* __MATH_CONST_FROM_MEMORY__ */

    /* |x| in [0.5*ln2,22], return (exp(|x|)+1/exp(|x|)/2; */
	if (ix < 0x40360000) {
	  return t*half+half/t;
	}

    /* |x| in [22, log(maxdouble)] return half*exp(|x|) */
	if (ix < 0x40862E42)  return t*half;

    /* |x| in [log(maxdouble), overflowthresold] */
	GET_LOW_WORD(lx,x);
	if (ix<0x408633CE || 
             (ix==0x408633ce && lx<=(__uint32_t)0x8fb9f87d)) {
	    w = __ieee754_exp(half*x);
	    t = half*w;
	    return t*w;
	}

    /* |x| > overflowthresold, cosh(x) overflow */
	errno=ERANGE;
#ifdef __MATH_EXCEPTION__
	return huge*huge;
#else /* __MATH_EXCEPTION__ */
	return HUGE_VAL;
#endif /* __MATH_EXCEPTION__ */
}

#endif /* defined(_DOUBLE_IS_32BITS) */
