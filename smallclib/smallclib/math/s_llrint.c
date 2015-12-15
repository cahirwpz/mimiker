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
* 		  file : $RCSfile: s_llrint.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* Soft-float version from Newlib 2.0, optimized FPU version from scratch */

/* lrint adapted to be llrint for Newlib, 2009 by Craig Howland.  */
/* @(#)s_lrint.c 5.1 93/09/24 */
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
 * llrint(x)
 * Return x rounded to integral value according to the prevailing
 * rounding mode.
 * Method:
 *	Using floating addition.
 * Exception:
 *	Inexact flag raised if x not equal to llrint(x).
 */

#include "low/_math.h"
#include "low/_fpuinst.h"
#include "low/_machine.inc"

typedef union 
{
  long long int llvalue;
  double dvalue;
} __long64;


#ifndef _DOUBLE_IS_32BITS

#ifdef __STDC__
static const double
#else
static double 
#endif

/* Adding a double, x, to 2^52 will cause the result to be rounded based on
   the fractional part of x, according to the implementation's current rounding
   mode.  2^52 is the smallest double that can be represented using all 52 significant
   digits. */
TWO52[2]={
  4.50359962737049600000e+15, /* 0x43300000, 0x00000000 */
 -4.50359962737049600000e+15, /* 0xC3300000, 0x00000000 */
};

long long int
#ifdef __STDC__
	llrint(double x)
#else
	llrint(x)
	double x;
#endif
{
#ifdef __MATH_SOFT_FLOAT__
  __int32_t i0,j0,sx;
  __uint32_t i1;
  double t;
  volatile double w;
  long long int result;
  
  EXTRACT_WORDS(i0,i1,x);

  /* Extract sign bit. */
  sx = (i0>>31)&1;

  /* Extract exponent field. */
  j0 = ((i0 & 0x7ff00000) >> 20) - 1023;
  /* j0 in [-1023,1024] */
  
  if(j0 < 20)
    {
      /* j0 in [-1023,19] */
      if(j0 < -1)
        return 0;
      else
        {
          /* j0 in [0,19] */
	  /* shift amt in [0,19] */
          w = TWO52[sx] + x;
          t = w - TWO52[sx];
          GET_HIGH_WORD(i0, t);
          /* Detect the all-zeros representation of plus and
             minus zero, which fails the calculation below. */
          if ((i0 & ~((__int32_t)1 << 31)) == 0)
              return 0;
          /* After round:  j0 in [0,20] */
          j0 = ((i0 & 0x7ff00000) >> 20) - 1023;
          i0 &= 0x000fffff;
          i0 |= 0x00100000;
	  /* shift amt in [20,0] */
          result = i0 >> (20 - j0);
        }
    }
  else if (j0 < (int)(8 * sizeof (long long int)) - 1)
    {
      /* 64bit return: j0 in [20,62] */
      if (j0 >= 52)
	/* 64bit return: j0 in [52,62] */
	/* 64bit return: left shift amt in [32,42] */
        result = ((long long int) ((i0 & 0x000fffff) | 0x0010000) << (j0 - 20)) | 
		/* 64bit return: right shift amt in [0,10] */
                   (i1 << (j0 - 52));
      else
        {
	  /* 64bit return: j0 in [20,51] */
          w = TWO52[sx] + x;
          t = w - TWO52[sx];
          EXTRACT_WORDS (i0, i1, t);
          j0 = ((i0 & 0x7ff00000) >> 20) - 1023;
          i0 &= 0x000fffff;
          i0 |= 0x00100000;
          /* After round:
	   * 64bit return: j0 in [20,52] */
	  /* 64bit return: left shift amt in [0,32] */
          /* ***64bit return: right shift amt in [32,0] */
          result = ((long long int) i0 << (j0 - 20))
			| SAFE_RIGHT_SHIFT (i1, (52 - j0));
        }
    }
  else
    {
      return (long long int) x;
    }
  
  return sx ? -result : result;
#else /* __MATH_SOFT_FLOAT__ */
  double xint;
  long long int retval;
  
#ifdef __MATH_MATCH_x86__
  __uint32_t state;  
  __inst_conv_L_D_state (xint, x, state);
#else /* __MATH_MATCH_x86__ */
  __inst_conv_L_D (xint, x);
#endif /* __MATH_MATCH_x86__ */

  EXTRACT_LLWORD (retval,xint);

#ifdef __MATH_MATCH_x86__
  if (state & FCSR_CAUSE_INVALIDOP_BIT)
    retval++;
#endif /* __MATH_MATCH_x86__ */

  return (retval);
#endif /* __MATH_SOFT_FLOAT__ */
}

#endif /* _DOUBLE_IS_32BITS */
