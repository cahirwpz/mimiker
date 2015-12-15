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
* 		  file : $RCSfile: sf_sinh.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* From Newlib 2.0, optimized in place for size, merged wrapper function*/

/* ef_sinh.c -- float version of e_sinh.c.
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
#include "low/_gpinst.h"
#include <errno.h>

#ifdef __STDC__
static const float shuge = 1.0e37;
#else
static float shuge = 1.0e37;
#endif

#define SHUGE_BITS 0x7CF0BDC2

#ifdef __STDC__
	float sinhf(float x)
#else
	float sinhf(x)
	float x;
#endif
{	
	float t,w,h,ax,one,two;
	__int32_t ix,jx;

	GET_FLOAT_WORD(jx,x);
	ix = jx&0x7fffffff;

#ifdef __MATH_NONFINITE__
	/* x is INF or NaN */
	if(!FLT_UWORD_IS_FINITE(ix)) 
#ifdef __MATH_MATCH_NEWLIB_EXCEPTION__
	    return x+x;	
#else /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
	    return x;
#endif /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
#endif /* __MATH_NONFINITE__ */

#ifdef __MATH_SOFT_FLOAT__
	ax=fabsf(x);
#else /* __MATH_SOFT_FLOAT__ */
	__inst_abs_S(ax,x);
#endif /* __MATH_SOFT_FLOAT__ */

#ifdef __MATH_CONST_FROM_MEMORY__
	h = 0.5;
	one=1.0f;
#else /* __MATH_CONST_FROM_MEMORY__ */
	__inst_ldi_S_H(h,0x3f00);
	__inst_ldi_S_H(one,0x3f80);
#endif
	if (jx<0) h = -h;
	t = expm1f(ax);
	/* |x| in [0,22], return sign(x)*0.5*(E+E/(E+1))) */
	if (ix < 0x41b00000) {		/* |x|<22 */
	    if (ix<0x31800000) 		/* |x|<2**-28 */
#ifdef __MATH_MATCH_NEWLIB_EXCEPTION__
		/* Condition generates inexact, not required by C99 */
		if(shuge+x>one) 
#endif /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
		    return x;/* sinh(tiny) = tiny */

	    if(ix<0x3f800000) {
#ifdef __MATH_CONST_FROM_MEMORY__
		two=2.0f;
#else /* __MATH_CONST_FROM_MEMORY__ */
		__inst_ldi_S_H(two,0x4000);
#endif
		return h*(two*t-t*t/(t+one));
	    }
	    else
		return h*(t+t/(t+one));
	}

	/* |x| in [22, log(maxdouble)] return 0.5*exp(|x|) */
	if (ix<=FLT_UWORD_LOG_MAX)  {
	    w = t + one;
	    return h*w;
	}

	/* |x| in [log(maxdouble), overflowthresold] */
	if (ix<=FLT_UWORD_LOG_2MAX) {
	    w = expm1f(h*x) + one;
	    t = h*w;
	    t = t*w;
	    return t;
	}

	/* |x| > overflowthresold, sinh(x) overflow */
	errno=ERANGE;
	return x*shuge;
}
