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
*              file : $RCSfile: s_scalbln.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

/* @(#)s_scalbn.c 5.1 93/09/24 */
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
 * scalbn (double x, int n)
 * scalbn(x,n) returns x* 2**n  computed by  exponent
 * manipulation rather than by actually performing an
 * exponentiation or a multiplication.
 */

#include "low/_math.h"
#include "low/_gpinst.h"

#ifndef _DOUBLE_IS_32BITS

#ifdef __STDC__
static const double
#else
static double
#endif
two54   =  1.80143985094819840000e+16, /* 0x43500000, 0x00000000 */
twom54  =  5.55111512312578270212e-17, /* 0x3C900000, 0x00000000 */
huge   = 1.0e+300,
tiny   = 1.0e-300;

#ifdef __STDC__
	double scalbln (double x, long int n)
#else
	double scalbln (x,n)
	double x; long int n;
#endif
{
  __int32_t k,hx,lx;
  EXTRACT_WORDS(hx,lx,x);
  k = (hx&0x7ff00000)>>20;		/* extract exponent */
  if ((lx|(hx&0x7fffffff))==0) return x; /* +-0 */
  if (k==0x7ff) return x+x;		/* NaN or Inf */
  if (k==0) {				/* 0 or subnormal x */
    x *= two54;
    GET_HIGH_WORD(hx,x);
    k = ((hx&0x7ff00000)>>20) - 54;
  }
  k = k+n;
#ifdef __MATH_EXCEPTION__         
  if (n> 50000 || k >  0x7fe)
    return huge*copysign(huge,x); /* overflow  */
  if (n< -50000 || k <= -54) return tiny*copysign(tiny,x); /*underflow*/
#else
  if (n> 50000 || k >  0x7fe)
    return huge*x; /* overflow  */
  if (n< -50000 || k <= -54) return tiny*x; /*underflow*/
#endif
  if (k > 0) { 				/* normal result */
#ifdef __HW_SET_BITS__
    __inst_set_bits (hx,k,20,11);
#else  /* __HW_SET_BITS__ */
    hx = (hx&0x800fffff)|(k<<20);
#endif /* __HW_SET_BITS__ */
    SET_HIGH_WORD(x, hx);
    return x;
  }
  k += 54;				/* subnormal result */
#ifdef __HW_SET_BITS__
  __inst_set_bits (hx,k,20,11);
#else  /* __HW_SET_BITS__ */
  hx = (hx&0x800fffff)|(k<<20);
#endif /* __HW_SET_BITS__ */
  SET_HIGH_WORD(x, hx);
  return x*twom54;
}

#endif /* _DOUBLE_IS_32BITS */
