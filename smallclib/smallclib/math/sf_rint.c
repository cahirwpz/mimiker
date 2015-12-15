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
*              file : $RCSfile: sf_rint.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

/* sf_rint.c -- float version of s_rint.c.
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
#include "low/_fpuinst.h"
#include "low/_machine.inc"

#ifdef __STDC__
static const float
#else
static float 
#endif
TWO23[2]={
  8.3886080000e+06, /* 0x4b000000 */
 -8.3886080000e+06, /* 0xcb000000 */
};

#ifdef __STDC__
	float rintf(float x)
#else
	float rintf(x)
	float x;
#endif
{
#ifdef __MATH_SOFT_FLOAT__
  __int32_t i0,j0,sx;
  __uint32_t i,i1,ix;
  float t;
  volatile float w;
  GET_FLOAT_WORD(i0,x);
  sx = (i0>>31)&1;
  ix = (i0&0x7fffffff);
  j0 = (ix>>23)-0x7f;
  if(j0<23) {
    if(FLT_UWORD_IS_ZERO(ix))
      return x;
    if(j0<0) {
      i1 = (i0&0x07fffff);
      i0 &= 0xfff00000;
      i0 |= ((i1|-i1)>>9)&0x400000;
    } else {
      i = (0x007fffff)>>j0;
      if((i0&i)==0) return x; /* x is integral */
      i>>=1;
      if((i0&i)!=0) i0 = (i0&(~i))|((0x200000)>>j0);
    }
  } else {
    if(!FLT_UWORD_IS_FINITE(ix)) return x+x; /* inf or NaN */
    else return x;		/* x is integral */
  }
  SET_FLOAT_WORD(x,i0);
  w = TWO23[sx]+x;
  t =  w-TWO23[sx];
  GET_FLOAT_WORD(i0,t);
  ix = (i0&0x7fffffff);
  if (FLT_UWORD_IS_ZERO(ix))  {  /*  t == 0  */
    SET_FLOAT_WORD(t,(i0&0x7fffffff)|(sx<<31));  /*  assign sign to 0  */
  }
  return t;
#else /* __MATH_SOFT_FLOAT__ */

        float xfl;
        __uint32_t flags;

        __inst_rint_S(xfl,x,flags);

        /* Argument outside valid range [-2^63,2^63], or x=0 return argument */
        if (flags & FCSR_CAUSE_INVALIDOP_BIT)
            return x;

        __inst_copysign_S (xfl, x);

        return (xfl);
#endif /* __MATH_SOFT_FLOAT__ */

}

#ifdef _DOUBLE_IS_32BITS

#ifdef __STDC__
	double rint(double x)
#else
	double rint(x)
	double x;
#endif
{
  return (double) rintf((float) x);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
