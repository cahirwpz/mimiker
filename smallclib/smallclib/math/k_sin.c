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
* 		  file : $RCSfile: k_sin.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* From Newlib 2.0, make inexact exception optional */

/* @(#)k_sin.c 5.1 93/09/24 */
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

/* __kernel_sin( x, y, iy)
 * kernel sin function on [-pi/4, pi/4], pi/4 ~ 0.7854
 * Input x is assumed to be bounded by ~pi/4 in magnitude.
 * Input y is the tail of x.
 * Input iy indicates whether y is 0. (if iy=0, y assume to be 0). 
 *
 * Algorithm
 *	1. Since sin(-x) = -sin(x), we need only to consider positive x. 
 *	2. if x < 2^-27 (hx<0x3e400000 0), return x with inexact if x!=0.
 *	3. sin(x) is approximated by a polynomial of degree 13 on
 *	   [0,pi/4]
 *		  	         3            13
 *	   	sin(x) ~ x + S1*x + ... + S6*x
 *	   where
 *	
 * 	|sin(x)         2     4     6     8     10     12  |     -58
 * 	|----- - (1+S1*x +S2*x +S3*x +S4*x +S5*x  +S6*x   )| <= 2
 * 	|  x 					           | 
 * 
 *	4. sin(x+y) = sin(x) + sin'(x')*y
 *		    ~ sin(x) + (1-x*x/2)*y
 *	   For better accuracy, let 
 *		     3      2      2      2      2
 *		r = x *(S2+x *(S3+x *(S4+x *(S5+x *S6))))
 *	   then                   3    2
 *		sin(x) = x + (S1*x + (x *(r-y/2)+y))
 */

#include "low/_math.h"
#include "low/_fpuinst.h"


#ifndef _DOUBLE_IS_32BITS

#ifdef __MATH_FORCE_EXPAND_POLY__
#ifdef __STDC__
static const double 
#else
static double 
#endif
S1  = -1.66666666666666324348e-01, /* 0xBFC55555, 0x55555549 */
S2  =  8.33333333332248946124e-03, /* 0x3F811111, 0x1110F8A6 */
S3  = -1.98412698298579493134e-04, /* 0xBF2A01A0, 0x19C161D5 */
S4  =  2.75573137070700676789e-06, /* 0x3EC71DE3, 0x57B1FE7D */
S5  = -2.50507602534068634195e-08, /* 0xBE5AE5E6, 0x8A2B9CEB */
S6  =  1.58969099521155010221e-10; /* 0x3DE5D93A, 0x5ACFD57C */
#else /* __MATH_FORCE_EXPAND_POLY__ */
static const double S[] = {
 8.33333333332248946124e-03, /* S2  = 0x3F811111, 0x1110F8A6 */
-1.98412698298579493134e-04, /* S3  = 0xBF2A01A0, 0x19C161D5 */
 2.75573137070700676789e-06, /* S4  = 0x3EC71DE3, 0x57B1FE7D */
-2.50507602534068634195e-08, /* S5  = 0xBE5AE5E6, 0x8A2B9CEB */
 1.58969099521155010221e-10  /* S6  = 0x3DE5D93A, 0x5ACFD57C */
},
S1 = -1.66666666666666324348e-01; /* 0xBFC55555, 0x55555549 */

static double poly(double w, const double cf[], double retval)
{
	int i=4;
	while (i >= 0)
	    retval=w*retval+cf[i--];
	return retval;
}
#endif /* __MATH_FORCE_EXPAND_POLY__ */

#ifdef __STDC__
	double __kernel_sin(double x, double y, int iy)
#else
	double __kernel_sin(x, y, iy)
	double x,y; int iy;		/* iy=0 if y is zero */
#endif
{
	double z,r,v;
	__int32_t ix;
	GET_HIGH_WORD(ix,x);
	ix &= 0x7fffffff;			/* high word of x */
	if(ix<0x3e400000) {			/* |x| < 2**-27 */
#ifdef __MATH_MATCH_NEWLIB_EXCEPTION__
	/* Not needed for sin/cos as per ISO standard */
	   if((int)x==0)	/* generate inexact */
#endif /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
	       return x;
	}
	z	=  x*x;
	v	=  z*x;
#ifdef __MATH_FORCE_EXPAND_POLY__
	r	=  S2+z*(S3+z*(S4+z*(S5+z*S6)));
#else  /* __MATH_FORCE_EXPAND_POLY__ */
	r 	= poly(z,S,0);
#endif /* __MATH_FORCE_EXPAND_POLY__ */

	if(iy==0) return x+v*(S1+z*r);
	else      return x-((z*(0.5*y-v*r)-y)-v*S1);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
