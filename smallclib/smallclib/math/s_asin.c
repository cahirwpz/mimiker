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
* 		  file : $RCSfile: s_asin.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* From Newlib 2.0, merged wrapper, factored out common polynomial step */

/* @(#)e_asin.c 5.1 93/09/24 */
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

/* __ieee754_asin(x)
 * Method :                  
 *	Since  asin(x) = x + x^3/6 + x^5*3/40 + x^7*15/336 + ...
 *	we approximate asin(x) on [0,0.5] by
 *		asin(x) = x + x*x^2*R(x^2)
 *	where
 *		R(x^2) is a rational approximation of (asin(x)-x)/x^3 
 *	and its remez error is bounded by
 *		|(asin(x)-x)/x^3 - R(x^2)| < 2^(-58.75)
 *
 *	For x in [0.5,1]
 *		asin(x) = pi/2-2*asin(sqrt((1-x)/2))
 *	Let y = (1-x), z = y/2, s := sqrt(z), and pio2_hi+pio2_lo=pi/2;
 *	then for x>0.98
 *		asin(x) = pi/2 - 2*(s+s*z*R(z))
 *			= pio2_hi - (2*(s+s*z*R(z)) - pio2_lo)
 *	For x<=0.98, let pio4_hi = pio2_hi/2, then
 *		f = hi part of s;
 *		c = sqrt(z) - f = (z-f*f)/(s+f) 	...f+c=sqrt(z)
 *	and
 *		asin(x) = pi/2 - 2*(s+s*z*R(z))
 *			= pio4_hi+(pio4-2s)-(2s*z*R(z)-pio2_lo)
 *			= pio4_hi+(pio4-2f)-(2s*z*R(z)-(pio2_lo+2c))
 *
 * Special cases:
 *	if x is NaN, return x itself;
 *	if |x|>1, return NaN with invalid signal.
 *
 */

#include "low/_math.h"
#include "low/_fpuinst.h"
#include <errno.h>

extern const double pi, pio2_hi, pio2_lo, pio4_hi, huge;
extern const double coeff[];

#ifndef _DOUBLE_IS_32BITS
#ifdef __STDC__
	double asin(double x)
#else
	double asin(x)
	double x;
#endif
{
        double t,w,p,q,c,r,s,tmp,one,two=2.0;
	__int32_t hx,ix,lx;
	EXTRACT_WORDS(hx,lx,x);
	ix = hx&0x7fffffff;
	
#ifdef __MATH_CONST_FROM_MEMORY__
	one=1.0;
#else /* __MATH_CONST_FROM_MEMORY__ */
	__inst_ldi_D_W(one,1);
#endif /*__MATH_CONST_FROM_MEMORY__ */

	if(ix>= 0x3ff00000) {		/* |x|>= 1 */
	  if(((ix-0x3ff00000)|lx)==0)
#ifdef __MATH_MATCH_NEWLIB_EXCEPTION__
	    /* asin(1)=+-pi/2 with inexact */
	      return x*pio2_hi+x*pio2_lo;	
#else /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
	    return x*pio2_hi;
#endif /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
	    if ((ix<0x7ff00000) ||
	    	((ix==0x7ff00000) && (lx==0)))
	    	errno=EDOM;
	    /* asin(|x|>1) is NaN with Invalid Op exception as required */
	   return (x-x)/(x-x);
	} else if (ix<0x3fe00000) {	/* |x|<0.5 */
	    if(ix<0x3e400000) {		/* if |x| < 2**-27 */
#ifdef __MATH_MATCH_NEWLIB_EXCEPTION__
		if(huge+x>one) 
#endif /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
		  return x;/* return x with inexact if x!=0*/
          } else {
		t = x*x;
	        w = ___poly2 (t, 6+1, 6+4, coeff, 1);
		return x+x*w;
          }
	  }
	/* 1> |x|>= 0.5 */
#ifdef __MATH_SOFT_FLOAT__
	x=fabs(x);
#else /* __MATH_SOFT_FLOAT__ */
	__inst_abs_D(x,x);
#endif /* __MATH_SOFT_FLOAT__ */
	w = one-x;
	t = w*0.5;
	tmp = ___poly2 (t, 6+1, 6+4,  coeff, 1);
#ifdef __MATH_SOFT_FLOAT__
	s = __ieee754_sqrt(t);
#else /* __MATH_SOFT_FLOAT__ */
	__inst_sqrt_D(s,t);
#endif /* __MATH_SOFT_FLOAT__ */

	w  = s;
#ifdef __MATH_SOFT_FLOAT__
	SET_LOW_WORD(w,0);
#else /* __MATH_SOFT_FLOAT__ */
	__inst_reset_low_D (w)
#endif /* __MATH_SOFT_FLOAT__ */
		c  = (t-w*w)/(s+w);
	r  = tmp;
	p  = two*s*r-(pio2_lo-two*c);
	q  = pio4_hi-two*w;
	t  = pio4_hi-(p-q);

#ifdef __MATH_SOFT_FLOAT__
	if (hx<0) t=-t;
#else /* __MATH_SOFT_FLOAT__ */
	__inst_copysign_Dx (t,hx);
#endif /* __MATH_SOFT_FLOAT__ */
	return t;

	}
#endif /* defined(_DOUBLE_IS_32BITS) */
