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
* 		  file : $RCSfile: sf_round.c,v $ 
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

#include "low/_math.h"
#include "low/_fpuinst.h"
#include "low/_machine.inc"

#ifdef __STDC__
	float roundf(float x)
#else
	float roundf(x)
	float x;
#endif
{
#ifdef __MATH_SOFT_FLOAT__ 
  __uint32_t w;
  /* Most significant word, least significant word. */
  int exponent_less_127;

  GET_FLOAT_WORD(w, x);

  /* Extract exponent field. */
  exponent_less_127 = (int)((w & 0x7f800000) >> 23) - 127;

  if (exponent_less_127 < 23)
    {
      if (exponent_less_127 < 0)
        {
          w &= 0x80000000;
          if (exponent_less_127 == -1)
            /* Result is +1.0 or -1.0. */
            w |= ((__uint32_t)127 << 23);
        }
      else
        {
          unsigned int exponent_mask = 0x007fffff >> exponent_less_127;
          if ((w & exponent_mask) == 0)
            /* x has an integral value. */
            return x;

          w += 0x00400000 >> exponent_less_127;
          w &= ~exponent_mask;
        }
    }
  else
    {
#ifdef __MATH_NONFINITE__
      if (exponent_less_127 == 128)
        /* x is NaN or infinite. */
        return x + x;
      else
#endif /* __MATH_NONFINITE__ */
        return x;
    }
  SET_FLOAT_WORD(x, w);
  return x;

#else /* __MATH_SOFT_FLOAT__ */
  float xr, xin;
  __uint32_t flags;
  float half;
  
  __inst_abs_S (xin, x);

  __inst_round_S (xr, xin, flags);
  /* Argument outside valid range [-2^63,2^63], return argument */
  if ((flags & FCSR_CAUSE_INVALIDOP_BIT))
    xr=xin;

#ifdef __MATH_CONST_FROM_MEMORY__
  half=0.5;
#else /* __MATH_CONST_FROM_MEMORY__ */
  __inst_ldi_S_H (half,0x3f00);
#endif /* __MATH_CONST_FROM_MEMORY__ */

  if (xin-xr >= half)
    {
#ifdef __MATH_CONST_FROM_MEMORY__
      xr+=1.0;
#else /* __MATH_CONST_FROM_MEMORY__ */
      float correction;
      __inst_ldi_S_W (correction, 1);
        xr+=correction;
#endif /* __MATH_CONST_FROM_MEMORY__ */
    }

  __inst_copysign_S (xr,x);

  return (xr);
#endif /* __MATH_SOFT_FLOAT__ */
}

#ifdef _DOUBLE_IS_32BITS

#ifdef __STDC__
	double round(double x)
#else
	double round(x)
	double x;
#endif
{
	return (double) roundf((float) x);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
