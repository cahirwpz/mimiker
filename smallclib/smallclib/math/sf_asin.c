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
* 		  file : $RCSfile: sf_asin.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* From Newlib 2.0, merged wrapper, factored out common polynomial step */

/* ef_asin.c -- float version of e_asin.c.
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
static const float 
#else
static float 
#endif
one = 1.0, 
huge =  1.000e+30,
pio2_hi = 1.57079637050628662109375f,
pio2_lo = -4.37113900018624283e-8f,
pio4_hi = 0.785398185253143310546875f;

	/* coefficient for R(x^2) */
static const float  coeff[] = {
    3.4793309169e-05, /*ps5 = 0x3811ef08 */
    7.7038154006e-02, /*qs4 = 0x3d9dc62e */
    7.9153501429e-04, /*ps4 = 0x3a4f7f04 */
   -4.0055535734e-02, /*ps3 = 0xbd241146 */
    2.0121252537e-01, /*ps2 = 0x3e4e0aa8 */
   -3.2556581497e-01, /*ps1 = 0xbea6b090 */
    1.6666667163e-01, /*ps0 = 0x3e2aaaab */
   -6.8828397989e-01, /*qs3 = 0xbf303361 */
    2.0209457874e+00, /*qs2 = 0x4001572d */
   -2.4033949375e+00 /*qs1 = 0xc019d139 */
};

#ifdef __STDC__
	float asinf(float x)
#else
	float asinf(x)
	float x;
#endif
{
	float t,w,p,q,c,r,s,tmp;
	__int32_t hx,ix,iw;

	GET_FLOAT_WORD(hx,x);
	ix = hx&0x7fffffff;

	if(ix>= 0x3f800000) {	/* |x|>= 1 */
		if(ix==0x3f800000) {
		/* asin(1)=+-pi/2 with inexact */
#ifdef __MATH_MATCH_NEWLIB_EXCEPTION__
			return x*pio2_hi+x*pio2_lo;
#else /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
			return x*pio2_hi;	
#endif /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
		}
 	    if (!FLT_UWORD_IS_NAN(ix))
 		errno=EDOM;
	    return (x-x)/(x-x);		/* asin(|x|>1) is NaN */   
	} else if (ix<0x3f000000) {	/* |x|<0.5 */
	    if(ix<0x32000000) {		/* if |x| < 2**-27 */
#ifdef __MATH_MATCH_NEWLIB_EXCEPTION__
		if(huge+x>one)
#endif /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
		    return x;/* return x with inexact if x!=0*/
          } else {
		t = x*x;
		w = ___poly2f (t, 6+1, 6+4,  coeff, 1);
		return x+x*w;
	    }
	}

	/* 1> |x|>= 0.5 */
#ifdef __MATH_SOFT_FLOAT__
	x=fabsf(x);
#else /* __MATH_SOFT_FLOAT__ */
	__inst_abs_S(x,x);
#endif /* __MATH_SOFT_FLOAT__ */

#ifdef __MATH_CONST_FROM_MEMORY__
	w = one-x;
	t = w*(float)0.5;
#else /* __MATH_CONST_FROM_MEMORY__ */
	__inst_ldi_S_H(w,0x3f80);
	w=w-x;
	__inst_ldi_S_H(t,0x3f00);
	t = w*t;
#endif /* __MATH_CONST_FROM_MEMORY__ */
	tmp = ___poly2f (t, 6+1, 6+4, coeff, 1);

#ifdef __MATH_SOFT_FLOAT__
	s = __ieee754_sqrtf(t);
#else /* __MATH_SOFT_FLOAT__ */
	__inst_sqrt_S(s,t);
#endif /* __MATH_SOFT_FLOAT__ */

	w  = s;
	GET_FLOAT_WORD(iw,w);
#ifdef __HW_SET_BITS__
	__inst_clear_lo_bits(iw,12);
	SET_FLOAT_WORD(w,iw);
#else /* __HW_SET_BITS__ */
	SET_FLOAT_WORD(w,iw&0xfffff000);
#endif /* __HW_SET_BITS__ */
	c  = (t-w*w)/(s+w);
	r  = tmp;

	p  = (float)2.0*s*r-(pio2_lo-(float)2.0*c);
	q  = pio4_hi-(float)2.0*w;
	t  = pio4_hi-(p-q);

	if(hx>0) return t; else return -t;    
}
