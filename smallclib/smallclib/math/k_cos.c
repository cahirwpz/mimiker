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
* 		  file : $RCSfile: k_cos.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* From Newlib 2.0, make inexact exception optional */

/* @(#)k_cos.c 5.1 93/09/24 */
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

/*
 * __kernel_cos( x,  y )
 * kernel cos function on [-pi/4, pi/4], pi/4 ~ 0.785398164
 * Input x is assumed to be bounded by ~pi/4 in magnitude.
 * Input y is the tail of x. 
 *
 * Algorithm
 *	1. Since cos(-x) = cos(x), we need only to consider positive x.
 *	2. if x < 2^-27 (hx<0x3e400000 0), return 1 with inexact if x!=0.
 *	3. cos(x) is approximated by a polynomial of degree 14 on
 *	   [0,pi/4]
 *		  	                 4            14
 *	   	cos(x) ~ 1 - x*x/2 + C1*x + ... + C6*x
 *	   where the remez error is
 *	
 * 	|              2     4     6     8     10    12     14 |     -58
 * 	|cos(x)-(1-.5*x +C1*x +C2*x +C3*x +C4*x +C5*x  +C6*x  )| <= 2
 * 	|    					               | 
 * 
 * 	               4     6     8     10    12     14 
 *	4. let r = C1*x +C2*x +C3*x +C4*x +C5*x  +C6*x  , then
 *	       cos(x) = 1 - x*x/2 + r
 *	   since cos(x+y) ~ cos(x) - sin(x)*y 
 *			  ~ cos(x) - x*y,
 *	   a correction term is necessary in cos(x) and hence
 *		cos(x+y) = 1 - (x*x/2 - (r - x*y))
 *	   For better accuracy when x > 0.3, let qx = |x|/4 with
 *	   the last 32 bits mask off, and if x > 0.78125, let qx = 0.28125.
 *	   Then
 *		cos(x+y) = (1-qx) - ((x*x/2-qx) - (r-x*y)).
 *	   Note that 1-qx and (x*x/2-qx) is EXACT here, and the
 *	   magnitude of the latter is at least a quarter of x*x/2,
 *	   thus, reducing the rounding error in the subtraction.
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
C1  =  4.16666666666666019037e-02, /* 0x3FA55555, 0x5555554C */
C2  = -1.38888888888741095749e-03, /* 0xBF56C16C, 0x16C15177 */
C3  =  2.48015872894767294178e-05, /* 0x3EFA01A0, 0x19CB1590 */
C4  = -2.75573143513906633035e-07, /* 0xBE927E4F, 0x809C52AD */
C5  =  2.08757232129817482790e-09, /* 0x3E21EE9E, 0xBDB4B1C4 */
C6  = -1.13596475577881948265e-11; /* 0xBDA8FAE9, 0xBE8838D4 */
#else  /* __MATH_FORCE_EXPAND_POLY__ */
static const double C[] = {
 4.16666666666666019037e-02, /* C1  = 0x3FA55555, 0x5555554C */
-1.38888888888741095749e-03, /* C2  = 0xBF56C16C, 0x16C15177 */
 2.48015872894767294178e-05, /* C3  = 0x3EFA01A0, 0x19CB1590 */
-2.75573143513906633035e-07, /* C4  = 0xBE927E4F, 0x809C52AD */
 2.08757232129817482790e-09, /* C5  = 0x3E21EE9E, 0xBDB4B1C4 */
},
C6  = -1.13596475577881948265e-11; /* 0xBDA8FAE9, 0xBE8838D4 */

static double poly(double w, const double cf[], double retval)
{
	int i=4;
	while (i >= 0)
	  retval=w*retval+cf[i--];
	return retval;
}
#endif /* __MATH_FORCE_EXPAND_POLY__ */

#ifdef __STDC__
	double __kernel_cos(double x, double y)
#else
	double __kernel_cos(x, y)
	double x,y;
#endif
{
	double a,hz,z,r,qx,one;
	__int32_t ix;
	GET_HIGH_WORD(ix,x);

#ifdef __MATH_SOFT_FLOAT__
	INSERT_WORDS(one,0x3ff00000,0);
#else /* __MATH_SOFT_FLOAT__ */
	__inst_ldi_D_W(one,1);
#endif /* __MATH_SOFT_FLOAT__ */

	ix &= 0x7fffffff;			/* ix = |x|'s high word*/
	if(ix<0x3e400000) {			/* if x < 2**27 */
#ifdef __MATH_MATCH_NEWLIB_EXCEPTION__
	/* Not needed for sin/cos as per ISO standard */
	   if((int)x==0)	/* generate inexact */
#endif /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
	       return one;		/* generate inexact */
	}
	z  = x*x;

#ifdef __MATH_FORCE_EXPAND_POLY__
	r  = z*(C1+z*(C2+z*(C3+z*(C4+z*(C5+z*C6)))));
#else  /* __MATH_FORCE_EXPAND_POLY__ */
	r = z*poly(z,C,C6);
#endif /* __MATH_FORCE_EXPAND_POLY__ */ 

	if(ix < 0x3FD33333) 			/* if |x| < 0.3 */ 
	    qx=0;
	else {
	    if(ix > 0x3fe90000) {		/* x > 0.78125 */
#ifdef __MATH_CONST_FROM_MEMORY__
		qx = 0.28125;
#else /* __MATH_CONST_FROM_MEMORY__ */
		__inst_ldi_D_H(qx,0x3fd20000);
#endif /* __MATH_CONST_FROM_MEMORY__ */
	    } else {

	        INSERT_WORDS(qx,ix-0x00200000,0);	/* x/4 */
	    }
	}

	hz = 0.5*z-qx;
	a  = one-qx;
	return a - (hz - (z*r-x*y));
}	

#endif /* defined(_DOUBLE_IS_32BITS) */
