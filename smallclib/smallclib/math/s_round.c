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
* 		  file : $RCSfile: s_round.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* Soft-float version from Newlib 2.0, optimized FPU version from scratch */

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
<<round>>, <<roundf>>--round to integer, to nearest
INDEX
	round
INDEX
	roundf

ANSI_SYNOPSIS
	#include <math.h>
	double round(double <[x]>);
	float roundf(float <[x]>);

DESCRIPTION
	The <<round>> functions round their argument to the nearest integer
	value in floating-point format, rounding halfway cases away from zero,
	regardless of the current rounding direction.  (While the "inexact"
	floating-point exception behavior is unspecified by the C standard, the
	<<round>> functions are written so that "inexact" is not raised if the
	result does not equal the argument, which behavior is as recommended by
	IEEE 754 for its related functions.)

RETURNS
<[x]> rounded to an integral value.

PORTABILITY
ANSI C, POSIX

SEEALSO
<<nearbyint>>, <<rint>>

*/

#include "low/_math.h"
#include "low/_fpuinst.h"
#include "low/_machine.inc"

#ifndef _DOUBLE_IS_32BITS

#ifdef __STDC__
	double round(double x)
#else
	double round(x)
	double x;
#endif
{
#ifdef __MATH_SOFT_FLOAT__
  /* Most significant word, least significant word. */
  __int32_t msw, exponent_less_1023;
  __uint32_t lsw;

  EXTRACT_WORDS(msw, lsw, x);

  /* Extract exponent field. */
  exponent_less_1023 = ((msw & 0x7ff00000) >> 20) - 1023;

  if (exponent_less_1023 < 20)
    {
      if (exponent_less_1023 < 0)
        {
          msw &= 0x80000000;
          if (exponent_less_1023 == -1)
            /* Result is +1.0 or -1.0. */
            msw |= (1023 << 20);
          lsw = 0;
        }
      else
        {
          __uint32_t exponent_mask = 0x000fffff >> exponent_less_1023;
          if ((msw & exponent_mask) == 0 && lsw == 0)
            /* x in an integral value. */
            return x;

          msw += 0x00080000 >> exponent_less_1023;
          msw &= ~exponent_mask;
          lsw = 0;
        }
    }
  else if (exponent_less_1023 > 51)
    {
#ifdef __MATH_NONFINITE__
      if (exponent_less_1023 == 1024)
        /* x is NaN or infinite. */
        return x + x;
      else
#endif /* __MATH_NONFINITE__ */
        return x;
    }
  else
    {
      __uint32_t exponent_mask = 0xffffffff >> (exponent_less_1023 - 20);
      __uint32_t tmp;

      if ((lsw & exponent_mask) == 0)
        /* x is an integral value. */
        return x;

      tmp = lsw + (1 << (51 - exponent_less_1023));
      if (tmp < lsw)
        msw += 1;
      lsw = tmp;

      lsw &= ~exponent_mask;
    }
  INSERT_WORDS(x, msw, lsw);

  return x;
#else /* __MATH_SOFT_FLOAT__ */
  double xr, xin;
  __uint32_t flags;
  double half;
  
  __inst_abs_D (xin, x);
  __inst_round_D (xr, xin, flags);

  /* Argument outside valid range [-2^63,2^63], return argument */
  if (flags & FCSR_CAUSE_INVALIDOP_BIT)
    xr=xin;

#ifdef __MATH_CONST_FROM_MEMORY__
  half=0.5;
#else /* __MATH_CONST_FROM_MEMORY__ */
  __inst_ldi_D_H (half, 0x3fe00000);
#endif /* __MATH_CONST_FROM_MEMORY__ */

  if (xin-xr >= half) {
#ifdef __MATH_CONST_FROM_MEMORY__
    xr+=1.0;
#else /* __MATH_CONST_FROM_MEMORY__ */
    double correction;
    __inst_ldi_D_W (correction, 1);
    xr+=correction;
#endif /* __MATH_CONST_FROM_MEMORY__ */
  }

  __inst_copysign_D (xr,x);

  return (xr);
#endif /* __MATH_SOFT_FLOAT__ */
}

#endif /* _DOUBLE_IS_32BITS */
