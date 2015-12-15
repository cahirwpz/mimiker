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
* 		  file : $RCSfile: ef_log.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* From Newlib 2.0, remove NaN/Inf handling for tiny version
Refactored arithmetic conditions and modified constants loads */

/* ef_log.c -- float version of e_log.c.
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
ln2_hi =   6.9313812256e-01,	/* 0x3f317180 */
ln2_lo =   9.0580006145e-06,	/* 0x3717f7d1 */
Lg1 = 6.6666668653e-01,	/* 3F2AAAAB */
Lg2 = 4.0000000596e-01,	/* 3ECCCCCD */
Lg3 = 2.8571429849e-01, /* 3E924925 */
Lg4 = 2.2222198546e-01, /* 3E638E29 */
Lg5 = 1.8183572590e-01, /* 3E3A3325 */
Lg6 = 1.5313838422e-01, /* 3E1CD04F */
Lg7 = 1.4798198640e-01; /* 3E178897 */

#ifdef __STDC__
	float __ieee754_logf(float x)
#else
	float __ieee754_logf(x)
	float x;
#endif
{
	float hfsq,f,s,z,R,w,t1,t2,dk;
	__int32_t k,ix,i,j;

	GET_FLOAT_WORD(ix,x);
	k=0;

#ifdef __MATH_SUBNORMAL__
	if (FLT_UWORD_IS_SUBNORMAL(ix)) {
		float two25;
#ifdef __MATH_CONST_FROM_MEMORY__
		two25 = 3.355443200e+07;	/* 0x4c000000 */
#else /* __MATH_CONST_FROM_MEMORY__ */
		__inst_ldi_S_H (two25, 0x4c00);
#endif /* __MATH_CONST_FROM_MEMORY__ */
	    k -= 25; x *= two25; /* subnormal number, scale up x */
	    GET_FLOAT_WORD(ix,x);
	} 
#endif /* __MATH_SUBNORMAL__ */

	k += (ix>>23)-127;
	ix &= 0x007fffff;
	i = (ix+(0x95f64<<3))&0x800000;
	SET_FLOAT_WORD(x,ix|(i^0x3f800000));	/* normalize x or x/2 */
	k += (i>>23);
#ifdef __MATH_CONST_FROM_MEMORY__
	f = x-(float)1.0;
#else /* __MATH_CONST_FROM_MEMORY__ */
	__inst_ldi_S_H (f,0x3f80);
	f = x - f;
#endif /* __MATH_CONST_FROM_MEMORY__ */

#ifdef __MATH_SPEED_OVER_SIZE__
	dk=(float)k;
	if((0x007fffff&(15+ix))<16) {	/* |f| < 2**-20 */
	    R = f*f*((float)0.5-(float)0.33333333333333333*f);
	    return dk*ln2_hi-((R-dk*ln2_lo)-f);
	}
#endif /* __MATH_SPEED_OVER_SIZE__ */

#ifdef __MATH_CONST_FROM_MEMORY__
 	s = f/((float)2.0+f); 
#else /* __MATH_CONST_FROM_MEMORY__ */
	__inst_ldi_S_H (s,0x4000);
 	s = f/(s+f); 
#endif /* __MATH_CONST_FROM_MEMORY__ */
	dk = (float)k;
	z = s*s;
	i = ix-(0x6147a<<3);
	w = z*z;
	j = (0x6b851<<3)-ix;
	t1= w*(Lg2+w*(Lg4+w*Lg6)); 
	t2= z*(Lg1+w*(Lg3+w*(Lg5+w*Lg7))); 
	i |= j;
	R = t2+t1;

	z=dk*ln2_lo;
	if(i>0) {
	    float half;
#ifdef 	__MATH_CONST_FROM_MEMORY__
		half=0.5;
#else /* __MATH_CONST_FROM_MEMORY__ */
	    __inst_ldi_S_H(half,0x3f00);
#endif /* __MATH_CONST_FROM_MEMORY__ */
	    hfsq=half*f*f;
	    s=(hfsq-(s*(hfsq+R)+z));
	} else {
	    s=(s*(f-R)-z);
	}
	return dk*ln2_hi - (s-f);
}
