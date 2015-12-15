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
*              file : $RCSfile: e_exp.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

/*
 * ====================================================
 * Copyright (C) 2004 by Sun Microsystems, Inc. All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

/* __ieee754_exp(x)
 * Returns the exponential of x.
 *
 * Method
 *   1. Argument reduction:
 *      Reduce x to an r so that |r| <= 0.5*ln2 ~ 0.34658.
 *	Given x, find r and integer k such that
 *
 *               x = k*ln2 + r,  |r| <= 0.5*ln2.  
 *
 *      Here r will be represented as r = hi-lo for better 
 *	accuracy.
 *
 *   2. Approximation of exp(r) by a special rational function on
 *	the interval [0,0.34658]:
 *	Write
 *	    R(r**2) = r*(exp(r)+1)/(exp(r)-1) = 2 + r*r/6 - r**4/360 + ...
 *      We use a special Reme algorithm on [0,0.34658] to generate 
 * 	a polynomial of degree 5 to approximate R. The maximum error 
 *	of this polynomial approximation is bounded by 2**-59. In
 *	other words,
 *	    R(z) ~ 2.0 + P1*z + P2*z**2 + P3*z**3 + P4*z**4 + P5*z**5
 *  	(where z=r*r, and the values of P1 to P5 are listed below)
 *	and
 *	    |                  5          |     -59
 *	    | 2.0+P1*z+...+P5*z   -  R(z) | <= 2 
 *	    |                             |
 *	The computation of exp(r) thus becomes
 *		               2*r
 *		exp(r) = 1 + -------
 *		              R - r
 *                       	   r*R1(r)	
 *		       = 1 + r + ----------- (for better accuracy)
 *		                  2 - R1(r)
 *	where
 *			         2       4             10
 *		R1(r) = r - (P1*r  + P2*r  + ... + P5*r   ).
 *	
 *   3. Scale back to obtain exp(x):
 *	From step 1, we have
 *	   exp(x) = 2^k * exp(r)
 *
 * Special cases:
 *	exp(INF) is INF, exp(NaN) is NaN;
 *	exp(-INF) is 0, and
 *	for finite argument, only exp(0)=1 is exact.
 *
 * Accuracy:
 *	according to an error analysis, the error is always less than
 *	1 ulp (unit in the last place).
 *
 * Misc. info.
 *	For IEEE double 
 *	    if x >  7.09782712893383973096e+02 then exp(x) overflow
 *	    if x < -7.45133219101941108420e+02 then exp(x) underflow
 *
 * Constants:
 * The hexadecimal values are the intended ones for the following 
 * constants. The decimal values may be used, provided that the 
 * compiler will convert from decimal to binary accurately enough
 * to produce the hexadecimal values shown.
 */

#include "low/_math.h"
#include "low/_gpinst.h"
#include "low/_fpuinst.h"

#ifndef _DOUBLE_IS_32BITS

#ifdef __STDC__
static const double
#else
static double
#endif
#ifdef __MATH_CONST_FROM_MEMORY__
halF[2]	= {0.5,-0.5,},
#endif /* __MATH_CONST_FROM_MEMORY__ */
huge	= 1.0e+300,
twom1000= 9.33263618503218878990e-302,     /* 2**-1000=0x01700000,0*/
o_threshold=  7.09782712893383973096e+02,  /* 0x40862E42, 0xFEFA39EF */
u_threshold= -7.45133219101941108420e+02,  /* 0xc0874910, 0xD52D3051 */
ln2HI   = 6.93147180369123816490e-01,  /* 0x3fe62e42, 0xfee00000 */
ln2LO   = 1.90821492927058770002e-10,  /* 0x3dea39ef, 0x35793c76 */
invln2 =  1.44269504088896338700e+00; /* 0x3ff71547, 0x652b82fe */

#ifdef __MATH_FORCE_EXPAND_POLY__
static const double
P1   =  1.66666666666666019037e-01, /* 0x3FC55555, 0x5555553E */
P2   = -2.77777777770155933842e-03, /* 0xBF66C16C, 0x16BEBD93 */
P3   =  6.61375632143793436117e-05, /* 0x3F11566A, 0xAF25DE2C */
P4   = -1.65339022054652515390e-06, /* 0xBEBBBD41, 0xC5D26BF1 */
P5   =  4.13813679705723846039e-08; /* 0x3E663769, 0x72BEA4D0 */
#else  /* __MATH_FORCE_EXPAND_POLY__ */
static const double P[] = {
 1.66666666666666019037e-01, /* P1   = 0x3FC55555, 0x5555553E */
-2.77777777770155933842e-03, /* P2   = 0xBF66C16C, 0x16BEBD93 */
 6.61375632143793436117e-05, /* P3   = 0x3F11566A, 0xAF25DE2C */
-1.65339022054652515390e-06, /* P4   = 0xBEBBBD41, 0xC5D26BF1 */
 4.13813679705723846039e-08  /* P5   = 0x3E663769, 0x72BEA4D0 */
};

static double poly(double w, const double cf[], double retval)
{
  int i=4;
  while (i >= 0)
    retval=w*retval+cf[i--];
  return w*retval;
}
#endif /* __MATH_FORCE_EXPAND_POLY__ */

#ifdef __STDC__
	double __ieee754_exp(double x)	/* default IEEE double exp */
#else
	double __ieee754_exp(x)	/* default IEEE double exp */
	double x;
#endif
{
	double y,hi,lo,c,t,two,scale,one;
	__int32_t k = 0,xsb;
	__uint32_t hx, hy;

	GET_HIGH_WORD(hx,x);
	xsb = (hx>>31)&1;		/* sign bit of x */
	hx &= 0x7fffffff;		/* high word of |x| */

#ifdef __MATH_NONFINITE__
	/* filter out non-finite argument */
        if(hx>=0x7ff00000) {
	        __uint32_t lx;
		GET_LOW_WORD(lx,x);
		if(((hx&0xfffff)|lx)!=0) 
		     return x+x; 		/* NaN */
		else return (xsb==0)? x:0.0;	/* exp(+-inf)={inf,0} */
	}
#endif
	if(x > o_threshold) return huge*huge; /* overflow */
	if(x < u_threshold) return twom1000*twom1000; /* underflow */

#ifdef __MATH_CONST_FROM_MEMORY__
	one=1.0;
#else /* __MATH_CONST_FROM_MEMORY__ */
	__inst_ldi_D_H(one, 0x3ff00000);
#endif /* __MATH_CONST_FROM_MEMORY__ */

	/* argument reduction */
	if (hx > 0x3fd62e42) {  /* if |x| > 0.5 ln2 */
		if (hx >= 0x3ff0a2b2)  /* if |x| >= 1.5 ln2 */
		{
#ifdef __MATH_CONST_FROM_MEMORY__
			k = (int)(invln2*x+halF[xsb]);
#else /* __MATH_CONST_FROM_MEMORY__ */
			double half;
			__uint32_t hbits=(0x3fe00000 | (xsb<<31));
			__inst_ldi_D_H(half,hbits);
			k = (int)(invln2*x+half);
#endif /* __MATH_CONST_FROM_MEMORY__ */
		}
		else
			k = 1-xsb-xsb;
	} 
	else if (hx > 0x3e300000)  {  /* if |x| > 2**-28 */
		k = 0;
	} else {
#ifdef __MATH_MATCH_NEWLIB_EXCEPTION__
		/* inexact if x!=0 */
		if(huge+x>one) 
#endif /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
			return one+x;
	}

	t = k;
	hi = x - t*ln2HI;  /* t*ln2HI is exact here */
	lo = t*ln2LO;
	x = hi - lo;
    	/* x is now in primary range */
	t  = x*x;
#ifdef __MATH_FORCE_EXPAND_POLY__
 	c  = x - t*(P1+t*(P2+t*(P3+t*(P4+t*P5)))); 
#else  /* __MATH_FORCE_EXPAND_POLY__ */
 	c  = x - poly(t,P,0);
#endif  /* __MATH_FORCE_EXPAND_POLY__ */

#ifdef __MATH_CONST_FROM_MEMORY__
	two=2.0;
#else /* __MATH_CONST_FROM_MEMORY__ */
	__inst_ldi_D_W(two, 2);
#endif /* __MATH_CONST_FROM_MEMORY__ */

	y = one + (x*c/(two-c) - lo + hi);

#ifdef __MATH_SPEED_OVER_SIZE__	
	if (k == 0)
		return y;
#endif /* __MATH_SPEED_OVER_SIZE__ */
	
	GET_HIGH_WORD(hy,y);

	if(k < -1021) {
		SET_HIGH_WORD(y,hy+((k+1000)<<20));	/* add k to y's exponent */
#ifdef __MATH_CONST_FROM_MEMORY__
		scale=twom1000;
#else /* __MATH_CONST_FROM_MEMORY__ */
		__inst_ldi_D_H(scale,0x01700000);
#endif /* __MATH_CONST_FROM_MEMORY__ */
	    	y=y*scale;
	} else {
	    	SET_HIGH_WORD(y,hy+(k<<20));	/* add k to y's exponent */
	}

	return y;
}


#endif /* defined(_DOUBLE_IS_32BITS) */
