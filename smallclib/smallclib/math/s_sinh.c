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
*       file : $RCSfile: s_sinh.c,v $ 
*         author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* @(#)e_sinh.c 5.1 93/09/24 */
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

/* __ieee754_sinh(x)
 * Method : 
 * mathematically sinh(x) if defined to be (exp(x)-exp(-x))/2
 *  1. Replace x by |x| (sinh(-x) = -sinh(x)). 
 *  2. 
 *                                        E + E/(E+1)
 *      0        <= x <= 22     :  sinh(x) := --------------, E=expm1(x)
 *                          2
 *
 *      22       <= x <= lnovft :  sinh(x) := exp(x)/2 
 *      lnovft   <= x <= ln2ovft:  sinh(x) := exp(x/2)/2 * exp(x/2)
 *      ln2ovft  <  x     :  sinh(x) := x*shuge (overflow)
 *
 * Special cases:
 *  sinh(x) is |x| if x is +INF, -INF, or NaN.
 *  only sinh(0)=0 is exact for finite x.
 */

#include "low/_math.h"
#include "low/_fpuinst.h"
#include "low/_gpinst.h"
#include <errno.h>

#ifndef _DOUBLE_IS_32BITS
#ifdef __STDC__
static const double one = 1.0, shuge = 1.0e307;
#else
static double one = 1.0, shuge = 1.0e307;
#endif


#ifdef __STDC__
  double sinh(double x)
#else
  double sinh(x)
  double x;
#endif
{ 
  double t,w,h,y,z,one;
  __int32_t ix,jx;
  __uint32_t lx;

    /* High word of |x|. */
  GET_HIGH_WORD(jx,x);
  ix = jx&0x7fffffff;

#ifdef __MATH_NONFINITE__
  /* x is INF or NaN */
  if(ix>=0x7ff00000) return x+x;  
#endif /*__MATH_NONFINITE__*/

  /*if (x<0) h=0.5 else -0.5 */
#ifdef __MATH_CONST_FROM_MEMORY__
  h=0.5;
  if(jx<0) h=-h;
  one=1.0;
#else /* __MATH_CONST_FROM_MEMORY__ */
  __inst_set_lo_bits(jx,0x3fe00000, 31);
  __inst_ldi_D_H(h,jx);
  __inst_ldi_D_W(one,1);
#endif /* __MATH_CONST_FROM_MEMORY__ */

#ifdef __MATH_SOFT_FLOAT__
  y=fabs(x);
#else /* __MATH_SOFT_FLOAT__ */
  __inst_abs_D(y,x);
#endif /* __MATH_SOFT_FLOAT__ */

  t = expm1(y);
    
  /* |x| in [0,22], return sign(x)*0.5*(E+E/(E+1))) */
  if (ix < 0x40360000) 
  {    /* |x|<22 */
    if (ix<0x3e300000) {
#ifdef __MATH_MATCH_NEWLIB_EXCEPTION__
      if (shuge+x>one)     /* |x|<2**-28 */
#endif /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
	return x ;/* sinh(tiny) = tiny with inexact */
    }

    if(ix<0x3ff00000) return h*(2.0*t-t*t/(t+one));
    return h*(t+t/(t+one));
  }
  else
  {
    /* |x| in [22, log(maxdouble)] return 0.5*exp(|x|) */
    if (ix < 0x40862E42)  
      return h*(t+one);

    /* |x| in [log(maxdouble), overflowthresold] */
    GET_LOW_WORD(lx,x);
    if (ix<0x408633CE || (ix==0x408633ce && lx<=(__uint32_t)0x8fb9f87d)) 
    {
      w = expm1(h*x) + one;
      t = h*w;
      z=t*w;
    } 
    else {
      /* |x| > overflowthresold, sinh(x) overflow */
      z= x*shuge;
      errno = ERANGE;
    }
  }

  return z; 
}

#endif /* defined(_DOUBLE_IS_32BITS) */

