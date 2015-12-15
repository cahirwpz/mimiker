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
* 		  file : $RCSfile: sf_ilogb.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/



/* sf_ilogb.c -- float version of s_ilogb.c.
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

#include <limits.h>
#include "low/_math.h"
#include "low/_gpinst.h"

#ifdef __STDC__
  int ilogbf(float x)
#else /* __STDC__ */
  int ilogbf(x)
  float x;
#endif /* __STDC__ */
{
  __int32_t hx;

  GET_FLOAT_WORD(hx,x);
  hx &= 0x7fffffff;
  if(FLT_UWORD_IS_ZERO(hx))
    return FP_ILOGB0; /* ilogb(0) = special case error */

#ifdef __MATH_SUBNORMAL__
    if(FLT_UWORD_IS_SUBNORMAL(hx)) 
    {
#ifdef __HW_COUNT_LEAD_ZEROS__
      __int32_t temp;
      hx<<=8;
      __inst_count_lead_zeros(temp,hx);
      return -126-temp;
#else /* __HW_COUNT_LEAD_ZEROS__ */
      __int32_t ix;
      for (ix = -126,hx<<=8; hx>0; hx<<=1)
        ix -=1;
      return ix;
#endif /* __HW_COUNT_LEAD_ZEROS__ */
    }

#endif  /*__MATH_SUBNORMAL__ */

#ifdef __MATH_NONFINITE__ 
#if FP_ILOGBNAN != INT_MAX
  else if (FLT_UWORD_IS_NAN(hx))
    return FP_ILOGBNAN;  /* NAN */
#endif
  else if (!FLT_UWORD_IS_FINITE(hx)) 
    return INT_MAX;
#endif /*__MATH_NONFINITE__ */
  else 
    return (hx>>23)-127;
}


#ifdef _DOUBLE_IS_32BITS

#ifdef __STDC__
  int ilogb(double x)
#else /* __STDC__ */
  int ilogb(x)
  double x;
#endif /* __STDC__ */
{
  return ilogbf((float) x);
}

#endif 
