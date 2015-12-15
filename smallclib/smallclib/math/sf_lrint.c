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
* 		  file : $RCSfile: sf_lrint.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* Soft-float version from Newlib 2.0, optimized FPU version from scratch */

/* @(#)sf_lrint.c 5.1 93/09/24 */
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
 * lrintf(x)
 * Return x rounded to integral value according to the prevailing
 * rounding mode.
 * Method:
 *	Using floating addition.
 * Exception:
 *	Inexact flag raised if x not equal to lrintf(x).
 */

#include "low/_math.h"
#include "low/_fpuinst.h"
#include "low/_machine.inc"

#ifdef __STDC__
static const float
#else
static float 
#endif
/* Adding a float, x, to 2^23 will cause the result to be rounded based on
   the fractional part of x, according to the implementation's current rounding
   mode.  2^23 is the smallest float that can be represented using all 23 significant
   digits. */
TWO23[2]={
  8.3886080000e+06, /* 0x4b000000 */
 -8.3886080000e+06, /* 0xcb000000 */
};

#ifdef __STDC__
	long int lrintf(float x)
#else
	long int lrintf(x)
	float x;
#endif
{
#ifdef __MATH_SOFT_FLOAT__
  __int32_t j0,sx;
  __uint32_t i0;
  float t;
  volatile float w;
  long int result;

  GET_FLOAT_WORD(i0,x);

  /* Extract sign bit. */
  sx = (i0 >> 31);

  /* Extract exponent field. */
  j0 = ((i0 & 0x7f800000) >> 23) - 127;
  
  if (j0 < (int)(sizeof (long int) * 8) - 1)
    {
      if (j0 < -1)
        return 0;
      else if (j0 >= 23)
        result = (long int) ((i0 & 0x7fffff) | 0x800000) << (j0 - 23);
      else
        {
          w = TWO23[sx] + x;
          t = w - TWO23[sx];
          GET_FLOAT_WORD (i0, t);
          /* Detect the all-zeros representation of plus and
             minus zero, which fails the calculation below. */
          if ((i0 & ~(1L << 31)) == 0)
              return 0;
          j0 = ((i0 >> 23) & 0xff) - 0x7f;
          i0 &= 0x7fffff;
          i0 |= 0x800000;
          result = i0 >> (23 - j0);
        }
    }
  else
    {
      return (long int) x;
    }
  return sx ? -result : result;
#else /* __MATH_SOFT_FLOAT__ */
  float xint;
  long int retval;

#ifdef __MATH_MATCH_x86__
  __uint32_t state;  
  __inst_conv_W_S_state (xint, x, state);
#else /* __MATH_MATCH_x86__ */
  __inst_conv_W_S (xint, x);
#endif /* __MATH_MATCH_x86__ */

  GET_FLOAT_WORD (retval,xint);

#ifdef __MATH_MATCH_x86__
  if (state & FCSR_CAUSE_INVALIDOP_BIT)
    retval++;
#endif /* __MATH_MATCH_x86__ */

  return (retval);
#endif /* __MATH_SOFT_FLOAT__ */

}

#ifdef _DOUBLE_IS_32BITS

#ifdef __STDC__
	long int lrint(double x)
#else
	long int lrint(x)
	double x;
#endif
{
  return lrintf((float) x);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
