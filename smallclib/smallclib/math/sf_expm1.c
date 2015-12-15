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
*              file : $RCSfile: sf_expm1.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

/* sf_expm1.c -- float version of s_expm1.c.
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

#include <errno.h>
#include "low/_math.h"
#include "low/_fpuinst.h"

#ifdef __v810__
#define const
#endif

#ifdef __STDC__
static const float
#else
static float
#endif
huge		= 1.0e+30,
tiny		= 1.0e-30,
ln2_hi		= 6.9313812256e-01,/* 0x3f317180 */
ln2_lo		= 9.0580006145e-06,/* 0x3717f7d1 */
invln2		= 1.4426950216e+00,/* 0x3fb8aa3b */
	/* scaled coefficients related to expm1 */
Q1  =  -3.3333335072e-02, /* 0xbd088889 */
Q2  =   1.5873016091e-03, /* 0x3ad00d01 */
Q3  =  -7.9365076090e-05, /* 0xb8a670cd */
Q4  =   4.0082177293e-06, /* 0x36867e54 */
Q5  =  -2.0109921195e-07; /* 0xb457edbb */

#ifdef __STDC__
	float expm1f(float x)
#else
	float expm1f(x)
	float x;
#endif
{
  float y,hi,lo,c,t,e,hxs,hfx,r1,three,six,half,one;
  __int32_t k,xsb;
  __uint32_t hx,hy;
  GET_FLOAT_WORD(hx,x);
  xsb = hx&0x80000000;		/* sign bit of x */
  hx &= 0x7fffffff;		/* high word of |x| */
  
#ifdef __MATH_CONST_FROM_MEMORY__
  one=1.0f;
#else /* __MATH_CONST_FROM_MEMORY__ */
  __inst_ldi_S_W(one,1);
#endif /* __MATH_CONST_FROM_MEMORY__ */

  /* filter out huge and non-finite argument */
#ifdef __MATH_NONFINITE__
  if(FLT_UWORD_IS_NAN(hx))
    return x+x;
  if(FLT_UWORD_IS_INFINITE(hx))
    return (xsb==0)? x:-one;/* exp(+-inf)={inf,-1} */
  if(xsb == 0 && hx > FLT_UWORD_LOG_MAX) {/* if x>=o_threshold */
    /* Signal range-error */
    errno=ERANGE;
    return huge*huge; /* overflow */
  }
  if(hx >= 0x4195b844) { 
    if(xsb!=0) { /* x < -27*ln2, return -1.0 with inexact */
#ifdef __MATH_MATCH_NEWLIB_EXCEPTION__
      if(x+tiny<(float)0.0)	/* raise inexact */
        return tiny-one;	/* return -1 */
#else /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
      return -one;
#endif /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
    }
  }
#endif /* __MATH_NONFINITE__ */

  /* argument reduction */
  if(hx > 0x3eb17218) {		/* if  |x| > 0.5 ln2 */ 
    if(hx < 0x3F851592) {	/* and |x| < 1.5 ln2 */
      if(xsb==0)
        {k =  1;}
      else
        {k = -1;}
    } else {
#ifdef __MATH_CONST_FROM_MEMORY__
      half=(xsb==0)?0.5:-0.5;
#else /* __MATH_CONST_FROM_MEMORY__ */
      SET_FLOAT_WORD (half,0x3f000000|xsb);
#endif /* __MATH_CONST_FROM_MEMORY__ */
      k  = invln2*x+half;
    }
    t  = k;
    hi = x - t*ln2_hi;	/* t*ln2_hi is exact here */
    lo = t*ln2_lo;
    x  = hi - lo;
    c  = (hi-x)-lo;
  } 
  else if(hx < 0x33000000) {  	/* when |x|<2**-25, return x */
#ifdef __MATH_MATCH_NEWLIB_EXCEPTION__
    t = huge+x;	/* return x with inexact flags when x!=0 */
    return x - (t-(huge+x));	
#else /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
	 return x;
#endif /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
  }
  else k = 0;

  /* x is now in primary range */
  hfx = (float)0.5*x;
  hxs = x*hfx;
  r1 = one+hxs*(Q1+hxs*(Q2+hxs*(Q3+hxs*(Q4+hxs*Q5))));
#ifdef __MATH_CONST_FROM_MEMORY__
  three=3.0f;
  six=6.0f;
#else /* __MATH_CONST_FROM_MEMORY__ */
  __inst_ldi_S_H(three,0x4040);
  __inst_ldi_S_H(six,0x40c0);
#endif /* __MATH_CONST_FROM_MEMORY__ */
  t  = three-r1*hfx;
  e  = hxs*((r1-t)/(six - x*t));
  if(k==0) return x - (x*e-hxs);		/* c is 0 */
  e  = (x*(e-c)-c);
  e -= hxs;
  if(k== -1) return (float)0.5*(x-e)-(float)0.5;
  if(k==1) {
    float two;
#ifdef __MATH_CONST_FROM_MEMORY__
  two=2.0f;
#else /* __MATH_CONST_FROM_MEMORY__ */
  __inst_ldi_S_H(two,0x4000);
#endif /* __MATH_CONST_FROM_MEMORY__ */

    if(x < (float)-0.25) return -two*(e-(x+(float)0.5));
    else return  one+two*(x-e);
  }
  if (k <= -2 || k>56) {   /* suffice to return exp(x)-1 */
    __int32_t i;
    y = one-(e-x);
    GET_FLOAT_WORD(i,y);
    SET_FLOAT_WORD(y,i+(k<<23));	/* add k to y's exponent */
    return y-one;
  }
  if(k<23) {
    SET_FLOAT_WORD(t,0x3f800000 - (0x1000000>>k)); /* t=1-2^-k */
    y = t-(e-x);
  } else {
    SET_FLOAT_WORD(t,((0x7f-k)<<23));	/* 2^-k */
    y = x-(e+t);
    y += one;
  }

  GET_FLOAT_WORD(hy,y);
  SET_FLOAT_WORD(y,hy+(k<<23));	/* add k to y's exponent */
  return y;
}

#ifdef _DOUBLE_IS_32BITS

#ifdef __STDC__
  double expm1(double x)
#else
  double expm1(x)
  double x;
#endif
{
  return (double) expm1f((float) x);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
