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
* 		  file : $RCSfile: s_tanh.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/



/* @(#)s_tanh.c 5.1 93/09/24 */
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
        <<tanh>>, <<tanhf>>---hyperbolic tangent

INDEX
tanh
INDEX
tanhf

ANSI_SYNOPSIS
        #include <math.h>
        double tanh(double <[x]>);
        float tanhf(float <[x]>);

TRAD_SYNOPSIS
        #include <math.h>
        double tanh(<[x]>)
        double <[x]>;

        float tanhf(<[x]>)
        float <[x]>;


DESCRIPTION

<<tanh>> computes the hyperbolic tangent of
the argument <[x]>.  Angles are specified in radians.  

<<tanh(<[x]>)>> is defined as 
. sinh(<[x]>)/cosh(<[x]>)
	
<<tanhf>> is identical, save that it takes and returns <<float>> values.

RETURNS
The hyperbolic tangent of <[x]> is returned.

PORTABILITY
<<tanh>> is ANSI C.  <<tanhf>> is an extension.

*/

/* Tanh(x)
 * Return the Hyperbolic Tangent of x
 *
 * Method :
 *				       x    -x
 *				      e  - e
 *	0. tanh(x) is defined to be -----------
 *				       x    -x
 *				      e  + e
 *	1. reduce x to non-negative by tanh(-x) = -tanh(x).
 *	2.  0      <= x <= 2**-55 : tanh(x) := x*(one+x)
 *					        -t
 *	    2**-55 <  x <=  1     : tanh(x) := -----; t = expm1(-2x)
 *					       t + 2
 *						     2
 *	    1      <= x <=  22.0  : tanh(x) := 1-  ----- ; t=expm1(2x)
 *						   t + 2
 *	    22.0   <  x <= INF    : tanh(x) := 1.
 *
 * Special cases:
 *	tanh(NaN) is NaN;
 *	only tanh(0)=0 is exact for finite argument.
 */

#include "low/_math.h"

#ifndef _DOUBLE_IS_32BITS

#ifdef __STDC__
static const double one=1.0, two=2.0, tiny = 1.0e-300;
#else
static double one=1.0, two=2.0, tiny = 1.0e-300;
#endif

#ifdef __STDC__
	double tanh(double x)
#else
	double tanh(x)
	double x;
#endif
{
	double t,z,y=fabs(x);
	__int32_t jx,ix;
#ifdef __MATH_NONFINITE__
	double invx;
#endif
    /* High word of |x|. */
	GET_HIGH_WORD(jx,x);
	ix = jx&0x7fffffff;
  

#ifdef __MATH_NONFINITE__

    /* x is INF or NaN */
	if(ix>=0x7ff00000) 
  { 
	  invx =one/x; 
    if (jx>=0) return invx+one;    /* tanh(+-inf)=+-1 */
	  else       return invx-one;    /* tanh(NaN) = NaN */
	}
#endif /*__MATH_NONFINITE__*/

    /* |x| < 22 */
	if (ix < 0x40360000)
  {		/* |x|<22 */
    if (ix<0x3c800000) 		/* |x|<2**-55 */
		  return x*(one+x);    	/* tanh(small) = small */

    z=two*y;
    if (ix>=0x3ff00000)
    {	/* |x|>=1  */
	  	t = expm1(z);
      z = one - two/(t+two);
    } 
    else
    {
	    t = expm1(-z);
	    z= -t/(t+two);
	  }
    /* |x| > 22, return +-1 */
	} 
  else 
  {
	  z = one - tiny;		/* raised inexact flag */
	}
	return (jx>=0)? z: -z;
}

#endif /* _DOUBLE_IS_32BITS */
