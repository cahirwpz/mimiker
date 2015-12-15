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
* 		  file : $RCSfile: sf_log10.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* From Newlib 2.0, remove NaN/Inf handling for tiny version 
Merged wrapper and ieee754(core) function */

/* ef_log10.c -- float version of e_log10.c.
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

#include <errno.h>
#include "low/_math.h"
#include "low/_gpinst.h"
#include "low/_fpuinst.h"

#ifdef __STDC__
static const float
#else
static float
#endif
ivln10     =  4.3429449201e-01, /* 0x3ede5bd9 */
log10_2hi  =  3.0102920532e-01, /* 0x3e9a2080 */
log10_2lo  =  7.9034151668e-07; /* 0x355427db */

#define isnan_bits(hx) (hx>0xff000000)
#define isinf_bits(hx) (hx==0x7f800000)


#ifdef __STDC__
	float log10f(float x)
#else
	float log10f(x)
	float x;
#endif
{
	float y,z;
	__int32_t i,k,hx,ix;

	GET_FLOAT_WORD(hx,x);
	ix=hx<<1;
#ifdef __MATH_NONFINITE__ 
	if (isnan_bits(ix))
	    return x;
	if (isinf_bits(hx))
		 return x+x;
#endif /* __MATH_NONFINITE__ */

	if (hx<=0)
	{
	    float retval;
	    /* log(0) or log (x<0) */
	    errno = (ix==0)?ERANGE:EDOM;
	    if (ix==0) {
#ifdef __MATH_EXCEPTION__
		    /* -2^25 / 0.0  */
		    __inst_ldi_S_H(retval,0xcc00);
		    retval/=0.0f;
#else /* __MATH_EXCEPTION__ */
		    retval=-HUGE_VALF;
#endif /* __MATH_EXCEPTION__ */
	    }
	    else {
#ifdef __MATH_EXCEPTION__
		    retval=(x-x)/0.0f;
#else /* __MATH_EXCEPTION__ */
		    __inst_ldi_S_H(retval,0x7fc0);
#endif /* __MATH_EXCEPTION__ */
	    }
	    return retval;
	}

        k=0;
#ifdef __MATH_SUBNORMAL__
        if (FLT_UWORD_IS_SUBNORMAL(hx)) {
	    float two25;
#ifdef  __MATH_SOFT_FLOAT__
	    SET_FLOAT_WORD(two25,0x4c000000);
#else /* __MATH_SOFT_FLOAT__ */
	    __inst_ldi_S_H(two25,0x4c00);
#endif /* __MATH_SOFT_FLOAT__ */
            k -= 25; x *= two25; /* subnormal number, scale up x */
	    GET_FLOAT_WORD(hx,x);
        }
#endif /* __MATH_SUBNORMAL__ */

	k += (hx>>23)-127;
	i  = ((__uint32_t)k&0x80000000)>>31;

#ifdef __HW_SET_BITS__
	__inst_set_hi_bits (hx, (0x7f-i), 9);
#else /* __HW_SET_BITS__ */
        hx = (hx&0x007fffff)|((0x7f-i)<<23);
#endif /* __HW_SET_BITS__ */
        y  = (float)(k+i);
	SET_FLOAT_WORD(x,hx);
	z  = y*log10_2lo + ivln10*__ieee754_logf(x);

	return  z+y*log10_2hi;
}
