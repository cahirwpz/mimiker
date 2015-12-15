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
*              file : $RCSfile: sf_frexp.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

/* sf_frexp.c -- float version of s_frexp.c.
 * Conversion to float by Ian Lance Taylor, Cygnus Support, ian@cygnus.com.
 */

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
#include "low/_gpinst.h"
#include "low/_fpuinst.h"

#ifdef __STDC__
	float frexpf(float x, int *eptr)
#else
	float frexpf(x, eptr)
	float x; int *eptr;
#endif
{
  __int32_t hx, ix, exp;
  GET_FLOAT_WORD(hx,x);
  ix = 0x7fffffff&hx;
  exp = 0;
#ifdef __MATH_NONFINITE__
  if(!FLT_UWORD_IS_FINITE(ix)||FLT_UWORD_IS_ZERO(ix)) {*eptr=0; return x;}	/* 0,inf,nan */
#endif /* __MATH_NONFINITE__ */

#ifdef __MATH_SUBNORMAL__
  if (FLT_UWORD_IS_SUBNORMAL(ix)) {		/* subnormal */
    float two25;
#ifdef __MATH_SOFT_FLOAT__
    SET_FLOAT_WORD(two25,0x4c000000);
#else /* __MATH_SOFT_FLOAT__ */
    __inst_ldi_S_H(two25,0x4c00);
#endif /* __MATH_SOFT_FLOAT__ */

    x *= two25;
    GET_FLOAT_WORD(hx,x);
    ix = hx&0x7fffffff;
    exp = -25;
  }
#endif /* __MATH_SUBNORMAL__ */

  *eptr = exp + ((ix>>23)-126);
#ifdef __HW_SET_BITS__
  __inst_set_bits (hx,0x7e,23,8);
#else  /* __HW_SET_BITS__ */
  hx = (hx&0x807fffff)|0x3f000000;
#endif /* __HW_SET_BITS__ */  
  SET_FLOAT_WORD(x,hx);
  return x;
}

#ifdef _DOUBLE_IS_32BITS

#ifdef __STDC__
  double frexp(double x, int *eptr)
#else
  double frexp(x, eptr)
  double x; int *eptr;
#endif
{
  return (double) frexpf((float) x, eptr);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
