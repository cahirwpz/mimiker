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
*              file : $RCSfile: sf_erf.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

/* sf_erf.c -- float version of s_erf.c.
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

static const float
tiny	    = 1e-30,
half=  5.0000000000e-01, /* 0x3F000000 */
one =  1.0000000000e+00, /* 0x3F800000 */
two =  2.0000000000e+00, /* 0x40000000 */
	/* c = (subfloat)0.84506291151 */
erx =  8.4506291151e-01; /* 0x3f58560b */

extern const float __coeff1f[];
extern const float __coeff2f[];
extern const float __coeff3f[];
extern const float __coeff4f[];

float erfcf(float x)
{
  __int32_t hx,ix;
  float s,y,z,r;
  GET_FLOAT_WORD(hx,x);
  ix = hx&0x7fffffff;

#ifdef __MATH_NONFINITE__
  if(!FLT_UWORD_IS_FINITE(ix))
    {	/* erfc(nan)=nan */
	/* erfc(+-inf)=0,2 */
      return (float)(((__uint32_t)hx>>31)<<1)+one/x;
    }
#endif /* __MATH_NONFINITE__ */

  if(ix < 0x3f580000)
    {		/* |x|<0.84375 */
      if(ix < 0x23800000)  	/* |x|<2**-56 */
	return one-x;

      z = x*x;
      y = ___poly2f (z,5+1,5+5,__coeff1f,0);

      if(hx < 0x3e800000)
        {  	/* x<1/4 */
          return one-(x+x*y);
        }
      else
        {
	  r = x*y;
	  r += (x-half);
          return half - r ;
        }
    }
  if(ix < 0x3fa00000)
    {		/* 0.84375 <= |x| < 1.25 */
#ifdef __MATH_SOFT_FLOAT__
      x = fabsf(x);
#else /* __MATH_SOFT_FLOAT__ */
      __inst_abs_S (x,x);
#endif /* __MATH_SOFT_FLOAT__ */
      s = x-one;
      s = ___poly2f (s,7+1,7+6,__coeff2f,0);

      if(hx>=0)
        {
          z  = one-erx;
	  return z - s;
        }
      else
        {
          z = erx+s;
	  return one+z;
        }
    }

  if (ix < 0x41e00000)
    {		/* |x|<28 */
#ifdef __MATH_SOFT_FLOAT__
      x = fabsf(x);
#else /* __MATH_SOFT_FLOAT__ */
      __inst_abs_S (x,x);
#endif /* __MATH_SOFT_FLOAT__ */
      s = one/(x*x);

      if(ix< 0x4036DB6D)
        {	/* |x| < 1/.35 ~ 2.857143*/
	  s = ___poly2f (s,8+1,8+8,__coeff3f,0);
        }
      else
        {			/* |x| >= 1/.35 ~ 2.857143 */
	  if(hx<0&&ix>=0x40c00000)
            return two-tiny;/* x < -6 */

	  s = ___poly2f (s,7+1,7+7,__coeff4f,0);
        }

      GET_FLOAT_WORD(ix,x);
      SET_FLOAT_WORD(z,ix&0xfffff000);
      r  =  __ieee754_expf(-z*z-(float)0.5625)*
         	__ieee754_expf((z-x)*(z+x)+s);

      if(hx>0)
        return r/x;
      else
        return two-r/x;
    }
  else
    {
      if(hx>0)
        return tiny*tiny;
      else
        return two-tiny;
    }
}
