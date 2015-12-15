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
*              file : $RCSfile: sf_nextafter.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

/* sf_nextafter.c -- float version of s_nextafter.c.
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

#ifdef __STDC__
	float nextafterf(float x, float y)
#else
	float nextafterf(x,y)
	float x,y;
#endif
{
  __int32_t hx,hy,ix,iy;
  GET_FLOAT_WORD(hx,x);
  GET_FLOAT_WORD(hy,y);
  ix = hx&0x7fffffff;		/* |x| */
  iy = hy&0x7fffffff;		/* |y| */
  if(FLT_UWORD_IS_NAN(ix) ||
   FLT_UWORD_IS_NAN(iy))
   return x+y;
  if(x==y) return y;		/* x=y, return y */
  if(FLT_UWORD_IS_ZERO(ix)) {		/* x == 0 */
    SET_FLOAT_WORD(x,(hy&0x80000000)|FLT_UWORD_MIN);
#ifdef __MATH_EXCEPTION__
    y = x*x;
    if(y==x) return y; else return x;	/* raise underflow flag */
#else
    return x;
#endif
  }
  if (hx>hy || (hx<0 && hy>=0))
    hx -= 1;
  else
    hx += 1;
#ifdef __MATH_EXCEPTION__
  hy = hx&0x7f800000;
  if(hy>FLT_UWORD_MAX) return x+x;	/* overflow  */
  if(hy<0x00800000) {		/* underflow */
    y = x*x;
    if(y!=x) {		/* raise underflow flag */
      SET_FLOAT_WORD(y,hx);
      return y;
    }
  }
#endif	
  SET_FLOAT_WORD(x,hx);
  return x;
}

#ifdef _DOUBLE_IS_32BITS

#ifdef __STDC__
	double nextafter(double x, double y)
#else
	double nextafter(x,y)
	double x,y;
#endif
{
  return (double) nextafterf((float) x, (float) x);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
