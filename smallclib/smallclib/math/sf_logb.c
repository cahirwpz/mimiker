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
*       file : $RCSfile: sf_logb.c,v $ 
*         author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* 2009 for Newlib:  Sun's sf_ilogb.c converted to be sf_logb.c.  */
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

/* float logb(float x)
 * return the binary exponent of non-zero x
 * logbf(0) = -inf, raise divide-by-zero floating point exception
 * logbf(+inf|-inf) = +inf (no signal is raised)
 * logbf(NaN) = NaN (no signal is raised)
 * Per C99 recommendation, a NaN argument is returned unchanged.
 */

#include "low/_math.h"
#include "low/_gpinst.h"

float
#ifdef __STDC__
logbf(float x)
#else
logbf(x)
float x;
#endif
{
	__int32_t hx;

	GET_FLOAT_WORD(hx,x);
	hx &= 0x7fffffff;
	if(FLT_UWORD_IS_ZERO(hx))   
	{
		float  xx;
		/* arg==0:  return -inf and raise divide-by-zero exception */
		SET_FLOAT_WORD(xx,hx);	/* +0.0 */
		return -1./xx;	/* logbf(0) = -inf */
	}
#ifdef __MATH_SUBNORMAL__
  if(FLT_UWORD_IS_SUBNORMAL(hx))
  {
      __int32_t ix;
#ifdef __HW_COUNT_LEAD_ZEROS__
      hx<<=8;
      __inst_count_lead_zeros(ix,hx);
      return (float) -126-ix;
#else /* __HW_COUNT_LEAD_ZEROS__ */
      for (ix = -126,hx<<=8; hx>0; hx<<=1) 
        ix -=1;
      return (float) ix;
#endif /* __HW_COUNT_LEAD_ZEROS__ */
  }
#endif /*__MATH_SUBNORMAL__*/

#ifdef __MATH_NONFINITE__
  else if (FLT_UWORD_IS_INFINITE(hx))
    return HUGE_VALF;	/* x==+|-inf */
	else if (FLT_UWORD_IS_NAN(hx))
    return x;
#endif /*__MATH_NONFINITE__*/

	else return (float)
    ((hx>>23)-127);
}

#ifdef _DOUBLE_IS_32BITS

#ifdef __STDC__
	double logb(double x)
#else
	double logb(x)
	double x;
#endif
{
	return (double) logbf((float) x);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
