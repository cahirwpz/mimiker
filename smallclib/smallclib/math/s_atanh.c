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
*              file : $RCSfile: s_atanh.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

/* @(#)e_atanh.c 5.1 93/09/24 */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice 
 * is preserved.
 * ====================================================
 *
 */

/* atanh(x)
 * Method :
 *    1.Reduced x to positive by atanh(-x) = -atanh(x)
 *    2.For x>=0.5
 *                  1              2x                          x
 *	atanh(x) = --- * log(1 + -------) = 0.5 * log1p(2 * --------)
 *                  2             1 - x                      1 - x
 *	
 * 	For x<0.5
 *	atanh(x) = 0.5*log1p(2x+2x*x/(1-x))
 *
 * Special cases:
 *	atanh(x) is NaN if |x| > 1 with signal;
 *	atanh(NaN) is that NaN with no signal;
 *	atanh(+-1) is +-INF with signal.
 *
 */

#include "low/_math.h"
#include "low/_fpuinst.h"
#include <errno.h>

static const double one = 1.0;

#ifdef __STDC__
	double atanh(double x)
#else
	double atanh(x)
	double x;
#endif
{
	double t;
	__int32_t hx,ix;
	
 	GET_HIGH_WORD(hx,x);
	ix = hx&0x7fffffff;

#ifdef __MATH_NONFINITE__
	{
		__uint32_t lx;
		GET_LOW_WORD(lx,x);
		if (((hx<<1) >= 0xffe00000) && (lx != 0))	/* x = NaN */
			return x;
	}
#endif /* __MATH_NONFINITE__ */

        SET_HIGH_WORD(x,ix);

	if(x>one) {
	  	errno = EDOM;
	  	return (0.0/0.0);
	}
	else if(x==one) {
#ifdef __MATH_MATCH_x86__
		/* Optional as per ISO C99, Section 7.12.5.3 */
		errno = ERANGE;
#endif /* __MATH_MATCH_x86__ */
	  	return (x/0.0);
	}
         
	if(ix<0x3fe00000) {		/* x < 0.5 */
		t = x+x;
		t = 0.5*log1p(t+t*x/(one-x));
	} else 
		t = 0.5*log1p((x+x)/(one-x));
#ifdef __MATH_SOFT_FLOAT__
	if(hx>=0) return t; else return -t;
#else /* __MATH_SOFT_FLOAT__ */
	__inst_copysign_Dx (t, hx);
	return t;
#endif /* __MATH_SOFT_FLOAT__ */
}
