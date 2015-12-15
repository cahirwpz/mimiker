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
* 		  file : $RCSfile: s_log10.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* From Newlib 2.0. Removed NaN/Inf handling for tiny version.
Use INS for setting register bits
Merged wrapper and ieee754(core) function */

/* @(#)e_log10.c 5.1 93/09/24 */
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

/* __ieee754_log10(x)
 * Return the base 10 logarithm of x
 * 
 * Method :
 *	Let log10_2hi = leading 40 bits of log10(2) and
 *	    log10_2lo = log10(2) - log10_2hi,
 *	    ivln10   = 1/log(10) rounded.
 *	Then
 *		n = ilogb(x), 
 *		if(n<0)  n = n+1;
 *		x = scalbn(x,-n);
 *		log10(x) := n*log10_2hi + (n*log10_2lo + ivln10*log(x))
 *
 * Note 1:
 *	To guarantee log10(10**n)=n, where 10**n is normal, the rounding 
 *	mode must set to Round-to-Nearest.
 * Note 2:
 *	[1/log(10)] rounded to 53 bits has error  .198   ulps;
 *	log10 is monotonic at all binary break points.
 *
 * Special cases:
 *	log10(x) is NaN with signal if x < 0; 
 *	log10(+INF) is +INF with no signal; log10(0) is -INF with signal;
 *	log10(NaN) is that NaN with no signal;
 *	log10(10**N) = N  for N=0,1,...,22.
 *
 * Constants:
 * The hexadecimal values are the intended ones for the following constants.
 * The decimal values may be used, provided that the compiler will convert
 * from decimal to binary accurately enough to produce the hexadecimal values
 * shown.
 */

#include "low/_math.h"
#include "low/_gpinst.h"
#include "low/_fpuinst.h"
#include <errno.h>

#ifndef _DOUBLE_IS_32BITS

#ifdef __STDC__
static const double
#else
static double
#endif
ivln10     =  4.34294481903251816668e-01, /* 0x3FDBCB7B, 0x1526E50E */
log10_2hi  =  3.01029995663611771306e-01, /* 0x3FD34413, 0x509F6000 */
log10_2lo  =  3.69423907715893078616e-13; /* 0x3D59FEF3, 0x11F12B36 */

#define isnan_bits(hx,lx) (((int)(hx-0x7ff00000)>=0) && (((hx-0x7ff00000)|lx)!=0))
#define isinf_bits(hx,lx) (((hx-0x7ff00000)|lx)==0)

#ifdef __STDC__
	double log10(double x)
#else
	double log10(x)
	double x;
#endif
{
	double y,z;
	__int32_t i,k,hx;
	__uint32_t lx,ix;

	EXTRACT_WORDS(hx,lx,x);
	ix=hx&0x7fffffff;

#ifdef __MATH_NONFINITE__ 
	if (isnan_bits(ix,lx))
	    return x;
	if (isinf_bits(hx,lx))
	    return x+x;
#endif /* __MATH_NONFINITE__ */

	if ((hx<0) || (ix|lx)==0) {
	    double retval;
	    errno = ((ix|lx)==0)?ERANGE:EDOM;
	    if ((ix|lx)==0) {
		/* -2^54 / 0.0  */
#ifdef __MATH_SOFT_FLOAT__
		INSERT_WORDS(retval, 0xc3500000, 0);
#else /* __MATH_SOFT_FLOAT__ */
		__inst_ldi_D_H(retval,0xc3500000);
#endif /* __MATH_SOFT_FLOAT__ */
		retval/=0.0f;
	    }
	    else
		retval=(x-x)/0.0;
	    return retval;
	}

        k=0;

#ifdef __MATH_SUBNORMAL__
        if (hx < 0x00100000) {                  /* x < 2**-1022  */
	    double two54;
#ifdef __MATH_SOFT_FLOAT__
	    INSERT_WORDS(two54, 0x43500000, 0);
#else /* __MATH_SOFT_FLOAT__ */
	    __inst_ldi_D_H(two54, 0x43500000);
#endif /* __MATH_SOFT_FLOAT__ */
	  k -= 54; x *= two54; /* subnormal number, scale up x */
	  GET_HIGH_WORD(hx,x);
        }
#endif /* __MATH_SUBNORMAL__ */

	k += (hx>>20)-1023;

	i  = ((__uint32_t)k&0x80000000)>>31;
#ifdef __HW_SET_BITS__
	__inst_set_hi_bits (hx, (0x3ff-i), 12);
#else /* __HW_SET_BITS__ */
        hx = (hx&0x000fffff)|((0x3ff-i)<<20);
#endif /* __HW_SET_BITS__ */
        y  = (double)(k+i);
	SET_HIGH_WORD(x,hx);
	z  = y*log10_2lo + ivln10*__ieee754_log(x);
	return  z+y*log10_2hi;
}

#endif /* defined(_DOUBLE_IS_32BITS) */
