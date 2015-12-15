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
*              file : $RCSfile: s_scalbn.c,v $
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
FUNCTION
<<scalbn>>, <<scalbnf>>, <<scalbln>>, <<scalblnf>>--scale by power of FLT_RADIX (=2)
INDEX
	scalbn
INDEX
	scalbnf
INDEX
	scalbln
INDEX
	scalblnf

ANSI_SYNOPSIS
	#include <math.h>
	double scalbn(double <[x]>, int <[n]>);
	float scalbnf(float <[x]>, int <[n]>);
	double scalbln(double <[x]>, long int <[n]>);
	float scalblnf(float <[x]>, long int <[n]>);

DESCRIPTION
The <<scalbn>> and <<scalbln>> functions compute 
	@ifnottex
	<[x]> times FLT_RADIX to the power <[n]>.
	@end ifnottex
	@tex
	$x \cdot FLT\_RADIX^n$.
	@end tex
efficiently.  The result is computed by manipulating the exponent, rather than
by actually performing an exponentiation or multiplication.  In this
floating-point implementation FLT_RADIX=2, which makes the <<scalbn>>
functions equivalent to the <<ldexp>> functions.

RETURNS
<[x]> times 2 to the power <[n]>.  A range error may occur.

PORTABILITY
ANSI C, POSIX

SEEALSO
<<ldexp>>

*/

/* 
 * scalbn (double x, int n)
 * scalbn(x,n) returns x* 2**n  computed by  exponent  
 * manipulation rather than by actually performing an 
 * exponentiation or a multiplication.
 */

#include "low/_math.h"
#include "low/_gpinst.h"
#include "low/_fpuinst.h"

#ifndef _DOUBLE_IS_32BITS

#ifdef __STDC__
static const double
#else
static double
#endif
huge   = 1.0e+300,
tiny   = 1.0e-300;

#ifdef __STDC__
	double scalbn (double x, int n)
#else
	double scalbn (x,n)
	double x; int n;
#endif
{
  __int32_t  k,hx,lx;
  double twom54, two54, s;
  EXTRACT_WORDS(hx,lx,x);
  k = (hx&0x7ff00000)>>20;		/* extract exponent */
  if ((lx|(hx&0x7fffffff))==0) return x; /* +-0 */
#ifdef __MATH_NONFINITE__
  if (k==0x7ff) return x+x;		/* NaN or Inf */
#endif /* __MATH_NONFINITE__ */
  if (k==0) {				/* 0 or subnormal x */
#ifdef __MATH_CONST_FROM_MEMORY__
    two54 = 1.80143985094819840000e+16;
#else /* __MATH_CONST_FROM_MEMORY__ */
    __inst_ldi_D_H (two54, 0x43500000);
#endif /* __MATH_CONST_FROM_MEMORY__ */
    x *= two54; 
    GET_HIGH_WORD(hx,x);
    k = ((hx&0x7ff00000)>>20) - 54;

    if (n< -50000) 
      return tiny*x; 	/*underflow*/
  }
  k = k+n;

#ifdef __MATH_CONST_FROM_MEMORY__ 
  s=copysign(1.0, x);
#else  /* __MATH_CONST_FROM_MEMORY__ */
  __inst_ldi_D_H(s, 0x3ff00000|(hx&0x80000000));
#endif /* __MATH_CONST_FROM_MEMORY__ */

  if (k >  0x7fe || ((k<=-54)&&(n>50000)))
#ifdef __MATH_EXCEPTION__
    return s*huge*huge;
#else /* __MATH_EXCEPTION__ */
    return s*huge;
#endif /* __MATH_EXCEPTION__ */
  if(k<=-54)
#ifdef __MATH_EXCEPTION__
    return s*tiny*tiny;
#else /* __MATH_EXCEPTION__ */
    return s*tiny;
#endif /* __MATH_EXCEPTION__ */

  if (k > 0) {				/* normal result */
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


#ifdef __MATH_CONST_FROM_MEMORY__
  twom54 = 5.55111512312578270212e-17;
#else /* __MATH_CONST_FROM_MEMORY__ */
  __inst_ldi_D_H (twom54,  0x3C900000);
#endif /* __MATH_CONST_FROM_MEMORY__ */

  return x*twom54;
}

#endif /* _DOUBLE_IS_32BITS */
