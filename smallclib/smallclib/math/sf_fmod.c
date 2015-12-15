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
* 		  file : $RCSfile: s_fmod.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* From Newlib 2.0, optimized in place for size.
Wrapper code absorbed in to function */

/* ef_fmod.c -- float version of e_fmod.c.
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
 * fmodf(x,y)
 * Return x mod y in exact arithmetic
 * Method: shift and subtract
 */

#include "low/_math.h"
#include "low/_gpinst.h"
#include <errno.h>

#if defined(__MATH_CONST_FROM_MEMORY__) || defined (__MATH_EXCEPTION__)
#ifdef __STDC__
static const float one = 1.0, Zero[] = {0.0, -0.0,};
#else
static float one = 1.0, Zero[] = {0.0, -0.0,};
#endif
#endif /* (__MATH_CONST_FROM_MEMORY__ || __MATH_EXCEPTION__) */

#ifdef __STDC__
	float fmodf(float x, float y)
#else
	float fmodf(x,y)
	float x,y ;
#endif
{
	__int32_t n,hx,hy,hz,ix,iy,sx;

	GET_FLOAT_WORD(hx,x);
	GET_FLOAT_WORD(hy,y);
	sx = hx&0x80000000;		/* sign of x */
	hx ^=sx;		/* |x| */
	hy &= 0x7fffffff;	/* |y| */

#ifdef __MATH_NONFINITE__
    /* purge off exception values */
	if (FLT_UWORD_IS_NAN (hx))
	    return x;
	if (FLT_UWORD_IS_NAN (hy))
	    return y;
#endif /* __MATH_NONFINITE__ */


#ifdef __MATH_NONFINITE__
	if(FLT_UWORD_IS_ZERO(hy)||
	   !FLT_UWORD_IS_FINITE(hx)) {
#else /* __MATH_NONFINITE__ */
	if(FLT_UWORD_IS_ZERO(hy)) {
#endif /* __MATH_NONFINITE__ */
	    errno=EDOM;
	    return (x*y)/(x*y);
	}

	if(hx<hy) return x;			/* |x|<|y| return x */
	if(hx==hy) {
	    SET_FLOAT_WORD (x,sx);
	    return x;
	}

    /* Note: y cannot be zero if we reach here. */

    /* determine ix = ilogb(x) */
#ifdef __MATH_SUBNORMAL__
	if(FLT_UWORD_IS_SUBNORMAL(hx)) {	/* subnormal x */
	    __int32_t i;
#ifdef __HW_COUNT_LEAD_ZEROS__
	    __uint32_t tmp;
	    i=(hx<<8);
	    __inst_count_lead_zeros (tmp,i);
	    ix=-126-tmp;
#else /* __HW_COUNT_LEAD_ZEROS__ */
	    for (ix = -126,i=(hx<<8); i>0; i<<=1) ix -=1;
#endif /* __HW_COUNT_LEAD_ZEROS__ */
	} else 
#endif /* __MATH_SUBNORMAL__ */
	    ix = (hx>>23)-127;

    /* determine iy = ilogb(y) */
#ifdef __MATH_SUBNORMAL__
	if(FLT_UWORD_IS_SUBNORMAL(hy)) {	/* subnormal y */
	    __int32_t i;
#ifdef __HW_COUNT_LEAD_ZEROS__
	    __uint32_t tmp;
	    i=(hy<<8);
	    __inst_count_lead_zeros (tmp,i);
	    iy=-126-tmp;
#else /* __HW_COUNT_LEAD_ZEROS__ */
	    for (iy = -126,i=(hy<<8); i>=0; i<<=1) iy -=1;
#endif /* __HW_COUNT_LEAD_ZEROS__ */
	} else 
#endif /* __MATH_SUBNORMAL__ */
	    iy = (hy>>23)-127;

    /* set up {hx,lx}, {hy,ly} and align y to x */
#ifdef __MATH_SUBNORMAL__
	if(ix >= -126) {
#ifdef __HW_SET_BITS__
	    __inst_set_hi_bits(hx,1,9);
#else /* __HW_SET_BITS__ */
	    hx = 0x00800000|(0x007fffff&hx);
#endif /* __HW_SET_BITS__ */
	}
	else {		/* subnormal x, shift x to normal */
	    n = -126-ix;
	    hx = hx<<n;
	}
	if(iy >= -126) {
#ifdef __HW_SET_BITS__
	    __inst_set_hi_bits(hy,1,9);
#else /* __HW_SET_BITS__ */
	    hy = 0x00800000|(0x007fffff&hy);
#endif /* __HW_SET_BITS__ */
	}
	else {		/* subnormal y, shift y to normal */
	    n = -126-iy;
	    hy = hy<<n;
	}
#else /* __MATH_SUBNORMAL__ */
#ifdef __HW_SET_BITS__
	    __inst_set_hi_bits(hx,1,9);
	    __inst_set_hi_bits(hy,1,9);
#else /* __HW_SET_BITS__ */
	    hx = 0x00800000|(0x007fffff&hx);
	    hy = 0x00800000|(0x007fffff&hy);
#endif /* __HW_SET_BITS__ */
#endif /* __MATH_SUBNORMAL__ */

    /* fix point fmod */
	n = ix - iy;
	while(n--) {
	    hz=hx-hy;
	    if(hz<0){hx = hx+hx;}
	    else {
#ifdef __MATH_SPEED_OVER_SIZE__ /* Code for early exit */
	    	if(hz==0) { 		/* return sign(x)*0 */
		    SET_FLOAT_WORD (x,sx);
		    return x;
		}
#endif /* __MATH_SPEED_OVER_SIZE__ */
	    	hx = hz+hz;
	    }
	}
	hz=hx-hy;
	if(hz>=0) {hx=hz;}	    

    /* convert back to floating value and restore the sign */
	if(hx==0) 			/* return sign(x)*0 */
	{
	    SET_FLOAT_WORD (x,sx);
	    return x;
	}

	while(hx<0x00800000) {		/* normalize x */
	    hx = hx+hx;
	    iy -= 1;
	}

	if(iy>= -126) {		/* normalize output */
#ifdef __HW_SET_BITS__
	    /* Doesn't affect micromips, but gives smaller code size in mips32 encoding
	       due to fewer instructions */
	    hx=hx-0x00800000;
	    __inst_set_bits (hx, (iy+127), 23, 8);
#else /* __HW_SET_BITS__ */
	    hx = ((hx-0x00800000)|((iy+127)<<23));
#endif /* __HW_SET_BITS__ */
	    SET_FLOAT_WORD(x,hx|sx);
	} else {		/* subnormal output */
	    /* If denormals are not supported, this code will generate a
	       zero representation.  */
	    n = -126 - iy;
	    hx >>= n;
	    SET_FLOAT_WORD(x,hx|sx);
#ifdef __MATH_EXCEPTION__
	    x *= one;		/* create necessary signal */
#endif /* __MATH_EXCEPTION__ */
	}
	return x;		/* exact output */
}
