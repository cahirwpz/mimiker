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
* 		  file : $RCSfile: s_acosh.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* From Newlib 2.0, merged wrapper */

/* @(#)e_acosh.c 5.1 93/09/24 */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice 
 * is preserved.
 * ====================================================
 *
 */

/* __ieee754_acosh(x)
 * Method :
 *	Based on 
 *		acosh(x) = log [ x + sqrt(x*x-1) ]
 *	we have
 *		acosh(x) := log(x)+ln2,	if x is large; else
 *		acosh(x) := log(2x-1/(sqrt(x*x-1)+x)) if x>2; else
 *		acosh(x) := log1p(t+sqrt(2.0*t+t*t)); where t=x-1.
 *
 * Special cases:
 *	acosh(x) is NaN with signal if x<1.
 *	acosh(NaN) is NaN without signal.
 */

#include "low/_math.h"
#include "low/_fpuinst.h"
#include <errno.h>

#define __MATH_CONST_FROM_MEMORY__

#ifndef _DOUBLE_IS_32BITS

#ifdef __STDC__
static const double 
#else
static double 
#endif
ln2	= 6.93147180559945286227e-01;  /* 0x3FE62E42, 0xFEFA39EF */

#ifdef __STDC__
	double acosh(double x)
#else
	double acosh(x)
	double x;
#endif
{	
	double t;
	double one, two;
	__int32_t hx;

#ifdef __MATH_NONFINITE__
	__uint32_t lx;
#endif /* __MATH_NONFINITE__ */

#ifdef __MATH_CONST_FROM_MEMORY__
	one=1.0; two=2.0;
#else /* __MATH_CONST_FROM_MEMORY__ */
	__inst_ldi_D_W (one,1);
	__inst_ldi_D_W (two,2);
#endif /* __MATH_CONST_FROM_MEMORY__ */

	GET_HIGH_WORD(hx,x);
#ifdef __MATH_NONFINITE__
	GET_LOW_WORD(lx,x);
	if(((hx<<1)>0xffe00000) || (((hx<<1)==0xffe00000) &&  (lx!=0))) {	/* x is NaN */
	  return x;
	} else
#endif /* __MATH_NONFINITE__ */

	if(hx<0x3ff00000) {		/* x < 1 */
	  errno=EDOM;
	  return (x-x)/(x-x);
	} else if(hx >=0x41b00000) {	/* x > 2**28 */
	  return __ieee754_log(x)+ln2;	/* acosh(huge)=log(2x) */
	} 
	else if (hx > 0x40000000) {	/* 2**28 > x > 2 */
	    t=x*x - one;
#ifdef __MATH_SOFT_FLOAT__
	    t=sqrt(t);
#else /* __MATH_SOFT_FLOAT */ 
	    __inst_sqrt_D (t,t);
#endif /* __MATH_SOFT_FLOAT */ 
	    return __ieee754_log(two*x-one/(x+t));
	} else {			/* 1<x<2 */
	  double tmp;
	  t = x-one;
	  tmp=two*t+t*t;
#ifdef __MATH_SOFT_FLOAT__
	  tmp=sqrt(tmp);
#else /* __MATH_SOFT_FLOAT */ 
	  __inst_sqrt_D (tmp,tmp);
#endif /* __MATH_SOFT_FLOAT */ 
	  return log1p(t+tmp);
	}
}

#endif /* defined(_DOUBLE_IS_32BITS) */
