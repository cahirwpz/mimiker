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
*              file : $RCSfile: s_frexp.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

/* @(#)s_frexp.c 5.1 93/09/24 */
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
       <<frexp>>, <<frexpf>>---split floating-point number
INDEX
	frexp
INDEX
	frexpf

ANSI_SYNOPSIS
	#include <math.h>
        double frexp(double <[val]>, int *<[exp]>);
        float frexpf(float <[val]>, int *<[exp]>);

TRAD_SYNOPSIS
	#include <math.h>
        double frexp(<[val]>, <[exp]>)
        double <[val]>;
        int *<[exp]>;

        float frexpf(<[val]>, <[exp]>)
        float <[val]>;
        int *<[exp]>;


DESCRIPTION
	All nonzero, normal numbers can be described as <[m]> * 2**<[p]>.
	<<frexp>> represents the double <[val]> as a mantissa <[m]>
	and a power of two <[p]>. The resulting mantissa will always
	be greater than or equal to <<0.5>>, and less than <<1.0>> (as
	long as <[val]> is nonzero). The power of two will be stored
	in <<*>><[exp]>. 

@ifnottex
<[m]> and <[p]> are calculated so that
<[val]> is <[m]> times <<2>> to the power <[p]>.
@end ifnottex
@tex
<[m]> and <[p]> are calculated so that
$ val = m \times 2^p $.
@end tex

<<frexpf>> is identical, other than taking and returning
floats rather than doubles.

RETURNS
<<frexp>> returns the mantissa <[m]>. If <[val]> is <<0>>, infinity,
or Nan, <<frexp>> will set <<*>><[exp]> to <<0>> and return <[val]>.

PORTABILITY
<<frexp>> is ANSI.
<<frexpf>> is an extension.


*/

/*
 * for non-zero x 
 *	x = frexp(arg,&exp);
 * return a double fp quantity x such that 0.5 <= |x| <1.0
 * and the corresponding binary exponent "exp". That is
 *	arg = x*2^exp.
 * If arg is inf, 0.0, or NaN, then frexp(arg,&exp) returns arg 
 * with *exp=0. 
 */

#include "low/_math.h"
#include "low/_gpinst.h"
#include "low/_fpuinst.h"

#ifndef _DOUBLE_IS_32BITS

#ifdef __STDC__
	double frexp(double x, int *eptr)
#else
	double frexp(x, eptr)
	double x; int *eptr;
#endif
{
  __int32_t hx, ix, exp;
  GET_HIGH_WORD(hx,x);
  ix = 0x7fffffff&hx;
  exp = 0;

#ifdef __MATH_NONFINITE__
  {
    __int32_t lx;
    GET_LOW_WORD(lx,x);
    if(ix>=0x7ff00000||((ix|lx)==0)) {*eptr=0; return x;}	/* 0,inf,nan */
  }
#endif /* __MATH_NONFINITE__ */

#ifdef __MATH_SUBNORMAL__
  if (ix<0x00100000) {		/* subnormal */
    double two54;
#ifdef __MATH_SOFT_FLOAT__
    INSERT_WORDS (two54, 0x43500000, 0);
#else
    __inst_ldi_D_H(two54, 0x43500000);
#endif
    x *= two54;
    GET_HIGH_WORD(hx,x);
    ix = hx&0x7fffffff;
    exp = -54;
  }
#endif /* __MATH_SUBNORMAL__ */

  *eptr = exp + ((ix>>20)-1022);
#ifdef __HW_SET_BITS__
  __inst_set_bits (hx,0x3fe,20,11);
#else  /* __HW_SET_BITS__ */     
  hx = (hx&0x800fffff)|0x3fe00000;
#endif /* __HW_SET_BITS__ */ 
  SET_HIGH_WORD(x,hx);
  return x;
}

#endif /* _DOUBLE_IS_32BITS */
