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
*              file : $RCSfile: s_erf.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

/* @(#)s_erf.c 5.1 93/09/24 */
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
FUNCTION
        <<erf>>, <<erff>>, <<erfc>>, <<erfcf>>---error function 
INDEX
	erf
INDEX
	erff
INDEX
	erfc
INDEX
	erfcf

ANSI_SYNOPSIS
	#include <math.h>
	double erf(double <[x]>);
	float erff(float <[x]>);
	double erfc(double <[x]>);
	float erfcf(float <[x]>);
TRAD_SYNOPSIS
	#include <math.h>

	double erf(<[x]>)
	double <[x]>;

	float erff(<[x]>)
	float <[x]>;

	double erfc(<[x]>)
	double <[x]>;

	float erfcf(<[x]>)
	float <[x]>;

DESCRIPTION
	<<erf>> calculates an approximation to the ``error function'',
	which estimates the probability that an observation will fall within
	<[x]> standard deviations of the mean (assuming a normal
	distribution).
	@tex
	The error function is defined as
	$${2\over\sqrt\pi}\times\int_0^x e^{-t^2}dt$$
	 @end tex

	<<erfc>> calculates the complementary probability; that is,
	<<erfc(<[x]>)>> is <<1 - erf(<[x]>)>>.  <<erfc>> is computed directly,
	so that you can use it to avoid the loss of precision that would
	result from subtracting large probabilities (on large <[x]>) from 1.

	<<erff>> and <<erfcf>> differ from <<erf>> and <<erfc>> only in the
	argument and result types.

RETURNS
	For positive arguments, <<erf>> and all its variants return a
	probability---a number between 0 and 1.

PORTABILITY
	None of the variants of <<erf>> are ANSI C.
*/

/* double erf(double x)
 * double erfc(double x)
 *			     x
 *		      2      |\
 *     erf(x)  =  ---------  | exp(-t*t)dt
 *	 	   sqrt(pi) \| 
 *			     0
 *
 *     erfc(x) =  1-erf(x)
 *  Note that 
 *		erf(-x) = -erf(x)
 *		erfc(-x) = 2 - erfc(x)
 *
 * Method:
 *	1. For |x| in [0, 0.84375]
 *	    erf(x)  = x + x*R(x^2)
 *          erfc(x) = 1 - erf(x)           if x in [-.84375,0.25]
 *                  = 0.5 + ((0.5-x)-x*R)  if x in [0.25,0.84375]
 *	   where R = P/Q where P is an odd poly of degree 8 and
 *	   Q is an odd poly of degree 10.
 *						 -57.90
 *			| R - (erf(x)-x)/x | <= 2
 *	
 *
 *	   Remark. The formula is derived by noting
 *          erf(x) = (2/sqrt(pi))*(x - x^3/3 + x^5/10 - x^7/42 + ....)
 *	   and that
 *          2/sqrt(pi) = 1.128379167095512573896158903121545171688
 *	   is close to one. The interval is chosen because the fix
 *	   point of erf(x) is near 0.6174 (i.e., erf(x)=x when x is
 *	   near 0.6174), and by some experiment, 0.84375 is chosen to
 * 	   guarantee the error is less than one ulp for erf.
 *
 *      2. For |x| in [0.84375,1.25], let s = |x| - 1, and
 *         c = 0.84506291151 rounded to single (24 bits)
 *         	erf(x)  = sign(x) * (c  + P1(s)/Q1(s))
 *         	erfc(x) = (1-c)  - P1(s)/Q1(s) if x > 0
 *			  1+(c+P1(s)/Q1(s))    if x < 0
 *         	|P1/Q1 - (erf(|x|)-c)| <= 2**-59.06
 *	   Remark: here we use the taylor series expansion at x=1.
 *		erf(1+s) = erf(1) + s*Poly(s)
 *			 = 0.845.. + P1(s)/Q1(s)
 *	   That is, we use rational approximation to approximate
 *			erf(1+s) - (c = (single)0.84506291151)
 *	   Note that |P1/Q1|< 0.078 for x in [0.84375,1.25]
 *	   where 
 *		P1(s) = degree 6 poly in s
 *		Q1(s) = degree 6 poly in s
 *
 *      3. For x in [1.25,1/0.35(~2.857143)], 
 *         	erfc(x) = (1/x)*exp(-x*x-0.5625+R1/S1)
 *         	erf(x)  = 1 - erfc(x)
 *	   where 
 *		R1(z) = degree 7 poly in z, (z=1/x^2)
 *		S1(z) = degree 8 poly in z
 *
 *      4. For x in [1/0.35,28]
 *         	erfc(x) = (1/x)*exp(-x*x-0.5625+R2/S2) if x > 0
 *			= 2.0 - (1/x)*exp(-x*x-0.5625+R2/S2) if -6<x<0
 *			= 2.0 - tiny		(if x <= -6)
 *         	erf(x)  = sign(x)*(1.0 - erfc(x)) if x < 6, else
 *         	erf(x)  = sign(x)*(1.0 - tiny)
 *	   where
 *		R2(z) = degree 6 poly in z, (z=1/x^2)
 *		S2(z) = degree 7 poly in z
 *
 *      Note1:
 *	   To compute exp(-x*x-0.5625+R/S), let s be a single
 *	   precision number and s := x; then
 *		-x*x = -s*s + (s-x)*(s+x)
 *	        exp(-x*x-0.5626+R/S) = 
 *			exp(-s*s-0.5625)*exp((s-x)*(s+x)+R/S);
 *      Note2:
 *	   Here 4 and 5 make use of the asymptotic series
 *			  exp(-x*x)
 *		erfc(x) ~ ---------- * ( 1 + Poly(1/x^2) )
 *			  x*sqrt(pi)
 *	   We use rational approximation to approximate
 *      	g(s)=f(1/x^2) = log(erfc(x)*x) - x*x + 0.5625
 *	   Here is the error bound for R1/S1 and R2/S2
 *      	|R1/S1 - f(x)|  < 2**(-62.57)
 *      	|R2/S2 - f(x)|  < 2**(-61.52)
 *
 *      5. For inf > x >= 28
 *         	erf(x)  = sign(x) *(1 - tiny)  (raise inexact)
 *         	erfc(x) = tiny*tiny (raise underflow) if x > 0
 *			= 2 - tiny if x<0
 *
 *      7. Special case:
 *         	erf(0)  = 0, erf(inf)  = 1, erf(-inf) = -1,
 *         	erfc(0) = 1, erfc(inf) = 0, erfc(-inf) = 2, 
 *	   	erfc/erf(NaN) is NaN
 */


#include "low/_math.h"

static const double
  tiny	    = 1e-300,
  one =  1.00000000000000000000e+00, /* 0x3FF00000, 0x00000000 */
  
  /* c = (float)0.84506291151 */
  erx =  8.45062911510467529297e-01, /* 0x3FEB0AC1, 0x60000000 */

  /*
   * Coefficients for approximation to  erf on [0,0.84375]
  */
  efx =  1.28379167095512586316e-01, /* 0x3FC06EBA, 0x8214DB69 */
  efx8=  1.02703333676410069053e+00; /* 0x3FF06EBA, 0x8214DB69 */

/* pp0-pp4, qq1- qq5 */
extern const double __coeff1[];

/* pa0 - pa6 ,qa1-qa6 */
extern const double __coeff2[];

/* ra0-ra7, sa1-sa8 */
extern const double __coeff3[];

/* rb0-rb6, sb1-sb7 */
extern const double __coeff4[];

double erf(double x)
{
  __int32_t hx,ix;
  double s,y,z,r;

  GET_HIGH_WORD(hx,x);
  ix = hx&0x7fffffff;
#ifdef __MATH_NONFINITE__
  if(ix>=0x7ff00000)
    { /* erf(nan)=nan */
      __int32_t i;
      i = ((__uint32_t)hx>>31)<<1;
      return (double)(1-i)+one/x;	/* erf(+-inf)=+-1 */
    }
#endif /* __MATH_NONFINITE__ */

  if(ix < 0x3feb0000)
    {		/* |x|<0.84375 */
      if(ix < 0x3e300000)
        { 	/* |x|<2**-28 */
          if (ix < 0x00800000)
	    return 0.125*(8.0*x+efx8*x);  /*avoid underflow */
          return x + efx*x;
        }

      z = x*x;
      y = ___poly2 (z,5+1,5+5,__coeff1,0);
       return x + x*y;
    }

  if(ix < 0x3ff40000)
    {		/* 0.84375 <= |x| < 1.25 */
      s = fabs(x)-one;
      s = ___poly2 (s,7+1,7+6,__coeff2, 0);
      if(hx>=0)
        return erx + s;
      else
        return -erx - s;
    }

  if (ix >= 0x40180000)
    {		/* inf>|x|>=6 */
      if(hx>=0)
        return one-tiny;
      else
        return tiny-one;
    }

  x = fabs(x);
  s = one/(x*x);

  if(ix< 0x4006DB6E)
    { /* |x| < 1/0.35 */
      s = ___poly2 (s,8+1,8+8,__coeff3, 0);
    }
  else
    {	/* |x| >= 1/0.35 */
      s = ___poly2 (s,7+1,7+7,__coeff4, 0);
    }

  z  = x;
  SET_LOW_WORD(z,0);
  r  =  __ieee754_exp(-z*z-0.5625)*__ieee754_exp((z-x)*(z+x)+ s);

  if(hx>=0)
    return one-r/x;
  else
    return  r/x-one;
}

