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
* 		  file : $RCSfile: s_acos.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* From Newlib 2.0, merged wrapper, factored out common polynomial step */

/* @(#)e_acos.c 5.1 93/09/24 */
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

/* __ieee754_acos(x)
 * Method :
 *	acos(x)  = pi/2 - asin(x)
 *	acos(-x) = pi/2 + asin(x)
 * For |x|<=0.5
 *	acos(x) = pi/2 - (x + x*x^2*R(x^2))	(see asin.c)
 * For x>0.5
 * 	acos(x) = pi/2 - (pi/2 - 2asin(sqrt((1-x)/2)))
 *		= 2asin(sqrt((1-x)/2))
 *		= 2s + 2s*z*R(z) 	...z=(1-x)/2, s=sqrt(z)
 *		= 2f + (2c + 2s*z*R(z))
 *     where f=hi part of s, and c = (z-f*f)/(s+f) is the correction term
 *     for f so that f+c ~ sqrt(z).
 * For x<-0.5
 *	acos(x) = pi - 2asin(sqrt((1-|x|)/2))
 *		= pi - 0.5*(s+s*z*R(z)), where z=(1-|x|)/2,s=sqrt(z)
 *
 * Special cases:
 *	if x is NaN, return x itself;
 *	if |x|>1, return NaN with invalid signal.
 *
 * Function needed: sqrt
 */

#include "low/_math.h"
#include "low/_fpuinst.h"
#include <errno.h>

extern const double one, pi, pio2_hi, pio2_lo;
extern const double coeff[];

double acos(double x)
{
  double z,r,w,s,c,df,one,two=2.0, half;
  __int32_t hx,ix;

  GET_HIGH_WORD(hx,x);
  ix = hx&0x7fffffff;
  if(ix>=0x3ff00000)
  {	/* |x| >= 1 */
    __uint32_t lx;
    GET_LOW_WORD(lx,x);

    if(((ix-0x3ff00000)|lx)==0)
    { /* |x|==1 */
      if(hx>0)
        return 0.0;		/* acos(1) = 0  */
      else
#ifdef __MATH_MATCH_NEWLIB_EXCEPTION__
        return pi+two*pio2_lo;	/* acos(-1)= pi */
#else /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
        return pi;
#endif /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
    }
    if ((ix<0x7ff00000) ||
	((ix==0x7ff00000) && (lx==0)))
      errno=EDOM;
    return (x-x)/(x-x);		/* acos(|x|>1) is NaN */
  }

  if(ix<0x3fe00000)
  { /* |x| < 0.5 */
    if(ix<=0x3c600000)
      return pio2_hi+pio2_lo;/*if|x|<2**-57*/

    z = x*x;
    r = ___poly2(z, 6+1, 6+4, coeff, 1);
    return pio2_hi - (x - (pio2_lo-x*r));
  }

#ifdef __MATH_SOFT_FLOAT__
  x=fabs(x);
#else /* __MATH_SOFT_FLOAT__ */
  __inst_abs_D(x,x);
#endif /* __MATH_SOFT_FLOAT__ */

#ifdef __MATH_CONST_FROM_MEMORY__
  half=0.5;
  one=1.0;
#else /* __MATH_CONST_FROM_MEMORY__ */
  __inst_ldi_D_H(half,0x3fe00000);
  __inst_ldi_D_W(one,1);
#endif /* __MATH_CONST_FROM_MEMORY__ */

  z = (one-x)*half;
  r = ___poly2(z, 6+1, 6+4, coeff, 1);
#ifdef __MATH_SOFT_FLOAT__
  s = __ieee754_sqrt(z);
#else /* __MATH_SOFT_FLOAT__ */
  __inst_sqrt_D (s,z);
#endif /* __MATH_SOFT_FLOAT__ */

  if (hx<0)
  {		/* x < -0.5 */
    w = r*s-pio2_lo;
    return pi - two*(s+w);
  }
  else
  {			/* x >= 0.5 */
    df = s;
#ifdef __MATH_SOFT_FLOAT__
    SET_LOW_WORD(df,0);
#else /* __MATH_SOFT_FLOAT__ */
    __inst_reset_low_D(df);
#endif /* __MATH_SOFT_FLOAT__ */
    c  = (z-df*df)/(s+df);
    w = r*s+c;
    return two*(df+w);
  }
}

