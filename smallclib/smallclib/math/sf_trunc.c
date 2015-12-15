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
* 		  file : $RCSfile: sf_trunc.c,v $ 
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
	float truncf(float x)
#else
	float truncf(x)
	float x;
#endif
{
#ifdef __MATH_SOFT_FLOAT__
  __int32_t signbit, w, exponent_less_127;

  GET_FLOAT_WORD(w,x);

  /* Extract sign bit. */
  signbit = w & 0x80000000;

  /* Extract exponent field. */
  exponent_less_127 = ((w & 0x7f800000) >> 23) - 127;

  if (exponent_less_127 < 23)
    {
      if (exponent_less_127 < 0)
        {
          /* -1 < x < 1, so result is +0 or -0. */
          SET_FLOAT_WORD(x, signbit);
        }
      else
        {
          SET_FLOAT_WORD(x, signbit | (w & ~(0x007fffff >> exponent_less_127)));
        }
    }
#ifdef __MATH_NONFINITE__
  else
    {
      if (exponent_less_127 == 255)
        /* x is NaN or infinite. */
        return x + x;

      /* All bits in the fraction field are relevant. */
    }
#endif /* __MATH_NONFINITE__ */
  return x;
#else /* __MATH_SOFT_FLOAT__ */
  float xtr;
  __uint32_t flags;

  __inst_trunc_S (xtr, x, flags);

  /* Argument outside valid range [-2^63,2^63], return argument */
  if (flags & FCSR_CAUSE_INVALIDOP_BIT)
	  return x;

  __inst_copysign_S (xtr,x);

  return (xtr);
#endif /* __MATH_SOFT_FLOAT__ */
}

#ifdef _DOUBLE_IS_32BITS

#ifdef __STDC__
	double trunc(double x)
#else
	double trunc(x)
	double x;
#endif
{
	return (double) truncf((float) x);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
