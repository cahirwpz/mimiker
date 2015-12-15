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

/* lroundf adapted to be llroundf for Newlib, 2009 by Craig Howland.  */
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

long long int
llroundf(float x)
{
#ifdef __MATH_SOFT_FLOAT__
  __int32_t exponent_less_127;
  __uint32_t w;
  long long int result;
  __int32_t sign;

  GET_FLOAT_WORD (w, x);
  exponent_less_127 = ((w & 0x7f800000) >> 23) - 127;
  sign = (w & 0x80000000) != 0 ? -1 : 1;
  w &= 0x7fffff;
  w |= 0x800000;

  if (exponent_less_127 < (int)((8 * sizeof (long long int)) - 1))
    {
      if (exponent_less_127 < 0)
        return exponent_less_127 < -1 ? 0 : sign;
      else if (exponent_less_127 >= 23)
        result = (long long int) w << (exponent_less_127 - 23);
      else
        {
          w += 0x400000 >> exponent_less_127;
          result = w >> (23 - exponent_less_127);
        }
    }
  else
      return (long long int) x;

  return sign * result;
#else /* __MATH_SOFT_FLOAT__ */
  float xin;
  double xr;
  long long int retval;
#ifdef __MATH_MATCH_x86_IASim__
  __uint32_t state;
#endif /* __MATH_MATCH_x86_IASim__ */

  __inst_abs_S (xin, x);

#ifdef __MATH_MATCH_x86_IASim__
  __inst_round_L_S_state (xr, xin, state);
#else /* __MATH_MATCH_x86_IASim__ */
  __inst_round_L_S (xr, xin);
#endif /* __MATH_MATCH_x86_IASim__ */
  EXTRACT_LLWORD (retval, xr);

#ifdef __MATH_MATCH_x86_IASim__
  if (state & FCSR_CAUSE_INVALIDOP_BIT)
      retval++;
  else
#endif /* __MATH_MATCH_x86_IASim__ */
    {
      float xrf;  
#ifdef __MATH_CONST_FROM_MEMORY__
      const float half=0.5;
#else /* __MATH_CONST_FROM_MEMORY__ */
	/* Load 0.5 */
      float half;
	__inst_ldi_S_H (half, 0x3f00);
#endif /* __MATH_CONST_FROM_MEMORY__ */
      /* Recreate rounded float value in xrf */
      __inst_conv_S_L (xrf,xr);
      if ((xin-xrf) >= half)
	retval++;
    }

  if (x < 0)
    retval=-retval;

  return (retval);
#endif /* __MATH_SOFT_FLOAT__ */
}

#ifdef _DOUBLE_IS_32BITS

long long int
llround(double x)
{
	return llroundf((float) x);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
