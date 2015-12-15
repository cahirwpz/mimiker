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
* 		  file : $RCSfile: kf_sin.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* From Newlib 2.0, make inexact exception optional */

/* kf_sin.c -- float version of k_sin.c
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
S1  = -1.6666667163e-01, /* 0xbe2aaaab */
S2  =  8.3333337680e-03, /* 0x3c088889 */
S3  = -1.9841270114e-04, /* 0xb9500d01 */
S4  =  2.7557314297e-06, /* 0x3638ef1b */
S5  = -2.5050759689e-08, /* 0xb2d72f34 */
S6  =  1.5896910177e-10; /* 0x2f2ec9d3 */
#else /* __MATH_FORCE_EXPAND_POLY__ */
static const float S[] = {
 8.3333337680e-03, /* S2  = 0x3c088889 */
-1.9841270114e-04, /* S3  = 0xb9500d01 */
 2.7557314297e-06, /* S4  = 0x3638ef1b */
-2.5050759689e-08, /* S5  = 0xb2d72f34 */
 1.5896910177e-10  /* S6  = 0x2f2ec9d3 */
},
S1 = -1.6666667163e-01; /* S1  = 0xbe2aaaab */

static float poly(float w, const float cf[], float retval)
{
	int i=4;
	while (i >= 0)
	  retval=w*retval+cf[i--];
	return retval;
}
#endif /* __MATH_FORCE_EXPAND_POLY__ */

#ifdef __STDC__
	float __kernel_sinf(float x, float y, int iy)
#else
	float __kernel_sinf(x, y, iy)
	float x,y; int iy;		/* iy=0 if y is zero */
#endif
{
	float z,r,v,half;
	__int32_t ix;
	GET_FLOAT_WORD(ix,x);
	ix &= 0x7fffffff;			/* high word of x */

	if(ix<0x32000000) {			/* |x| < 2**-27 */
#ifdef __MATH_MATCH_NEWLIB_EXCEPTION__
	/* Not needed for sinf/cosf as per ISO standard */
	   if((int)x==0)	/* generate inexact */
#endif /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
		   return x;
	}

#ifdef __MATH_SOFT_FLOAT__
	SET_FLOAT_WORD(half,0x3f000000);
#else /* __MATH_SOFT_FLOAT__ */
	__inst_ldi_S_H(half,0x3f00);
#endif /* __MATH_SOFT_FLOAT__ */

	z	=  x*x;
	v	=  z*x;
#ifdef __MATH_FORCE_EXPAND_POLY__
	r	=  S2+z*(S3+z*(S4+z*(S5+z*S6)));
#else  /* __MATH_FORCE_EXPAND_POLY__ */
	r 	= poly(z,S,0);
#endif /* __MATH_FORCE_EXPAND_POLY__ */
	if(iy==0) return x+v*(S1+z*r);
	else      return x-((z*(half*y-v*r)-y)-v*S1);
}
