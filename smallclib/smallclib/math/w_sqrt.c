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
* 		  file : $RCSfile: w_sqrt.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* From Newlib 2.0, replaced core call with mips instruction */
/* Removed _LIB_VERSION based error-handling, support only POSIX */

/* @(#)w_sqrt.c 5.1 93/09/24 */
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
	<<sqrt>>, <<sqrtf>>---positive square root

INDEX
	sqrt
INDEX
	sqrtf

ANSI_SYNOPSIS
	#include <math.h>
	double sqrt(double <[x]>);
	float  sqrtf(float <[x]>);

TRAD_SYNOPSIS
	#include <math.h>
	double sqrt(<[x]>);
	float  sqrtf(<[x]>);

DESCRIPTION
	<<sqrt>> computes the positive square root of the argument.
	You can modify error handling for this function with
	<<matherr>>.

RETURNS
	On success, the square root is returned. If <[x]> is real and
	positive, then the result is positive.  If <[x]> is real and
	negative, the global value <<errno>> is set to <<EDOM>> (domain error).


PORTABILITY
	<<sqrt>> is ANSI C.  <<sqrtf>> is an extension.
*/

/* 
 * wrapper sqrt(x)
 */

#include "low/_math.h"
#include "low/_fpuinst.h"
#include <errno.h>

#define isnan_bits(hx,lx) (((int)(hx-0x7ff00000)>=0) && (((hx-0x7ff00000)|lx)!=0))

#ifndef _DOUBLE_IS_32BITS

#ifdef __STDC__
	double sqrt(double x)		/* wrapper sqrt */
#else
	double sqrt(x)			/* wrapper sqrt */
	double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_sqrt(x);
#else
	double z;
#ifdef __MATH_NONFINITE__
	__uint32_t hx,lx;
	EXTRACT_WORDS(hx,lx,x);
	hx=hx&0x7fffffff;
	/* sqrt(Nan) = NaN */
	if (isnan_bits(hx,lx))
	  return x;
#endif /* __MATH_NONFINITE__ */

#ifdef __MATH_SOFT_FLOAT__
	z=__ieee754_sqrt (x);
#else /* __MATH_SOFT_FLOAT__ */
	__inst_sqrt_D (z, x);
#endif /* __MATH_SOFT_FLOAT__ */

	if (x < 0)
	  errno = EDOM;
	return z;
#endif
}

#endif /* defined(_DOUBLE_IS_32BITS) */
