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
*              file : $RCSfile: sf_cbrt.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

/* sf_cbrt.c -- float version of s_cbrt.c.
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
 *
 */

#include "low/_math.h"

/* cbrtf(x)
 * Return cube root of x
 */
static const __uint32_t
  B1 = 709958130, /* B1 = (84+2/3-0.03306235651)*2**23 */
  B2 = 642849266; /* B2 = (76+2/3-0.03306235651)*2**23 */

static const float
  C =  5.4285717010e-01, /* 19/35     = 0x3f0af8b0 */
  D = -7.0530611277e-01, /* -864/1225 = 0xbf348ef1 */
  E =  1.4142856598e+00, /* 99/70     = 0x3fb50750 */
  F =  1.6071428061e+00, /* 45/28     = 0x3fcdb6db */
  G =  3.5714286566e-01; /* 5/14      = 0x3eb6db6e */

float cbrtf(float x)
{
  __int32_t	hx;
  float r,s,t;
  __uint32_t sign;
  __uint32_t high;

  GET_FLOAT_WORD(hx,x);
  sign = hx&0x80000000; 		/* sign= sign(x) */
  hx ^= sign;

  if(!FLT_UWORD_IS_FINITE(hx))
#ifndef __MATH_EXCEPTION__
    return x;
#else /* __MATH_EXCEPTION__ */
    return(x+x);		/* cbrt(NaN,INF) is itself */
#endif /* __MATH_EXCEPTION__ */

  if(FLT_UWORD_IS_ZERO(hx))
    return(x);			/* cbrt(0) is itself */

  SET_FLOAT_WORD(x,hx);	/* x <- |x| */

#ifdef __MATH_SUBNORMAL__
  /* rough cbrt to 5 bits */
  if(FLT_UWORD_IS_SUBNORMAL(hx)) 		/* subnormal number */
    {
      SET_FLOAT_WORD(t,0x4b800000); /* set t= 2**24 */
      t*=x;
      GET_FLOAT_WORD(high,t);
      SET_FLOAT_WORD(t,high/3+B2);
    }
  else
#endif /* __MATH_SUBNORMAL__ */
    SET_FLOAT_WORD(t,hx/3+B1);

  /* new cbrt to 23 bits */
  r=t*t/x;
  s=C+r*t;
  t*=G+F/(s+E+D/s);

  /* retore the sign bit */
  GET_FLOAT_WORD(high,t);
  SET_FLOAT_WORD(t,high|sign);
  return(t);
}
