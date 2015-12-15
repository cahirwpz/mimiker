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
*       file : $RCSfile: s_logb.c,v $ 
*         author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/


/* 2009 for Newlib:  Sun's s_ilogb.c converted to be s_logb.c.  */
/* @(#)s_ilogb.c 5.1 93/09/24 */
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
       <<logb>>, <<logbf>>--get exponent of floating-point number
INDEX
  logb
INDEX
  logbf

ANSI_SYNOPSIS
  #include <math.h>
        double logb(double <[x]>);
        float logbf(float <[x]>);

DESCRIPTION
The <<logb>> functions extract the exponent of <[x]>, as a signed integer value
in floating-point format.  If <[x]> is subnormal it is treated as though it were
normalized; thus, for positive finite <[x]>,
@ifnottex
1 <= (<[x]> * FLT_RADIX to the power (-logb(<[x]>))) < FLT_RADIX.
@end ifnottex
@tex
$1 \leq ( x \cdot FLT\_RADIX ^ {-logb(x)} ) < FLT\_RADIX$.
@end tex
A domain error may occur if the argument is zero.
In this floating-point implementation, FLT_RADIX is 2.  Which also means
that for finite <[x]>, <<logb>>(<[x]>) = <<floor>>(<<log2>>(<<fabs>>(<[x]>))).

All nonzero, normal numbers can be described as
@ifnottex
<[m]> * 2**<[p]>, where 1.0 <= <[m]> < 2.0.
@end ifnottex
@tex
$m \cdot 2^p$, where $1.0 \leq m < 2.0$.
@end tex
The <<logb>> functions examine the argument <[x]>, and return <[p]>.
The <<frexp>> functions are similar to the <<logb>> functions, but
returning <[m]> adjusted to the interval [.5, 1) or 0, and <[p]>+1.

RETURNS
@comment Formatting note:  "$@" forces a new line
When <[x]> is:@*
+inf or -inf, +inf is returned;@*
NaN, NaN is returned;@*
0, -inf is returned, and the divide-by-zero exception is raised;@*
otherwise, the <<logb>> functions return the signed exponent of <[x]>.

PORTABILITY
ANSI C, POSIX

SEEALSO
frexp, ilogb
*/

/* double logb(double x)
 * return the binary exponent of non-zero x
 * logb(0) = -inf, raise divide-by-zero floating point exception
 * logb(+inf|-inf) = +inf (no signal is raised)
 * logb(NaN) = NaN (no signal is raised)
 * Per C99 recommendation, a NaN argument is returned unchanged.
 */

#include "low/_math.h"
#include "low/_gpinst.h"

#ifndef _DOUBLE_IS_32BITS

#ifdef __STDC__
double logb(double x)
#else
double logb(x)
double x;
#endif
{
  __int32_t hx,lx;

  EXTRACT_WORDS(hx,lx,x);
  hx &= 0x7fffffff;   /* high |x| */
  if((hx|lx)==0)
  {
    double  xx;
    /* arg==0:  return -inf and raise divide-by-zero exception */
    INSERT_WORDS(xx,hx,lx); /* +0.0 */
    return -1./xx;  /* logb(0) = -inf */
  }

#ifdef __MATH_SUBNORMAL__
  if(hx<0x00100000)
  {   /*  subnormal */
    __int32_t ix;
    if(hx==0)
    {   
#ifdef __HW_COUNT_LEAD_ZEROS__
      __inst_count_lead_zeros(ix,lx);
      return (double) -1043-ix;
#else /* __HW_COUNT_LEAD_ZEROS__ */

      for (ix = -1043; lx>0; lx<<=1) ix -=1;
      return (double) ix;
#endif /* __HW_COUNT_LEAD_ZEROS__ */
    }
    else
    {
#ifdef __HW_COUNT_LEAD_ZEROS__
      hx<<=11;
      __inst_count_lead_zeros(ix,hx);
      return (double) -1022-ix;
#else   /* __HW_COUNT_LEAD_ZEROS__ */
      for (ix = -1022,hx<<=11; hx>0; hx<<=1) ix -=1;
      return (double) ix;
#endif   /* __HW_COUNT_LEAD_ZEROS__ */
    }

  }
#endif /* __MATH_SUBNORMAL__ */
  else if (hx<0x7ff00000)
    return (hx>>20)-1023; /* normal # */

#ifdef __MATH_NONFINITE__
  else if (hx>0x7ff00000 || lx)
    return x; /* x==NaN */
  else
    return HUGE_VAL;  /* x==inf (+ or -) */
#endif /* __MATH_NONFINITE__ */
	return 0;
}

#endif /* _DOUBLE_IS_32BITS */
