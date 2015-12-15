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
* 		  file : $RCSfile: sf_acosh.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* From Newlib 2.0, wrapper merged to save space */

/* ef_acosh.c -- float version of e_acosh.c.
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
 *
 */

#include "low/_math.h"
#include "low/_fpuinst.h"
#include <errno.h>

#define __MATH_CONST_FROM_MEMORY__

#ifdef __STDC__
static const float 
#else
static float 
#endif
ln2	= 6.9314718246e-01;  /* 0x3f317218 */

#ifdef __STDC__
	float acoshf(float x)
#else
	float acoshf(x)
	float x;
#endif
{	
	float t;
	__int32_t hx;
	float one, two;

#ifdef __MATH_CONST_FROM_MEMORY__
	one=1.0; two=2.0;
#else /* __MATH_CONST_FROM_MEMORY__ */
	__inst_ldi_S_W (one,1);
	__inst_ldi_S_W (two,2);
#endif /* __MATH_CONST_FROM_MEMORY__ */

	GET_FLOAT_WORD(hx,x);
#ifdef __MATH_NONFINITE__
	if(!FLT_UWORD_IS_FINITE(hx&0x7fffffff)) {	/* x is inf of NaN */
	  return x;
	} else
#endif /* __MATH_NONFINITE__ */
	if(hx<0x3f800000) {		/* x < 1 */
	  errno=EDOM;
	  return (x-x)/(x-x);
	}
	else 
	  if(hx >=0x4d800000) {	/* x > 2**28 */
		return __ieee754_logf(x)+ln2;	/* acosh(huge)=log(2x) */
	} 
	else if (hx > 0x40000000) {	/* 2**28 > x > 2 */
	    t=x*x-one;
#ifdef __MATH_SOFT_FLOAT__
	    t=sqrtf(t);
#else /* __MATH_SOFT_FLOAT */ 
	    __inst_sqrt_S (t,t);
#endif /* __MATH_SOFT_FLOAT */ 
	    return __ieee754_logf(two*x-one/(x+t));
	} else {			/* 1<x<2 */
	  float tmp;
	  t = x-one;
	  tmp=two*t+t*t;

#ifdef __MATH_SOFT_FLOAT__
	    tmp=sqrtf(tmp);
#else /* __MATH_SOFT_FLOAT */ 
	    __inst_sqrt_S (tmp,tmp);
#endif /* __MATH_SOFT_FLOAT */ 
	    return log1pf(t+tmp);
	}
}
