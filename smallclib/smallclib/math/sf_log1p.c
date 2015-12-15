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
* 		  file : $RCSfile: sf_log1p.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* From Newlib 2.0, remove NaN/Inf handling for tiny version
Modified constants loads, removed specific branch code which was
identical under IEEE754 arithmetic to the alternative branch. */

/* sf_log1p.c -- float version of s_log1p.c.
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

#include <errno.h>
#include "low/_math.h"
#include "low/_fpuinst.h"

#ifdef __STDC__
static const float
#else
static float
#endif
ln2_hi =   6.9313812256e-01,	/* 0x3f317180 */
ln2_lo =   9.0580006145e-06;	/* 0x3717f7d1 */

#ifdef __MATH_FORCE_EXPAND_POLY__
static const float
Lp1 = 6.6666668653e-01,	/* 3F2AAAAB */
Lp2 = 4.0000000596e-01,	/* 3ECCCCCD */
Lp3 = 2.8571429849e-01, /* 3E924925 */
Lp4 = 2.2222198546e-01, /* 3E638E29 */
Lp5 = 1.8183572590e-01, /* 3E3A3325 */
Lp6 = 1.5313838422e-01, /* 3E1CD04F */
Lp7 = 1.4798198640e-01; /* 3E178897 */
#else /* __MATH_FORCE_EXPAND_POLY__ */
static const float Lp[] = {
6.6666668653e-01, /* Lp1 = 3F2AAAAB */
4.0000000596e-01, /* Lp2 = 3ECCCCCD */
2.8571429849e-01, /* Lp3 = 3E924925 */
2.2222198546e-01, /* Lp4 = 3E638E29 */
1.8183572590e-01, /* Lp5 = 3E3A3325 */
1.5313838422e-01  /* Lp6 = 3E1CD04F */
},
Lp7 = 1.4798198640e-01; /* 3E178897 */

static float poly(float w, const float cf[], float retval)
{
	int i=5;
	while (i >= 0)
	    retval=w*retval+cf[i--];
	return retval;
};
#endif /* __MATH_FORCE_EXPAND_POLY__ */

#ifdef __STDC__
static const float zero = 0.0;
#else
static float zero = 0.0;
#endif

#ifdef __STDC__
	float log1pf(float x)
#else
	float log1pf(x)
	float x;
#endif
{
	float hfsq,f,c,s,z,R,u;
	__int32_t k,hx,hu,ax;
	float one,two,half;

#ifdef __MATH_CONST_FROM_MEMORY__
	one=1.0f;
	half=0.5;
#else /* __MATH_CONST_FROM_MEMORY__ */
	__inst_ldi_S_H(one,0x3f80)
	__inst_ldi_S_H(half,0x3f00)
#endif /* __MATH_CONST_FROM_MEMORY__ */

	GET_FLOAT_WORD(hx,x);
	ax = hx&0x7fffffff;

	k = 1;
#ifdef __MATH_NONFINITE__
	if (!FLT_UWORD_IS_FINITE(hx)) return x+x;
#endif /* __MATH_NONFINITE__ */
	if (hx < 0x3ed413d7) {			/* x < 0.41422  */
	    if(ax>=0x3f800000) {		/* x <= -1.0 */
#ifndef __MATH_MATCH_x86__
	/* Required as per ISO-C99, Section 7.12.6.9, even though
	   neither glibc nor newlib bother about it */
		errno=EDOM;
#endif /* __MATH_MATCH_x86__ */
		if(x==(-one))
		    /* Generate inf using -1/0 instead of -two25/zero */
		    return x/zero;  /* log1p(-1)=+inf */
		else
		    return (x-x)/(x-x);	/* log1p(x<-1)=NaN */
	    }

	    if(ax<0x31000000) {			/* |x| < 2**-29 */
#ifdef __MATH_MATCH_NEWLIB_EXCEPTION__
		    float two25;
#ifdef __MATH_CONST_FROM_MEMORY__
		    two25 =    3.355443200e+07;	/* 0x4c000000 */
#else /* __MATH_CONST_FROM_MEMORY__ */
		    __inst_ldi_S_H(two25,0x4c00);
#endif /* __MATH_CONST_FROM_MEMORY__ */
		if(two25+x>zero			/* raise inexact */
	            &&ax<0x24800000) 		/* |x| < 2**-54 */
#else /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
		if(ax<0x24800000) 		/* |x| < 2**-54 */
#endif /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
		    return x;
		else
		    return x - x*x*half;
	    }
	    if(hx>0||hx<=((__int32_t)0xbe95f61f)) {
		k=0;f=x;hu=1;}	/* -0.2929<x<0.41422 */
	}
	if(k!=0) {
	    if(hx<0x5a000000) {
		u  = one+x; 
		GET_FLOAT_WORD(hu,u);
	        k  = (hu>>23)-127;
		/* correction term */
	        c  = (k>0)? one-(u-x):x-(u-one);
		c /= u;
	    } else {
		u  = x;
		GET_FLOAT_WORD(hu,u);
	        k  = (hu>>23)-127;
		c  = 0;
	    }
	    hu &= 0x007fffff;
	    if(hu<0x3504f7) {
	        SET_FLOAT_WORD(u,hu|0x3f800000);/* normalize u */
	    } else {
	        k += 1; 
		SET_FLOAT_WORD(u,hu|0x3f000000);	/* normalize u/2 */
	        hu = (0x00800000-hu)>>2;
	    }
	    f = u-one;
	}
	else
	    c=0;
	hfsq=half*f*f;
	R = hfsq*(one-(float)0.66666666666666666*f);
	if(hu==0) {
	    /* Removed specific handling for k=0 & f=0 */
	    return k*ln2_hi-((R-(k*ln2_lo+c))-f);
	}

#ifdef __MATH_CONST_FROM_MEMORY__
	two=2.0f;
#else /* __MATH_CONST_FROM_MEMORY__ */
	__inst_ldi_S_H(two,0x4000)
#endif /* __MATH_CONST_FROM_MEMORY__ */

 	s = f/(two+f); 

	z = s*s;
#ifdef __MATH_FORCE_EXPAND_POLY__
	R = z*(Lp1+z*(Lp2+z*(Lp3+z*(Lp4+z*(Lp5+z*(Lp6+z*Lp7))))));
#else /* __MATH_FORCE_EXPAND_POLY__ */
	R = z*poly(z,Lp,Lp7);
#endif /* __MATH_FORCE_EXPAND_POLY__ */
	/* Removed specific handling for k=0 & f=0 */
	return k*ln2_hi-((hfsq-(s*(hfsq+R)+(k*ln2_lo+c)))-f);
}

#ifdef _DOUBLE_IS_32BITS

#ifdef __STDC__
	double log1p(double x)
#else
	double log1p(x)
	double x;
#endif
{
	return (double) log1pf((float) x);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
