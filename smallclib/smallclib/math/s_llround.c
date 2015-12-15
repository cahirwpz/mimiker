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

/* lround adapted to be llround for Newlib, 2009 by Craig Howland.  */
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
#include "low/_fpuinst.h"
#include "low/_machine.inc"

#ifndef _DOUBLE_IS_32BITS

long long int
llround(double x)
{
#ifdef __MATH_SOFT_FLOAT__
  __int32_t sign, exponent_less_1023;
  /* Most significant word, least significant word. */
  __uint32_t msw, lsw;
  long long int result;
  
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
  else if (exponent_less_1023 < (8 * sizeof (long long int)) - 1)
    {
      /* 64bit longlong: exponent_less_1023 in [20,62] */
      if (exponent_less_1023 >= 52)
	/* 64bit longlong: exponent_less_1023 in [52,62] */
	/* 64bit longlong: shift amt in [32,42] */
        result = ((long long int) msw << (exponent_less_1023 - 20))
		    /* 64bit longlong: shift amt in [0,10] */
                    | (lsw << (exponent_less_1023 - 52));
      else
        {
	  /* 64bit longlong: exponent_less_1023 in [20,51] */
          unsigned int tmp = lsw
		    /* 64bit longlong: shift amt in [0,31] */
                    + (0x80000000 >> (exponent_less_1023 - 20));
          if (tmp < lsw)
            ++msw;
	  /* 64bit longlong: shift amt in [0,31] */
          result = ((long long int) msw << (exponent_less_1023 - 20))
		    /* ***64bit longlong: shift amt in [32,1] */
                    | SAFE_RIGHT_SHIFT (tmp, (52 - exponent_less_1023));
        }
    }
  else
    /* Result is too large to be represented by a long long int. */
    return (long long int)x;

  return sign * result;
#else /* __MATH_SOFT_FLOAT__ */
  double xr, xin;
  long long int retval;
#ifdef __MATH_MATCH_x86_IASim__
  __uint32_t state;  
#endif /* __MATH_MATCH_x86_IASim__ */

  __inst_abs_D (xin, x);

#ifdef __MATH_MATCH_x86_IASim__
  __inst_round_L_D_state (xr, xin, state);
#else /* __MATH_MATCH_x86_IASim__ */
  __inst_round_L_D (xr, xin);
#endif /* __MATH_MATCH_x86_IASim__ */

  EXTRACT_LLWORD (retval,xr);

#ifdef __MATH_MATCH_x86_IASim__
  if (state & FCSR_CAUSE_INVALIDOP_BIT)
      retval++;
  else
#endif /* __MATH_MATCH_x86_IASim__ */
  {
#ifdef __MATH_CONST_FROM_MEMORY__
    const double half = 0.5;
#else /* __MATH_CONST_FROM_MEMORY__ */
    /* Load 0.5 */
    double half;
    __inst_ldi_D_H (half, 0x3fe00000);
#endif /* __MATH_CONST_FROM_MEMORY__ */
    __inst_conv_D_L (xr,xr);
    if ((xin-xr) >= half)
      retval++;
  }

  if (x < 0)
    retval=-retval;

  return (retval);
#endif /* __MATH_SOFT_DOUBLE__ */
}

#endif /* _DOUBLE_IS_32BITS */
