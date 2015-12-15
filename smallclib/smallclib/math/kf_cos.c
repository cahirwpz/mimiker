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
* 		  file : $RCSfile: kf_cos.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* From Newlib 2.0, make inexact exception optional */

/* kf_cos.c -- float version of k_cos.c
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

#ifdef __MATH_FORCE_EXPAND_POLY__
#ifdef __STDC__
static const float 
#else
static float 
#endif
//one =  1.0000000000e+00, /* 0x3f800000 */
C1  =  4.1666667908e-02, /* 0x3d2aaaab */
C2  = -1.3888889225e-03, /* 0xbab60b61 */
C3  =  2.4801587642e-05, /* 0x37d00d01 */
C4  = -2.7557314297e-07, /* 0xb493f27c */
C5  =  2.0875723372e-09, /* 0x310f74f6 */
C6  = -1.1359647598e-11; /* 0xad47d74e */
#else
static const float C[] = {
 4.1666667908e-02, /* C1  = 0x3d2aaaab */
-1.3888889225e-03, /* C2  = 0xbab60b61 */
 2.4801587642e-05, /* C3  = 0x37d00d01 */
-2.7557314297e-07, /* C4  = 0xb493f27c */
 2.0875723372e-09  /* C5  = 0x310f74f6 */
},
C6  = -1.1359647598e-11; /* 0xad47d74e */

static float poly(float w, const float cf[], float retval)
{
	int i=4;
	while (i >= 0)
	    retval=w*retval+cf[i--];
	return retval;
}
#endif /* __MATH_FORCE_EXPAND_POLY__ */


#ifdef __STDC__
	float __kernel_cosf(float x, float y)
#else
	float __kernel_cosf(x, y)
	float x,y;
#endif
{
	float a,hz,z,r,qx,half,one;
	__int32_t ix;
	GET_FLOAT_WORD(ix,x);
	ix &= 0x7fffffff;			/* ix = |x|'s high word*/

#ifdef __MATH_SOFT_FLOAT__
	SET_FLOAT_WORD(one,0x3f800000);
#else /* __MATH_SOFT_FLOAT__ */
	__inst_ldi_S_H(one,0x3f80);
#endif /* __MATH_SOFT_FLOAT__ */

	if(ix<0x32000000) {			/* if x < 2**27 */
#ifdef __MATH_MATCH_NEWLIB_EXCEPTION__
	/* Not needed for sinf/cosf as per ISO standard */
	    if((int)x==0)	/* generate inexact */
#endif /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
		return one;		/* generate inexact */
	}
	z  = x*x;
#ifdef __MATH_FORCE_EXPAND_POLY__
	r  = z*(C1+z*(C2+z*(C3+z*(C4+z*(C5+z*C6)))));
#else  /* __MATH_FORCE_EXPAND_POLY__ */
	r  = z*poly(z,C,C6);
#endif /* __MATH_FORCE_EXPAND_POLY__ */

#ifdef __MATH_SOFT_FLOAT__
	SET_FLOAT_WORD(half,0x3f000000);
#else /* __MATH_SOFT_FLOAT__ */
	__inst_ldi_S_H(half,0x3f00);
#endif /* __MATH_SOFT_FLOAT__ */

	if(ix < 0x3e99999a) { 			/* if |x| < 0.3 */ 
	    qx=0;
	}
	else {
	    if(ix > 0x3f480000) {		/* x > 0.78125 */
		qx = (float)0.28125;
	    } else {
	        SET_FLOAT_WORD(qx,ix-0x01000000);	/* x/4 */
	    }
	}
	hz = half*z-qx;
	a  = one-qx;
	return a - (hz - (z*r-x*y));
}
