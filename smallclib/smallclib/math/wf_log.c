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
* 		  file : $RCSfile: wf_log.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* From Newlib 2.0, removed _LIB_VERSION based error-handling, 
   support only POSIX */

/* wf_log.c -- float version of w_log.c.
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

/*
 * wrapper logf(x)
 */

#include "low/_math.h"
#include <errno.h>
#include "low/_fpuinst.h"

#define isinf_bits(hx) (hx==0x7f800000)
#define isnan_bits(hx) (hx>0x7f800000)

#ifdef __STDC__
	float logf(float x)		/* wrapper logf */
#else
	float logf(x)			/* wrapper logf */
	float x;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_logf(x);
#else
	float retval;
	__uint32_t hx,sx;
	GET_FLOAT_WORD(sx,x);
	hx=sx&0x7fffffff;

#ifdef __MATH_NONFINITE__ 
	if(isnan_bits(hx))
	    return x;
	if (isinf_bits(sx))
		 return x+x;
#endif /* __MATH_NONFINITE__ */

	if ((int)sx > 0)
	    return __ieee754_logf(x);

	/* log(0) or log (x<0) */
	errno = (hx==0)?ERANGE:EDOM;
	if (hx==0)
	{
		/* -2^25 / 0.0  */
		__inst_ldi_S_H(retval,0xcc00);
		retval/=0.0f;
	}
	else
	{
#ifdef __MATH_EXCEPTION__
		retval=(x-x)/0.0f;
#else /* __MATH_EXCEPTION__ */
		__inst_ldi_S_H(retval,0x7fc0);
#endif /* __MATH_EXCEPTION__ */
	}
        return retval;
#endif
}

#ifdef _DOUBLE_IS_32BITS

#ifdef __STDC__
	double log(double x)
#else
	double log(x)
	double x;
#endif
{
	return (double) logf((float) x);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
