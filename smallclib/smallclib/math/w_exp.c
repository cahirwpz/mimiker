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
*              file : $RCSfile: w_exp.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

/* @(#)w_exp.c 5.1 93/09/24 */
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
	<<exp>>, <<expf>>---exponential
INDEX
	exp
INDEX
	expf

ANSI_SYNOPSIS
	#include <math.h>
	double exp(double <[x]>);
	float expf(float <[x]>);

TRAD_SYNOPSIS
	#include <math.h>
	double exp(<[x]>);
	double <[x]>;

	float expf(<[x]>);
	float <[x]>;

DESCRIPTION
	<<exp>> and <<expf>> calculate the exponential of <[x]>, that is, 
	@ifnottex
	e raised to the power <[x]> (where e
	@end ifnottex
	@tex
	$e^x$ (where $e$
	@end tex
	is the base of the natural system of logarithms, approximately 2.71828).

	You can use the (non-ANSI) function <<matherr>> to specify
	error handling for these functions.

RETURNS
	On success, <<exp>> and <<expf>> return the calculated value.
	If the result underflows, the returned value is <<0>>.  If the
	result overflows, the returned value is <<HUGE_VAL>>.  In
	either case, <<errno>> is set to <<ERANGE>>.

PORTABILITY
	<<exp>> is ANSI C.  <<expf>> is an extension.

*/

/* 
 * wrapper exp(x)
 */

#include "low/_math.h"
#include <errno.h>

#ifndef _DOUBLE_IS_32BITS

#ifdef __STDC__
static const double
#else
static double
#endif
o_threshold=  7.09782712893383973096e+02,  /* 0x40862E42, 0xFEFA39EF */
u_threshold= -7.45133219101941108420e+02;  /* 0xc0874910, 0xD52D3051 */

#ifdef __STDC__
	double exp(double x)		/* wrapper exp */
#else
	double exp(x)			/* wrapper exp */
	double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_exp(x);
#else
	__int32_t hx;
	__uint32_t ix;
	GET_HIGH_WORD(hx,x);
	ix=hx&0x7fffffff;
	if((ix-0x7ff00000)>>31) {	/* x is finite */
			if(x>o_threshold) {
		/* exp(finite) overflow */
#ifndef HUGE_VAL
#define HUGE_VAL inf
	        	double inf = 0.0;
		       	SET_HIGH_WORD(inf,0x7ff00000);	/* set inf to infinite */
#endif
		 	errno = ERANGE;
		  	return HUGE_VAL;
		} else if(x<u_threshold) {	/* exp(finite) underflow */
			errno = ERANGE;
			return 0.0;
	    	} 
	} 
	return __ieee754_exp(x);
#endif
}

#endif /* defined(_DOUBLE_IS_32BITS) */
