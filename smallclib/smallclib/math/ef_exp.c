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
*              file : $RCSfile: ef_exp.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

/* ef_exp.c -- float version of e_exp.c.
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

#ifdef __v810__
#define const
#endif

#ifdef __STDC__
static const float
#else
static float
#endif
#ifdef __MATH_CONST_FROM_MEMORY__
halF[2]	= {0.5,-0.5,},
#endif /* __MATH_CONST_FROM_MEMORY__ */
huge	= 1.0e+30,
twom100 = 7.8886090522e-31,     /* 2**-100=0x0d800000 */
ln2HI   = 6.9313812256e-01,	/* 0x3f317180 */
ln2LO   = 9.0580006145e-06,  	/* 0x3717f7d1 */
invln2 =  1.4426950216e+00; 	/* 0x3fb8aa3b */

#ifdef __MATH_FORCE_EXPAND_POLY__
static const float
P1   =  1.6666667163e-01, /* 0x3e2aaaab */
P2   = -2.7777778450e-03, /* 0xbb360b61 */
P3   =  6.6137559770e-05, /* 0x388ab355 */
P4   = -1.6533901999e-06, /* 0xb5ddea0e */
P5   =  4.1381369442e-08; /* 0x3331bb4c */
#else /* __MATH_FORCE_EXPAND_POLY__ */
static const float P[] = {
 1.6666667163e-01, /* P1   = 0x3e2aaaab */
-2.7777778450e-03, /* P2   = 0xbb360b61 */
 6.6137559770e-05, /* P3   = 0x388ab355 */
-1.6533901999e-06, /* P4   = 0xb5ddea0e */
 4.1381369442e-08  /* P5   = 0x3331bb4c */
};

static float poly(float w, const float cf[], float retval)
{
  int i=4;
  while (i >= 0)
    retval=w*retval+cf[i--];
  return w*retval;
}
#endif /* __MATH_FORCE_EXPAND_POLY__ */

#ifdef __STDC__
	float __ieee754_expf(float x)	/* default IEEE double exp */
#else
	float __ieee754_expf(x)	/* default IEEE double exp */
	float x;
#endif
{
	float y,hi,lo,c,t,two,scale,one;
	__int32_t k = 0,xsb,sx;
	__uint32_t hx;

	GET_FLOAT_WORD(sx,x);
	xsb = (sx>>31)&1;		/* sign bit of x */
	hx = sx & 0x7fffffff;		/* high word of |x| */

#ifdef __MATH_NONFINITE__
	/* filter out non-finite argument */
        if(FLT_UWORD_IS_NAN(hx))
            return x+x;	 	/* NaN */
        if(FLT_UWORD_IS_INFINITE(hx))
	    return (xsb==0)? x:0.0;		/* exp(+-inf)={inf,0} */
#endif
	if(sx > FLT_UWORD_LOG_MAX)
	    return huge*huge; /* overflow */
	if(sx < 0 && hx > FLT_UWORD_LOG_MIN)
	    return twom100*twom100; /* underflow */
	
#ifdef __MATH_CONST_FROM_MEMORY__
	one=1.0f;
#else /* __MATH_CONST_FROM_MEMORY__ */
	__inst_ldi_S_H(one, 0x3f80);
#endif /* __MATH_CONST_FROM_MEMORY__ */

	/* argument reduction */
	if (hx > 0x3eb17218) {  /* if |x| > 0.5 ln2 */
		if (hx >= 0x3f851592)  {/* if |x| >= 1.5 ln2 */
#ifdef __MATH_CONST_FROM_MEMORY__
			k = invln2*x + halF[xsb];
#else /* __MATH_CONST_FROM_MEMORY__ */
			float half;
			__uint32_t hbits=(0x3f000000 | (xsb<<31));
			SET_FLOAT_WORD(half,hbits);
			k = (int)(invln2*x+half);
#endif /* __MATH_CONST_FROM_MEMORY__ */
		}
		else
			k = 1 - xsb - xsb;
	} else if (hx >= 0x31800000) {  /* |x| >= 2**-28 */
		k = 0;
	} else {
#ifdef __MATH_MATCH_NEWLIB_EXCEPTION__
		/* inexact if x!=0 */
		if(huge+x>one) 
#endif /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
			return one+x;
	}

	t = k;
	hi = x - t*ln2HI;  /* k*ln2hi is exact here */
	lo = t*ln2LO;
	x = hi - lo;

	/* x is now in primary range */
	t  = x*x;
#ifdef __MATH_FORCE_EXPAND_POLY__
	c  = x - t*(P1+t*(P2+t*(P3+t*(P4+t*P5))));
#else  /* __MATH_FORCE_EXPAND_POLY__ */
	c  = x - poly(t,P,0);
#endif /* __MATH_FORCE_EXPAND_POLY__ */

#ifdef __MATH_CONST_FROM_MEMORY__
	two=2.0f;
#else /* __MATH_CONST_FROM_MEMORY__ */
	__inst_ldi_S_H(two, 0x4000);
#endif /* __MATH_CONST_FROM_MEMORY__ */

	y = one-((lo-(x*c)/((float)two-c))-hi);

	if(k >= -125) {
		__uint32_t hy;
	        GET_FLOAT_WORD(hy,y);
		SET_FLOAT_WORD(y,hy+(k<<23));	/* add k to y's exponent */
	} else {
		__uint32_t hy;
	        GET_FLOAT_WORD(hy,y);
		SET_FLOAT_WORD(y,hy+((k+100)<<23));	/* add k to y's exponent */
#ifdef __MATH_CONST_FROM_MEMORY__
		scale=twom100;
#else /* __MATH_CONST_FROM_MEMORY__ */
		__inst_ldi_S_H(scale,0x0d80);
#endif /* __MATH_CONST_FROM_MEMORY__ */
		y=y*scale;
	}
	return y;
}
