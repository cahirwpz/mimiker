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
* 		  file : $RCSfile: s_trunc.c,v $ 
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
<<trunc>>, <<truncf>>--round to integer, towards zero
INDEX
	trunc
INDEX
	truncf

ANSI_SYNOPSIS
	#include <math.h>
	double trunc(double <[x]>);
	float truncf(float <[x]>);

DESCRIPTION
	The <<trunc>> functions round their argument to the integer value, in
	floating format, nearest to but no larger in magnitude than the
	argument, regardless of the current rounding direction.  (While the
	"inexact" floating-point exception behavior is unspecified by the C
	standard, the <<trunc>> functions are written so that "inexact" is not
	raised if the result does not equal the argument, which behavior is as
	recommended by IEEE 754 for its related functions.)

RETURNS
<[x]> truncated to an integral value.

PORTABILITY
ANSI C, POSIX

*/

#include "low/_math.h"
#include "low/_fpuinst.h"
#include "low/_machine.inc"

#ifndef _DOUBLE_IS_32BITS

#ifdef __STDC__
	double trunc(double x)
#else
	double trunc(x)
	double x;
#endif
{
#ifdef __MATH_SOFT_FLOAT__
  int signbit;
  /* Most significant word, least significant word. */
  int msw;
  unsigned int lsw;
  int exponent_less_1023;

  EXTRACT_WORDS(msw, lsw, x);

  /* Extract sign bit. */
  signbit = msw & 0x80000000;

  /* Extract exponent field. */
  exponent_less_1023 = ((msw & 0x7ff00000) >> 20) - 1023;

  if (exponent_less_1023 < 20)
    {
      /* All significant digits are in msw. */
      if (exponent_less_1023 < 0)
        {
          /* -1 < x < 1, so result is +0 or -0. */
          INSERT_WORDS(x, signbit, 0);
        }
      else
        {
          /* All relevant fraction bits are in msw, so lsw of the result is 0. */
          INSERT_WORDS(x, signbit | (msw & ~(0x000fffff >> exponent_less_1023)), 0);
        }
    }
  else if (exponent_less_1023 > 51)
    {
#ifdef __MATH_NONFINITE__
      if (exponent_less_1023 == 1024)
        {
          /* x is infinite, or not a number, so trigger an exception. */
          return x + x;
        }
#endif /* __MATH_NONFINITE__ */
      /* All bits in the fraction fields of the msw and lsw are needed in the result. */
    }
  else
    {
      /* All fraction bits in msw are relevant.  Truncate irrelevant
         bits from lsw. */
      INSERT_WORDS(x, msw, lsw & ~(0xffffffffu >> (exponent_less_1023 - 20)));
    }
  return x;

#else /* __MATH_SOFT_FLOAT__ */
  double xtr;
  __uint32_t flags;

  __inst_trunc_D (xtr, x, flags);

  /* Argument outside valid range [-2^63,2^63], return argument */
  if (flags & FCSR_CAUSE_INVALIDOP_BIT)
    xtr=x;

  __inst_copysign_D (xtr,x);

  return (xtr);
#endif /* __MATH_SOFT_FLOAT__ */
}

#endif /* _DOUBLE_IS_32BITS */
