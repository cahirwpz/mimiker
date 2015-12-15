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
* 		  file : $RCSfile: sf_tan.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* From Newlib 2.0. Modified to generate EDOM for infinite argument */

/* sf_tan.c -- float version of s_tan.c.
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

#ifdef __MATH_MATCH_x86__
#include <errno.h>
#endif /* __MATH_MATCH_x86__ */

#ifdef __MATH_FULLRANGE_REDUCTION__
/* Full range-reduction is both big and expensive. If it is enabled,
   we will forego this optimization since takes a whole lot of time
   and doesn't save that much(relatively speaking) space */
#define __MATH_SPEED_OVER_SIZE__
#endif /* __MATH_FULLRANGE_REDUCTION */

#ifdef __STDC__
	float tanf(float x)
#else
	float tanf(x)
	float x;
#endif
{
	float y[2];
	__int32_t n,ix;

	GET_FLOAT_WORD(ix,x);

    /* |x| ~< pi/4 */
	ix &= 0x7fffffff;

#ifdef __MATH_NONFINITE__ 
    /* tan(Inf or NaN) is NaN */
	if (!FLT_UWORD_IS_FINITE(ix)) {
	    /* Domain error when input is infinity */
#ifdef __MATH_MATCH_x86__
	    if (FLT_UWORD_IS_INFINITE(ix))
		errno=EDOM;
#endif /*  __MATH_MATCH_x86__ */
	    return x-x;		/* NaN */
	}
	else
#endif /* __MATH_NONFINITE__ */

    /* argument reduction needed */
	{
#ifdef __MATH_SPEED_OVER_SIZE__
	    y[0]=x; y[1]=0.0;n=0;
	    if (ix>0x3f490fda)
#endif /* __MATH_SPEED_OVER_SIZE__ */
	    n = __ieee754_rem_pio2f(x,y);
	    return __kernel_tanf(y[0],y[1],1-((n&1)<<1)); /*   1 -- n even
							      -1 -- n odd */
	}
}

#ifdef _DOUBLE_IS_32BITS

#ifdef __STDC__
	double tan(double x)
#else
	double tan(x)
	double x;
#endif
{
	return (double) tanf((float) x);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
