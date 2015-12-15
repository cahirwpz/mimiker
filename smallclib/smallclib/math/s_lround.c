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
* 		  file : $RCSfile: s_lround.c,v $ 
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
<<lround>>, <<lroundf>>, <<llround>>, <<llroundf>>--round to integer, to nearest
INDEX
	lround
INDEX
	lroundf
INDEX
	llround
INDEX
	llroundf

ANSI_SYNOPSIS
	#include <math.h>
	long int lround(double <[x]>);
	long int lroundf(float <[x]>);
	long long int llround(double <[x]>);
	long long int llroundf(float <[x]>);

DESCRIPTION
	The <<lround>> and <<llround>> functions round their argument to the
	nearest integer value, rounding halfway cases away from zero, regardless
	of the current rounding direction.  If the rounded value is outside the
	range of the return type, the numeric result is unspecified (depending
	upon the floating-point implementation, not the library).  A range
	error may occur if the magnitude of x is too large.

RETURNS
<[x]> rounded to an integral value as an integer.

SEEALSO
See the <<round>> functions for the return being the same floating-point type
as the argument.  <<lrint>>, <<llrint>>.

PORTABILITY
ANSI C, POSIX

*/

#include "low/_math.h"
#include "low/_fpuinst.h"
#include "low/_machine.inc"

#ifndef _DOUBLE_IS_32BITS

#ifdef __STDC__
	long int lround(double x)
#else
	long int lround(x)
	double x;
#endif
{
#ifdef __MATH_SOFT_FLOAT__ 
  __int32_t sign, exponent_less_1023;
  /* Most significant word, least significant word. */
  __uint32_t msw, lsw;
  long int result;
  
  EXTRACT_WORDS(msw, lsw, x);

  /* Extract sign. */
  sign = ((msw & 0x80000000) ? -1 : 1);
  /* Extract exponent field. */
  exponent_less_1023 = ((msw & 0x7ff00000) >> 20) - 1023;
  msw &= 0x000fffff;
  msw |= 0x00100000;
  /* exponent_less_1023 in [-1023,1024] */
  if (exponent_less_1023 < 20)
    {
      /* exponent_less_1023 in [-1023,19] */
      if (exponent_less_1023 < 0)
        {
          if (exponent_less_1023 < -1)
            return 0;
          else
            return sign;
        }
      else
        {
          /* exponent_less_1023 in [0,19] */
	  /* shift amt in [0,19] */
          msw += 0x80000 >> exponent_less_1023;
	  /* shift amt in [20,1] */
          result = msw >> (20 - exponent_less_1023);
        }
    }
  else if (exponent_less_1023 < (8 * sizeof (long int)) - 1)
    {
      /* 32bit long: exponent_less_1023 in [20,30] */
      /* 64bit long: exponent_less_1023 in [20,62] */
      if (exponent_less_1023 >= 52)
	/* 64bit long: exponent_less_1023 in [52,62] */
	/* 64bit long: shift amt in [32,42] */
        result = ((long int) msw << (exponent_less_1023 - 20))
		/* 64bit long: shift amt in [0,10] */
                | (lsw << (exponent_less_1023 - 52));
      else
        {
	  /* 32bit long: exponent_less_1023 in [20,30] */
	  /* 64bit long: exponent_less_1023 in [20,51] */
          unsigned int tmp = lsw
		    /* 32bit long: shift amt in [0,10] */
		    /* 64bit long: shift amt in [0,31] */
                    + (0x80000000 >> (exponent_less_1023 - 20));
          if (tmp < lsw)
            ++msw;
	  /* 32bit long: shift amt in [0,10] */
	  /* 64bit long: shift amt in [0,31] */
          result = ((long int) msw << (exponent_less_1023 - 20))
		    /* ***32bit long: shift amt in [32,22] */
		    /* ***64bit long: shift amt in [32,1] */
                    | SAFE_RIGHT_SHIFT (tmp, (52 - exponent_less_1023));
        }
    }
  else
    /* Result is too large to be represented by a long int. */
    return (long int)x;

  return sign * result;
#else /* __MATH_SOFT_FLOAT__ */
  double xr, xin;
  long int retval;
#ifdef __MATH_MATCH_x86_IASim__
  __uint32_t state;
#endif /* __MATH_MATCH_x86_IASim__ */  

  __inst_abs_D (xin, x);

#ifdef __MATH_MATCH_x86_IASim__
  __inst_round_W_D_state (xr, xin, state);
#else /* __MATH_MATCH_x86_IASim__ */
  __inst_round_W_D (xr, xin);
#endif /* __MATH_MATCH_x86_IASim__ */

  GET_LOW_WORD (retval,xr);

#ifdef __MATH_MATCH_x86_IASim__
  if (state & FCSR_CAUSE_INVALIDOP_BIT)
      retval++;
  else
#endif /* __MATH_MATCH_x86_IASim__ */
  {
#ifdef __MATH_CONST_FROM_MEMORY__
    const double half=0.5;
#else /* __MATH_CONST_FROM_MEMORY__ */
    /* Load 0.5 */
    double half;
    __inst_ldi_D_H (half, 0x3fe00000);
#endif /* __MATH_CONST_FROM_MEMORY__ */
    if ((xin-(double)retval) >= half)
      retval++;
  }

  if (x < 0)
    retval=-retval;

  return (retval);
#endif /* __MATH_SOFT_DOUBLE__ */

}

#endif /* _DOUBLE_IS_32BITS */
