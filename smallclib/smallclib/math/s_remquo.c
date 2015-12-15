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
* 		  file : $RCSfile: s_remquo.c,v $ 
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
/*
FUNCTION
<<remquo>>, <<remquof>>--remainder and part of quotient
INDEX
	remquo
INDEX
	remquof

ANSI_SYNOPSIS
	#include <math.h>
	double remquo(double <[x]>, double <[y]>, int *<[quo]>);
	float remquof(float <[x]>, float <[y]>, int *<[quo]>);

DESCRIPTION
The <<remquo>> functions compute the same remainder as the <<remainder>>
functions; this value is in the range -<[y]>/2 ... +<[y]>/2.  In the object
pointed to by <<quo>> they store a value whose sign is the sign of <<x>>/<<y>>
and whose magnitude is congruent modulo 2**n to the magnitude of the integral
quotient of <<x>>/<<y>>.  (That is, <<quo>> is given the n lsbs of the
quotient, not counting the sign.)  This implementation uses n=31 if int is 32
bits or more, otherwise, n is 1 less than the width of int.

For example:
.	remquo(-29.0, 3.0, &<[quo]>)
returns -1.0 and sets <[quo]>=10, and
.	remquo(-98307.0, 3.0, &<[quo]>)
returns -0.0 and sets <[quo]>=-32769, although for 16-bit int, <[quo]>=-1.  In
the latter case, the actual quotient of -(32769=0x8001) is reduced to -1
because of the 15-bit limitation for the quotient.

RETURNS
When either argument is NaN, NaN is returned.  If <[y]> is 0 or <[x]> is
infinite (and neither is NaN), a domain error occurs (i.e. the "invalid"
floating point exception is raised or errno is set to EDOM), and NaN is
returned.
Otherwise, the <<remquo>> functions return <[x]> REM <[y]>.

BUGS
IEEE754-2008 calls for <<remquo>>(subnormal, inf) to cause the "underflow"
floating-point exception.  This implementation does not.

PORTABILITY
C99, POSIX.

*/

#include <limits.h>
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
 * because we wind up computing all the integer bits of the quotient anyway as
 * a side-effect of computing the remainder by the shift and subtract
 * method.  In practice, this is far more bits than are needed to use
 * remquo in reduction algorithms.
 */
double
remquo(double x, double y, int *quo)
{
	__int32_t n,hx,hy,hz,ix,iy,sx;
	__uint32_t lx,ly,lz,q=0,sxy;
	double half;

	EXTRACT_WORDS(hx,lx,x);
	EXTRACT_WORDS(hy,ly,y);
	sxy = (hx ^ hy) & 0x80000000;
	sx = hx&0x80000000;		/* sign of x */
	hx ^=sx;		/* |x| */
	hy &= 0x7fffffff;	/* |y| */
	q=0;

    /* purge off exception values */
#ifdef __MATH_NONFINITE__
	if((hy|ly)==0 || (hx>=0x7ff00000)||	/* y=0,or x not finite */
	  ((hy|((ly|-ly)>>31))>0x7ff00000))  {	/* or y is NaN */
#else /* __MATH_NONFINITE__ */
	if((hy|ly)==0) {	/* y=0 */
#endif /* __MATH_NONFINITE__ */
	    *quo = 0;	/* Not necessary, but return consistent value */
#ifndef __MATH_MATCH_x86__
	    if ((hy|ly) == 0)
		errno=EDOM;
#endif /* __MATH_MATCH_x86__ */
	    return (x*y)/(x*y);
	}
	
	if(hx<=hy) {
	    if((hx<hy)||(lx<ly)) {
		goto fixup;	/* |x|<|y| return x or x-y */
	    }
	    if(lx==ly) {
		    q=1;x=x-x; /* x==y, return 0 with sign of x */
		    goto fixup_final;
	    }
	}

    /* determine ix = ilogb(x) */
#ifdef __MATH_SUBNORMAL__
	if(hx<0x00100000) {	/* subnormal x */
	    __int32_t i;
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
	    } else {
		for (ix = -1022,i=(hx<<11); i>0; i<<=1) ix -=1;
	    }
#endif /* __HW_COUNT_LEAD_ZEROS__ */
	} else 
#endif /* __MATH_SUBNORMAL__ */
	    ix = (hx>>20)-1023;

    /* determine iy = ilogb(y) */
#ifdef __MATH_SUBNORMAL__
	if(hy<0x00100000) {	/* subnormal y */
	    __int32_t i;
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
	}
	else
#endif /* __MATH_SUBNORMAL__ */
#ifdef __HW_SET_BITS__
	    {__inst_set_hi_bits(hx,1,12);}
#else /* __HW_SET_BITS__ */
	    hx = 0x00100000|(0x000fffff&hx);
#endif /* __HW_SET_BITS__ */

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
#ifdef __HW_SET_BITS__
	    {__inst_set_hi_bits(hy,1,12);}
#else /* __HW_SET_BITS__ */
	    hy = 0x00100000|(0x000fffff&hy);
#endif /* __HW_SET_BITS__ */

    /* fix point fmod */
	n = ix - iy;
	while(n--) {
	    hz=hx-hy;lz=lx-ly; if(lx<ly) hz -= 1;
	    if(hz<0){hz=hx;lz=lx;q--;}
	    hx = hz+hz+(lz>>31); lx = lz+lz; q++;
	    q <<= 1;
	}
	hz=hx-hy;lz=lx-ly; if(lx<ly) hz -= 1;
	if(hz>=0) {hx=hz;lx=lz;q++;}

    /* convert back to floating value and restore the sign */
	if((hx|lx)==0) {			/* return sign(x)*0 */
	    goto fixup;
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
	} else {		/* subnormal output */
	    n = -1022 - iy;
	    if(n<=20) {
		lx = (lx>>n)|((__uint32_t)hx<<(32-n));
		hx >>= n;
	    } else if (n<=31) {
		lx = (hx<<(32-n))|(lx>>n); hx = sx;
	    } else {
		lx = hx>>(n-32); hx = sx;
	    }
	}
fixup:
	INSERT_WORDS(x,hx,lx);
#ifdef __MATH_SOFT_FLOAT__
	y = fabs(y);
#else /* __MATH_SOFT_FLOAT__ */
	__inst_abs_D (y,y);
#endif /* __MATH_SOFT_FLOAT__ */

#ifdef __MATH_CONST_FROM_MEMORY__ 
	half=0.5;
#else /*  __MATH_CONST_FROM_MEMORY__  */
	__inst_ldi_D_H (half, 0x3fe00000);
#endif /* __MATH_CONST_FROM_MEMORY__  */

	if (y < 0x1p-1021) {
	    if (x+x>y || (x+x==y && (q & 1))) {
		q++;		
		EXTRACT_WORDS (hy,ly,y);
		x-=y;
	    }
	} else if (x>half*y || (x==half*y && (q & 1))) {
	    q++;
	    x-=y;
	}
fixup_final:
	GET_HIGH_WORD(hx,x);
	SET_HIGH_WORD(x,hx^sx);
	q &= QUO_MASK;
	*quo = (sxy ? -q : q);
	return x;
}
