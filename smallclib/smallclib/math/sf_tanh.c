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
* 		  file : $RCSfile: sf_tanh.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/


/* sf_tanh.c -- float version of s_tanh.c.
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

#ifdef __STDC__
static const float one=1.0, two=2.0, tiny = 1.0e-30;
#else
static float one=1.0, two=2.0, tiny = 1.0e-30;
#endif

#ifdef __STDC__
	float tanhf(float x)
#else
	float tanhf(x)
	float x;
#endif
{
	float t,z,y=fabsf(x);
	__int32_t jx,ix;
#ifdef __MATH_NONFINITE__
	float invx;
#endif

	GET_FLOAT_WORD(jx,x);
	ix = jx&0x7fffffff;

#ifdef __MATH_NONFINITE__

    /* x is INF or NaN */
	if(!FLT_UWORD_IS_FINITE(ix)) 
  {
	  invx =one/x; 
    if (jx>=0) return invx+one;    /* tanh(+-inf)=+-1 */
	  else       return invx-one;    /* tanh(NaN) = NaN */
  }
#endif /*__MATH_NONFINITE__*/

    /* |x| < 22 */
	if (ix < 0x41b00000) 
  {		/* |x|<22 */
	  if (ix<0x24000000) 		/* |x|<2**-55 */
		  return x*(one+x);    	/* tanh(small) = small */

    z=two*y;
	  if (ix>=0x3f800000)
    {	/* |x|>=1  */
	  	t = expm1f(z);
		  z = one - two/(t+two);
	  }
    else 
    {
	    t = expm1f(-z);
	    z= -t/(t+two);
	  }
    /* |x| > 22, return +-1 */
	}
  else 
    z = one - tiny;		/* raised inexact flag */
	
	return (jx>=0)? z: -z;
}

#ifdef _DOUBLE_IS_32BITS

#ifdef __STDC__
	double tanh(double x)
#else
	double tanh(x)
	double x;
#endif
{
	return (double) tanhf((float) x);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
