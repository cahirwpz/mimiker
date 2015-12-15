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
* 		  file : $RCSfile: s_ilogb.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/


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
       <<ilogb>>, <<ilogbf>>---get exponent of floating-point number
INDEX
  ilogb
INDEX
  ilogbf

ANSI_SYNOPSIS
  #include <math.h>
        int ilogb(double <[val]>);
        int ilogbf(float <[val]>);

TRAD_SYNOPSIS
  #include <math.h>
        int ilogb(<[val]>)
        double <[val]>;

        int ilogbf(<[val]>)
        float <[val]>;


DESCRIPTION

  All nonzero, normal numbers can be described as <[m]> *
  2**<[p]>.  <<ilogb>> and <<ilogbf>> examine the argument
  <[val]>, and return <[p]>.  The functions <<frexp>> and
  <<frexpf>> are similar to <<ilogb>> and <<ilogbf>>, but also
  return <[m]>.

RETURNS

<<ilogb>> and <<ilogbf>> return the power of two used to form the
floating-point argument.
If <[val]> is <<0>>, they return <<FP_ILOGB0>>.
If <[val]> is infinite, they return <<INT_MAX>>.
If <[val]> is NaN, they return <<FP_ILOGBNAN>>.
(<<FP_ILOGB0>> and <<FP_ILOGBNAN>> are defined in math.h, but in turn are
defined as INT_MIN or INT_MAX from limits.h.  The value of FP_ILOGB0 may be
either INT_MIN or -INT_MAX.  The value of FP_ILOGBNAN may be either INT_MAX or
INT_MIN.)

@comment The bugs might not be worth noting, given the mass non-C99/POSIX
@comment behavior of much of the Newlib math library.
@comment BUGS
@comment On errors, errno is not set per C99 and POSIX requirements even if
@comment (math_errhandling & MATH_ERRNO) is non-zero.

PORTABILITY
C99, POSIX
*/

/* ilogb(double x)
 * return the binary exponent of non-zero x
 * ilogb(0) = 0x80000001
 * ilogb(inf/NaN) = 0x7fffffff (no signal is raised)
 */

#include <limits.h>
#include "low/_math.h"
#include "low/_gpinst.h"

#ifndef _DOUBLE_IS_32BITS

#ifdef __STDC__
  int ilogb(double x)
#else /* __STDC__ */
  int ilogb(x)
  double x;
#endif/* __STDC__ */
{
  __int32_t hx,lx;

  EXTRACT_WORDS(hx,lx,x);
  hx &= 0x7fffffff;

  if((hx|lx)==0) 
    return FP_ILOGB0; /* ilogb(0) = special case error */

#ifdef __MATH_SUBNORMAL__
  if(hx<0x00100000) 
  {
    __int32_t ix;
      /* subnormal x */
    if(hx==0) 
    {
#ifdef __HW_COUNT_LEAD_ZEROS__
      __inst_count_lead_zeros(ix,lx);
      return -1043-ix;
#else /* __HW_COUNT_LEAD_ZEROS__ */
      for (ix = -1043; lx>0; lx<<=1) 
        ix -=1;
      return ix;
#endif /* __HW_COUNT_LEAD_ZEROS__ */

    } 
    else 
    {
     
#ifdef __HW_COUNT_LEAD_ZEROS__
      hx<<=11;
      __inst_count_lead_zeros(ix,hx);
      return -1022-ix; 
#else /* __HW_COUNT_LEAD_ZEROS__ */
      for (ix = -1022,hx<<=11; hx>0; hx<<=1) 
        ix -=1;
      return ix;
#endif /* __HW_COUNT_LEAD_ZEROS__ */

    }
      
  }
#endif /*__MATH_SUBNORMAL__ */

  else if (hx<0x7ff00000) 
    return (hx>>20)-1023;

#ifdef __MATH_NONFINITE__ 
  #if FP_ILOGBNAN != INT_MAX
  else if (hx>0x7ff00000) 
    return FP_ILOGBNAN; /* NAN */
  #endif
  else 
    return INT_MAX;  /* infinite (or, possibly, NAN) */
#endif /*__MATH_NONFINITE__ */
	return 0;
}

#endif /* _DOUBLE_IS_32BITS */

