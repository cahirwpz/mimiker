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
* 		  file : $RCSfile: sf_remquo.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* From Newlib 2.0, optimized in place for size */

/* Adapted for Newlib, 2009.  (Allow for int < 32 bits; return *quo=0 during
 * errors to make test scripts easier.)  */
/* @(#)e_fmod.c 1.3 95/01/18 */
/*-
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunSoft, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice 
 * is preserved.
 * ====================================================
 */

#include <math.h>
#include <errno.h>
#include "low/_math.h"
#include "low/_gpinst.h"
#include "low/_fpuinst.h"

/* For quotient, return either all 31 bits that can from calculation (using
 * int32_t), or as many as can fit into an int that is smaller than 32 bits.  */
#ifdef __MATH_MATCH_x86__
#define QUO_MASK 0x7
#else /* __MATH_MATCH_x86__ */
#if INT_MAX > 0x7FFFFFFFL
  #define QUO_MASK 0x7FFFFFFF
# else
  #define QUO_MASK INT_MAX
#endif
#endif /* __MATH_MATCH_x86__ */

/*
 * Return the IEEE remainder and set *quo to the last n bits of the
 * quotient, rounded to the nearest integer.  We choose n=31--if that many fit--
 * we wind up computing all the integer bits of the quotient anyway as
 * a side-effect of computing the remainder by the shift and subtract
 * method.  In practice, this is far more bits than are needed to use
 * remquo in reduction algorithms.
 */
float
remquof(float x, float y, int *quo)
{
	__int32_t n,hx,hy,hz,ix,iy,sx;
	__uint32_t q,sxy;
	float half;

	GET_FLOAT_WORD(hx,x);
	GET_FLOAT_WORD(hy,y);
	sxy = (hx ^ hy) & 0x80000000;
	sx = hx&0x80000000;		/* sign of x */
	hx ^=sx;		/* |x| */
	hy &= 0x7fffffff;	/* |y| */
	q = 0;

    /* purge off exception values */
#ifdef __MATH_NONFINITE__
	if(hy==0||hx>=0x7f800000||hy>0x7f800000) { /* y=0,NaN;or x not finite */
#else /* __MATH_NONFINITE__ */
       	if (hy==0) {
#endif /* __MATH_NONFINITE__ */
	    *quo = 0;	/* Not necessary, but return consistent value */
#ifndef __MATH_MATCH_x86__
	    if (hy == 0)
		errno=EDOM;
#endif /* __MATH_MATCH_x86__ */
	    return (x*y)/(x*y);
	}

	if(hx<hy) {
	    goto fixup;	/* |x|<|y| return x or x-y */
	} else if(hx==hy) {
	    q=1; x=0; goto fixup_final;
	}

    /* determine ix = ilogb(x) */
#ifdef __MATH_SUBNORMAL__
	if(hx<0x00800000) {	/* subnormal x */
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
	if(hy<0x00800000) {	/* subnormal y */
	    __int32_t i;
#ifdef __HW_COUNT_LEAD_ZEROS__
	    __uint32_t tmp;
	    i=(hy<<8);
	    __inst_count_lead_zeros (tmp,i);
	    iy=-126-tmp;
#else /* __HW_COUNT_LEAD_ZEROS__ */
	    for (iy = -126,i=(hy<<8); i>0; i<<=1) iy -=1;
#endif /* __HW_COUNT_LEAD_ZEROS__ */
	} else 
#endif /* __MATH_SUBNORMAL__ */
	    iy = (hy>>23)-127;

    /* set up {hx,lx}, {hy,ly} and align y to x */
#ifdef __MATH_SUBNORMAL__
	if(ix < -126) {		/* subnormal x, shift x to normal */
	    n = -126-ix;
	    hx <<= n;
	} else
#endif /* __MATH_SUBNORMAL__ */
#ifdef __HW_SET_BITS__
	    {__inst_set_hi_bits(hx,1,9);}
#else /* __HW_SET_BITS__ */
	    hx = 0x00800000|(0x007fffff&hx);
#endif /* __HW_SET_BITS__ */
	

#ifdef __MATH_SUBNORMAL__
	if(iy < -126) {		/* subnormal y, shift y to normal */
	    n = -126-iy;
	    hy <<= n;
	} else
#endif /* __MATH_SUBNORMAL__ */
#ifdef __HW_SET_BITS__
	    {__inst_set_hi_bits(hy,1,9);}
#else /* __HW_SET_BITS__ */
	    hy = 0x00800000|(0x007fffff&hy);
#endif /* __HW_SET_BITS__ */

    /* fix point fmod */
	n = ix - iy;
	while(n--) {
	    hz=hx-hy;
	    if(hz<0) hx = hx << 1;
	    else {hx = hz << 1; q++;}
	    q <<= 1;
	}
	hz=hx-hy;
	if(hz>=0) {hx=hz;q++;}

    /* convert back to floating value and restore the sign */
	if(hx==0) {				/* return sign(x)*0 */
	    goto fixup;
	}

	while(hx<0x00800000) {		/* normalize x */
	    hx <<= 1;
	    iy -= 1;
	}

	if(iy>= -126) {		/* normalize output */
	    hx = ((hx-0x00800000)|((iy+127)<<23));
	} else {		/* subnormal output */
	    n = -126 - iy;
	    hx >>= n;
	}
fixup:
	SET_FLOAT_WORD(x,hx);
#ifdef __MATH_SOFT_FLOAT__
	y = fabsf(y);
#else /* __MATH_SOFT_FLOAT__ */
	__inst_abs_S (y,y);
#endif /* __MATH_SOFT_FLOAT__ */

#ifdef __MATH_CONST_FROM_MEMORY__ 
	half=0.5;
#else /*  __MATH_CONST_FROM_MEMORY__  */
	__inst_ldi_S_H (half, 0x3f00);
#endif /* __MATH_CONST_FROM_MEMORY__  */

	if (y < 0x1p-125f) {
	    if (x+x>y || (x+x==y && (q & 1))) {
		q++;
		x-=y;
	    }
	} else if (x>half*y || (x==half*y && (q & 1))) {
	    q++;
	    x-=y;
	}
fixup_final:
	GET_FLOAT_WORD(hx,x);
	SET_FLOAT_WORD(x,hx^sx);
	q &= 0x7fffffff;
	*quo = (sxy ? -q : q);
	return x;
}
