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
*              file : $RCSfile: sf_scalbn.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

/* sf_scalbn.c -- float version of s_scalbn.c.
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
#include "low/_gpinst.h"
#include "low/_fpuinst.h"
#include <limits.h>
#include <float.h>

#if INT_MAX > 50000
#define OVERFLOW_INT 50000
#else
#define OVERFLOW_INT 30000
#endif

#ifdef __STDC__
static const float
#else
static float
#endif
/* two25   =  3.355443200e+07,	/\* 0x4c000000 *\/ */
/* twom25  =  2.9802322388e-08,	/\* 0x33000000 *\/ */
huge   = 1.0e+30,
tiny   = 1.0e-30;

#ifdef __STDC__
	float scalbnf (float x, int n)
#else
	float scalbnf (x,n)
	float x; int n;
#endif
{
  float twom25, s;
  __int32_t  k,ix;
  __uint32_t hx;

  GET_FLOAT_WORD(ix,x);
  hx = ix&0x7fffffff;
  k = hx>>23;		/* extract exponent */
  if (FLT_UWORD_IS_ZERO(hx))
    return x;

#ifdef __MATH_NONFINITE__
  if (!FLT_UWORD_IS_FINITE(hx))
    return x+x;		/* NaN or Inf */
#endif /* __MATH_NONFINITE__ */

#ifdef __MATH_SUBNORMAL__
  if (FLT_UWORD_IS_SUBNORMAL(hx)) {
    float two25;
#ifdef __MATH_CONST_FROM_MEMORY__
    two25   =  3.355443200e+07;	/* 0x4c000000 */
#else /* __MATH_CONST_FROM_MEMORY__ */
    __inst_ldi_S_H (two25, 0x4c00);
#endif /* __MATH_CONST_FROM_MEMORY__ */
    x *= two25;   
    GET_FLOAT_WORD(ix,x);
    k = ((ix&0x7f800000)>>23) - 25;
    if (n< -50000) return tiny*x; 	/*underflow*/
  }
#endif /* __MATH_SUBNORMAL__ */

#ifdef __MATH_CONST_FROM_MEMORY__
  s=1.0f;
#else  /* __MATH_CONST_FROM_MEMORY__ */
  __inst_ldi_S_H(s, 0x3f80);
#endif /* __MATH_CONST_FROM_MEMORY__ */
  if (ix < 0)
    s=-s;

  k = k+n;
  if (k > FLT_LARGEST_EXP || ((k < FLT_SMALLEST_EXP)&&(n > OVERFLOW_INT))) 
#ifdef __MATH_EXCEPTION__
    return s*huge*huge;
#else /* __MATH_EXCEPTION__ */
    return s*huge;
#endif /* __MATH_EXCEPTION__ */
  if (k < FLT_SMALLEST_EXP) /*underflow*/
#ifdef __MATH_EXCEPTION__
    return s*tiny*tiny;
#else /* __MATH_EXCEPTION__ */
    return s*tiny;
#endif /* __MATH_EXCEPTION__ */
            
  if (k > 0) {				/* normal result */
#ifdef __HW_SET_BITS__
    __inst_set_bits (ix,k,23,8);
#else  /* __HW_SET_BITS__ */
    ix = (ix&0x807fffff)|(k<<23);
#endif /* __HW_SET_BITS__ */
    SET_FLOAT_WORD(x, ix);
    return x;
  }  
  k += 25;				/* subnormal result */
#ifdef __HW_SET_BITS__
  __inst_set_bits (ix,k,23,8);
#else  /* __HW_SET_BITS__ */
  ix = (ix&0x807fffff)|(k<<23);
#endif /* __HW_SET_BITS__ */
  SET_FLOAT_WORD(x, ix);

#ifdef __MATH_CONST_FROM_MEMORY__
  twom25  =  2.9802322388e-08;	/* 0x33000000 */
#else /* __MATH_CONST_FROM_MEMORY__ */
  __inst_ldi_S_H (twom25, 0x3300);
#endif /* __MATH_CONST_FROM_MEMORY__ */

  return x*twom25;
}

#ifdef _DOUBLE_IS_32BITS

#ifdef __STDC__
  double scalbn(double x, int n)
#else
  double scalbn(x,n)
  double x;
  int n;
#endif
{
  return (double) scalbnf((float) x, n);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
