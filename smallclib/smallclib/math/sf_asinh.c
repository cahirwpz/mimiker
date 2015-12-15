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
* 		  file : $RCSfile: sf_asinh.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* From Newlib 2.0, optimized in place for size*/

/* sf_asinh.c -- float version of s_asinh.c.
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

#ifdef __STDC__
static const float 
#else
static float 
#endif
one =  1.0000000000e+00, /* 0x3F800000 */
ln2 =  6.9314718246e-01, /* 0x3f317218 */
huge=  1.0000000000e+30; 

#ifdef __STDC__
	float asinhf(float x)
#else
	float asinhf(x)
	float x;
#endif
{	
	float t,w,rt;
	__int32_t hx,ix;
	GET_FLOAT_WORD(hx,x);
	ix = hx&0x7fffffff;

#ifdef __MATH_NONFINITE__
	if(!FLT_UWORD_IS_FINITE(ix)) 	/* x is inf or NaN */
#ifdef __MATH_MATCH_NEWLIB_EXCEPTION__
	  return x+x;
#else /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
	  return x;
#endif /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
#endif /* __MATH_NONFINITE__ */

	if(ix< 0x31800000) {	/* |x|<2**-28 */
#ifdef __MATH_MATCH_NEWLIB_EXCEPTION__
	    if(huge+x>one) 
#endif /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
		return x;	/* return x inexact except 0 */
	}

	__inst_abs_S (x,x);
#ifdef __MATH_SOFT_FLOAT__
	rt=__ieee754_sqrtf(x*x+one);
#else /* __MATH_SOFT_FLOAT__ */
	rt=(x*x+one);
	__inst_sqrt_S(rt,rt);
#endif  /* __MATH_SOFT_FLOAT__ */

	if(ix>0x4d800000) {	/* |x| > 2**28 */
	    w = __ieee754_logf(x)+ln2;
	} else if (ix>0x40000000) {	/* 2**28 > |x| > 2.0 */	    
	    t = x;
	    w = __ieee754_logf((float)2.0*t+one/(rt+t));
	} else {		/* 2.0 > |x| > 2**-28 */
	    t = x*x;
	    w =log1pf(x+t/(one+rt));
	}

	if(hx>0) return w; else return -w;
}

#ifdef _DOUBLE_IS_32BITS

#ifdef __STDC__
	double asinh(double x)
#else
	double asinh(x)
	double x;
#endif
{
	return (double) asinhf((float) x);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
