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
* 		  file : $RCSfile: sf_acos.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* From Newlib 2.0, merged wrapper, factored out common polynomial step */

/* ef_acos.c -- float version of e_acos.c.
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
one =  1.0000000000e+00, /* 0x3F800000 */
pi =  3.1415925026e+00, /* 0x40490fda */
pio2_hi =  1.5707962513e+00, /* 0x3fc90fda */
pio2_lo =  7.5497894159e-08; /* 0x33a22168 */

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
	float acosf(float x)
#else
	float acosf(x)
	float x;
#endif
{
	float z,r,w,s,c,df;
	__int32_t hx,ix;
	GET_FLOAT_WORD(hx,x);
	ix = hx&0x7fffffff;
	if(ix==0x3f800000) {		/* |x|==1 */
	    if(hx>0) return 0.0;	/* acos(1) = 0  */
	    else
#ifdef __MATH_MATCH_NEWLIB_EXCEPTION__
	       return pi+(float)2.0*pio2_lo;	/* acos(-1)= pi */
#else /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
	    return pi;
#endif /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
	} else if(ix>0x3f800000) {	/* |x| >= 1 */
		float y=x;
 	    if (!FLT_UWORD_IS_NAN(ix))
 		errno=EDOM;
	    return (y-y)/(y-y);		/* acos(|x|>1) is NaN */
	}

	if(ix<0x3f000000) {	/* |x| < 0.5 */
	    if(ix<=0x23000000) 
		return pio2_hi+pio2_lo;/*if|x|<2**-57*/
	    z = x*x;
	    r = ___poly2f(z, 6+1, 6+4, coeff, 1);
	    return pio2_hi - (x - (pio2_lo-x*r));
	}

#ifdef __MATH_SOFT_FLOAT__
	x=fabsf(x);
#else /* __MATH_SOFT_FLOAT__ */
	__inst_abs_S(x,x);
#endif /* __MATH_SOFT_FLOAT__ */

#ifdef __MATH_CONST_FROM_MEMORY__
	z = (one-x)*(float)0.5;
#else /* __MATH_CONST_FROM_MEMORY__ */
	__inst_ldi_S_H(r,0x3f80);
	__inst_ldi_S_H(z,0x3f00);
	z=z*(r-x);
#endif /* __MATH_CONST_FROM_MEMORY__ */

	r = ___poly2f(z, 6+1, 6+4, coeff, 1);
#ifdef __MATH_SOFT_FLOAT__
	s = __ieee754_sqrtf(z);
#else /* __MATH_SOFT_FLOAT__ */
	__inst_sqrt_S(s,z);
#endif /* __MATH_SOFT_FLOAT__ */

	if (hx<0) {		/* x < -0.5 */
	    w = r*s-pio2_lo;
	    return pi - (float)2.0*(s+w);
	} else {			/* x > 0.5 */
	    __int32_t idf;
	    df = s;
	    GET_FLOAT_WORD(idf,df);
#ifdef __HW_SET_BITS__
	    __inst_clear_lo_bits(idf,12);
	    SET_FLOAT_WORD(df,idf);
#else /* __HW_SET_BITS__ */
	    SET_FLOAT_WORD(df,idf&0xfffff000);
#endif /* __HW_SET_BITS__ */
	    c  = (z-df*df)/(s+df);
	    w = r*s+c;
	    return (float)2.0*(df+w);
	}
}
