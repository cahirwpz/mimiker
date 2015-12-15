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
Wrapper code absorbed in to function. */

/* @(#)e_fmod.c 5.1 93/09/24 */
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
 * fmod(x,y)
 * Return x mod y in exact arithmetic
 * Method: shift and subtract
 */

#include "low/_math.h"
#include "low/_gpinst.h"
#include "low/_fpuinst.h"
#include <errno.h>

#ifndef _DOUBLE_IS_32BITS

#if defined (__MATH_EXCEPTION__)
#ifdef __STDC__
static const double one = 1.0;
#else
static double one = 1.0;
#endif
#endif /* (__MATH_EXCEPTION__) */

#define isnan_bits(hx,lx) (((int)(hx-0x7ff00000)>=0) && (((hx-0x7ff00000)|lx)!=0))

#ifdef __STDC__
	double fmod(double x, double y)
#else
	double fmod(x,y)
	double x,y ;
#endif
{
	__int32_t n,hx,hy,hz,ix,iy,sx;
	__uint32_t lx,ly,lz;
#ifdef __MATH_SUBNORMAL__
	__int32_t i;
#endif /* __MATH_SUBNORMAL__ */

	EXTRACT_WORDS(hx,lx,x);
	EXTRACT_WORDS(hy,ly,y);
	sx = hx&0x80000000;		/* sign of x */
	hx ^=sx;		/* |x| */
	hy &= 0x7fffffff;	/* |y| */

#ifdef __MATH_NONFINITE__
    /* purge off exception values */
	if (isnan_bits(hx,lx) || isnan_bits(hy,ly))
	    return x+y;
#endif /* __MATH_NONFINITE__ */

	if((hy|ly)==0||(hx==0x7ff00000))	/* y = 0 or x = INF */
	{
		errno=EDOM;
		return (x*y)/(x*y);
	}

	if(hx<=hy) {
	    if((hx<hy)||(lx<ly)) return x;	/* |x|<|y| return x */
	    if(lx==ly) { 
		INSERT_WORDS(x,sx,0);
		return x;
	    }
	}

    /* determine ix = ilogb(x) */
#ifdef __MATH_SUBNORMAL__
	if(hx<0x00100000) {	/* subnormal x */
#ifdef __HW_COUNT_LEAD_ZEROS__
	    __uint32_t temp;
	    ix=-1043;
	    i=lx;
	    if(hx!=0) {
		i=hx<<11; ix+=(1043-1022);
	    }
	    __inst_count_lead_zeros(temp,i);
	    ix=ix-temp;	
#else /* __HW_COUNT_LEAD_ZEROS__ */
	    if(hx==0) {
		for (ix = -1043, i=lx; i>0; i<<=1) ix -=1;
	    }
	    else {
		for (ix = -1022,i=(hx<<11); i>0; i<<=1) ix -=1;
	    }
#endif /* __HW_COUNT_LEAD_ZEROS__ */
	} else 
#endif /*  __MATH_SUBNORMAL__ */
	    ix = (hx>>20)-1023;

    /* determine iy = ilogb(y) */
#ifdef __MATH_SUBNORMAL__
	if(hy<0x00100000) {	/* subnormal y */
#ifdef __HW_COUNT_LEAD_ZEROS__
	    __uint32_t temp;
	    iy=-1043;
	    i=ly;
	    if(hy!=0) {
		i=hy<<11; iy+=(1043-1022);
	    }
	    __inst_count_lead_zeros(temp,i);
	    iy=iy-temp;	
#else /* __HW_COUNT_LEAD_ZEROS__ */
	    if(hy==0) {
		for (iy = -1043, i=ly; i>0; i<<=1) iy -=1;
	    } else {
		for (iy = -1022,i=(hy<<11); i>0; i<<=1) iy -=1;
	    }
#endif /* __HW_COUNT_LEAD_ZEROS__ */
	} else 
#endif /* __MATH_SUBNORMAL__ */
		iy = (hy>>20)-1023;

    /* set up {hx,lx}, {hy,ly} and align y to x */
#ifdef __MATH_SUBNORMAL__
	if(ix < -1022) {		/* subnormal x, shift x to normal */
	    n = -1022-ix;
	    if(n<=31) {
	        hx = (hx<<n)|(lx>>(32-n));
	        lx <<= n;
	    } else {
		hx = lx<<(n-32);
		lx = 0;
	    }
	} else
#endif /* __MATH_SUBNORMAL__ */
	    hx = 0x00100000|(0x000fffff&hx);

#ifdef __MATH_SUBNORMAL__
	if(iy < -1022) {		/* subnormal y, shift y to normal */
	    n = -1022-iy;
	    if(n<=31) {
	        hy = (hy<<n)|(ly>>(32-n));
	        ly <<= n;
	    } else {
		hy = ly<<(n-32);
		ly = 0;
	    }
	}
	else
#endif /* __MATH_SUBNORMAL__ */
	    hy = 0x00100000|(0x000fffff&hy);

    /* fix point fmod */
	n = ix - iy;
	while(n--) {
	    hz=hx-hy;lz=lx-ly; if(lx<ly) hz -= 1;
	    if(hz<0){hz=hx; lz=lx;}
#ifdef __MATH_SPEED_OVER_SIZE__ 
	 /* check for early exit, does not affect code size for micromips */
	    else if((hz|lz)==0) { 		/* return sign(x)*0 */
		INSERT_WORDS (x,sx,0); 	/* |x|=|y| return x*0*/
		return x;
	    }
#endif /* __MATH_SPEED_OVER_SIZE__ */

	    hx = hz+hz+(lz>>31); lx = lz+lz;
	}

	hz=hx-hy;lz=lx-ly; if(lx<ly) hz -= 1;
	if(hz>=0) {hx=hz;lx=lz;}

    /* convert back to floating value and restore the sign */
	if((hx|lx)==0)  {      		/* return sign(x)*0 */
	    INSERT_WORDS (x,sx,0); 	/* |x|=|y| return x*0*/
	    return x;
	}

	while(hx<0x00100000) {		/* normalize x */
	    hx = hx+hx+(lx>>31); lx = lx+lx;
	    iy -= 1;
	}	

	if(iy>= -1022) {	/* normalize output */
#ifdef __HW_SET_BITS__
	    hx=hx-0x100000;
	    __inst_set_bits (hx, (iy+1023), 20, 11);
#else /* __HW_SET_BITS__ */
	    hx = ((hx-0x00100000)|((iy+1023)<<20));
#endif /* __HW_SET_BITS__ */

	    INSERT_WORDS(x,hx|sx,lx);
	}
	else {		/* subnormal output */
	    n = -1022 - iy;
	    if(n<=20) {
		lx = (lx>>n)|((__uint32_t)hx<<(32-n));
		hx >>= n;
	    } else if (n<=31) {
		lx = (hx<<(32-n))|(lx>>n); hx = sx;
	    } else {
		lx = hx>>(n-32); hx = sx;
	    }
	    INSERT_WORDS(x,hx|sx,lx);
#ifdef __MATH_EXCEPTION__
	    x *= one;		/* create necessary signal */
#endif /* __MATH_EXCEPTION__ */
	}
	return x;		/* exact output */
}

#endif /* defined(_DOUBLE_IS_32BITS) */
