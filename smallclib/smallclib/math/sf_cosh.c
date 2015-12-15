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
* 		  file : $RCSfile: ef_cosh.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* From Newlib 2.0, optimized in place for size, merged wrapper function*/

/* ef_cosh.c -- float version of e_cosh.c.
 * Conversion to float by Ian Lance Taylor, Cygnus Support, ian@cygnus.com.
 */

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

#include "low/_math.h"
#include "low/_fpuinst.h"
#include <errno.h>

#ifdef __v810__
#define const
#endif

#ifdef __STDC__
static const float huge = 1.0e30;
#else
static float huge = 1.0e30;
#endif

#ifdef __STDC__
	float coshf(float x)
#else
	float coshf(x)
	float x;
#endif
{	
	float t,w, half;
	__int32_t ix;

	GET_FLOAT_WORD(ix,x);
	ix &= 0x7fffffff;

#ifdef __MATH_NONFINITE__
    /* x is INF or NaN */
	if(!FLT_UWORD_IS_FINITE(ix)) 
#ifdef __MATH_MATCH_NEWLIB_EXCEPTION__
	  return x*x;	
#else /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
	  return x;
#endif /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
#endif /* __MATH_NONFINITE__ */

#ifdef __MATH_SOFT_FLOAT__
	  x=fabsf(x);
#else /* __MATH_SOFT_FLOAT__ */
	  __inst_abs_S (x,x);
#endif /* __MATH_SOFT_FLOAT__ */

	  t = __ieee754_expf(x);
    /* |x| in [0,0.5*ln2], return 1+expm1(|x|)^2/(2*exp(|x|)) */
#ifndef __MATH_SIZE_OVER_ULP__
	if(ix<0x3eb17218) {
	  float one;
#if 1//def __MATH_CONST_FROM_MEMORY__
	  one=1.0;
#else /* __MATH_CONST_FROM_MEMORY__ */
	__inst_ldi_S_H(one, 0x3f80);
#endif /* __MATH_CONST_FROM_MEMORY__ */
	    t = t - one;
	    w = one+t;
	    if (ix<0x24000000) return w;	/* cosh(tiny) = 1 */
	    return one+(t*t)/(w+w);
	}
#endif /* __MATH_SIZE_OVER_ULP__ */

#ifdef __MATH_CONST_FROM_MEMORY__
	half=0.5;
#else /* __MATH_CONST_FROM_MEMORY__ */
	__inst_ldi_S_H(half, 0x3f00);
#endif /* __MATH_CONST_FROM_MEMORY__ */

    /* |x| in [0.5*ln2,log(maxdouble)], return (exp(|x|)+1/exp(|x|)/2; */
	if (ix <= FLT_UWORD_LOG_MAX)
	  return half*t+half/t;

    /* |x| in [log(maxdouble), overflowthresold] */
	if (ix <= FLT_UWORD_LOG_2MAX) {
	    w = __ieee754_expf(half*x);
	    t = half*w;
	    return t*w;
	}

    /* |x| > overflowthresold, cosh(x) overflow */
	errno=ERANGE;
#ifdef __MATH_EXCEPTION__
	return huge*huge;
#else /* __MATH_EXCEPTION__ */
	return HUGE_VALF;
#endif /* __MATH_EXCEPTION__ */
}
