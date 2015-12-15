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
* 		  file : $RCSfile: w_log.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* From Newlib 2.0, removed _LIB_VERSION based error-handling, 
   support only POSIX */

/* @(#)w_log.c 5.1 93/09/24 */
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
       <<log>>, <<logf>>---natural logarithms

INDEX
    log
INDEX
    logf

ANSI_SYNOPSIS
       #include <math.h>
       double log(double <[x]>);
       float logf(float <[x]>);

TRAD_SYNOPSIS
       #include <math.h>
       double log(<[x]>);
       double <[x]>;

       float logf(<[x]>);
       float <[x]>;

DESCRIPTION
Return the natural logarithm of <[x]>, that is, its logarithm base e
(where e is the base of the natural system of logarithms, 2.71828@dots{}).
<<log>> and <<logf>> are identical save for the return and argument types.

You can use the (non-ANSI) function <<matherr>> to specify error
handling for these functions. 

RETURNS
Normally, returns the calculated value.  When <[x]> is zero, the
returned value is <<-HUGE_VAL>> and <<errno>> is set to <<ERANGE>>.
When <[x]> is negative, the returned value is NaN (not a number) and
<<errno>> is set to <<EDOM>>.  You can control the error behavior via
<<matherr>>.

PORTABILITY
<<log>> is ANSI. <<logf>> is an extension.
*/

/*
 * wrapper log(x)
 */

#include "low/_math.h"
#include "low/_fpuinst.h"
#include <errno.h>

#define isnan_bits(hx,lx) (((int)(hx-0x7ff00000)>=0) && (((hx-0x7ff00000)|lx)!=0))
#define isinf_bits(hx,lx) ((sx==0x7ff00000) && (lx==0))

#ifndef _DOUBLE_IS_32BITS

#ifdef __STDC__
	double log(double x)		/* wrapper log */
#else
	double log(x)			/* wrapper log */
	double x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_log(x);
#else
	double retval;
	__uint32_t hx,lx,sx;
	EXTRACT_WORDS(sx,lx,x);

	hx=sx&0x7fffffff;

#ifdef __MATH_NONFINITE__
	if (isnan_bits(hx,lx))
	    return x;
	if (isinf_bits(sx,lx))
		 return x+x;

#endif /* __MATH_NONFINITE__ */

	if (((int)sx>=0) && ((hx|lx)!=0))
	    return __ieee754_log(x);

	errno = ((hx|lx)==0)?ERANGE:EDOM;

	if ((hx|lx)==0)
	{
		/* -2^54 / 0.0  */
#ifdef __MATH_SOFT_FLOAT__
		INSERT_WORDS(retval, 0xc3500000, 0);
#else /* __MATH_SOFT_FLOAT__ */
		__inst_ldi_D_H(retval,0xc3500000);
#endif /* __MATH_SOFT_FLOAT__ */
		retval/=0.0f;
	}
	else
	{
		retval=(x-x)/0.0;
	}
        return retval; 
#endif
}

#endif /* defined(_DOUBLE_IS_32BITS) */
