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
*              file : $RCSfile: sf_scalbln.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

/* s_scalbnf.c -- float version of s_scalbn.c.
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

#ifdef __STDC__
static const float
#else
static float
#endif
two25   =  3.355443200e+07,	/* 0x4c000000 */
twom25  =  2.9802322388e-08,	/* 0x33000000 */
huge   = 1.0e+30,
tiny   = 1.0e-30;

#ifdef __STDC__
	float scalblnf (float x, long int n)
#else
	float scalblnf (x,n)
	float x; long int n;
#endif
{
  __int32_t k,ix;
  GET_FLOAT_WORD(ix,x);
  k = (ix&0x7f800000)>>23;		/* extract exponent */
  if ((ix&0x7fffffff)==0) return x; /* +-0 */
  if (k==0xff) return x+x;		/* NaN or Inf */
  if (k==0) {				/* 0 or subnormal x */
    x *= two25;
    GET_FLOAT_WORD(ix,x);
    k = ((ix&0x7f800000)>>23) - 25;
  }
  k = k+n;
#ifdef __MATH_EXCEPTION__        
  if (n> 50000 || k >  0xfe)
    return huge*copysignf(huge,x); /* raise overflow  */
  if (n< -50000 || k <= -25)
    return tiny*copysignf(tiny,x);	/* raise underflow */
#else
  if (n> 50000 || k >  0xfe)
    return huge*x; /* overflow  */
  if (n< -50000 || k <= -25)
    return tiny*x; /* underflow */
#endif    
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
  return x*twom25;
}

#ifdef _DOUBLE_IS_32BITS

#ifdef __STDC__
  double scalbln (double x, long int n)
#else
  double scalbln (x,n)
  double x; long int n;
#endif
{
  return (double) scalblnf((float) x, n);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
