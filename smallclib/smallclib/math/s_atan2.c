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
* 		  file : $RCSfile: s_atan2.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* From Newlib 2.0, disolved switch cases to simple conditions */

/* @(#)e_atan2.c 5.1 93/09/24 */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice 
 * is preserved.
 * ====================================================
 *
 */

/* __ieee754_atan2(y,x)
 * Method :
 *	1. Reduce y to positive by atan2(y,x)=-atan2(-y,x).
 *	2. Reduce x to positive by (if x and y are unexceptional): 
 *		ARG (x+iy) = arctan(y/x)   	   ... if x > 0,
 *		ARG (x+iy) = pi - arctan[y/(-x)]   ... if x < 0,
 *
 * Special cases:
 *
 *	ATAN2((anything), NaN ) is NaN;
 *	ATAN2(NAN , (anything) ) is NaN;
 *	ATAN2(+-0, +(anything but NaN)) is +-0  ;
 *	ATAN2(+-0, -(anything but NaN)) is +-pi ;
 *	ATAN2(+-(anything but 0 and NaN), 0) is +-pi/2;
 *	ATAN2(+-(anything but INF and NaN), +INF) is +-0 ;
 *	ATAN2(+-(anything but INF and NaN), -INF) is +-pi;
 *	ATAN2(+-INF,+INF ) is +-pi/4 ;
 *	ATAN2(+-INF,-INF ) is +-3pi/4;
 *	ATAN2(+-INF, (anything but,0,NaN, and INF)) is +-pi/2;
 *
 * Constants:
 * The hexadecimal values are the intended ones for the following 
 * constants. The decimal values may be used, provided that the 
 * compiler will convert from decimal to binary accurately enough 
 * to produce the hexadecimal values shown.
 */

#include "low/_math.h"
#include "low/_fpuinst.h"

#ifndef _DOUBLE_IS_32BITS

#ifdef __STDC__
static const double 
#else
static double 
#endif
tiny  = 1.0e-300,
pi_o_4  = 7.8539816339744827900E-01, /* 0x3FE921FB, 0x54442D18 */
pi_o_2  = 1.5707963267948965580E+00, /* 0x3FF921FB, 0x54442D18 */
pi      = 3.1415926535897931160E+00, /* 0x400921FB, 0x54442D18 */
pi_lo   = 1.2246467991473531772E-16; /* 0x3CA1A626, 0x33145C07 */

#ifdef __STDC__
	double atan2(double y, double x)
#else
	double atan2(y,x)
	double  y,x;
#endif
{  
	double z, retval;
	__int32_t hx,hy,ix,iy;
	__uint32_t lx,ly;

#ifdef __MATH_SPEED_OVER_SIZE__
	__int32_t k;
#endif /* __MATH_SPEED_OVER_SIZE__ */

	EXTRACT_WORDS(hx,lx,x);
	ix = hx&0x7fffffff;
	EXTRACT_WORDS(hy,ly,y);
	iy = hy&0x7fffffff;

#ifdef __MATH_NONFINITE__
	if(((ix|((lx|-lx)>>31))>0x7ff00000)||
	   ((iy|((ly|-ly)>>31))>0x7ff00000))	/* x or y is NaN */
	   return x+y;
#endif /* __MATH_NONFINITE__ */

#ifdef __MATH_NONFINITE__
    /* when x is INF */
	if((ix==0x7ff00000) && (iy==0x7ff00000)) {
	  retval=pi_o_4;		/* atan(+INF,+INF) */
	  if (hx<0)
	    retval*=3.0;	/*atan(+INF,-INF)*/

	  retval+=tiny;
	  goto common_exit;
	}
#endif /* __MATH_NONFINITE__ */

    /* when y = 0 */
#ifdef __MATH_NONFINITE__
	if(((iy|ly)==0) || (ix==0x7ff00000))  {
#else /* __MATH_NONFINITE__ */
	if((iy|ly)==0)  {
#endif /* __MATH_NONFINITE__ */
	  retval = 0.0;
	  if (hx<0)
#ifdef __MATH_MATCH_NEWLIB_EXCEPTION__
	    retval=pi+tiny;
#else /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
	    retval=pi;
#endif /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
	  goto common_exit;
	}

	/* when y is INF  or x=0 */
#ifdef __MATH_NONFINITE__
	if(((ix|lx)==0) || (iy==0x7ff00000)) {
#else /* __MATH_NONFINITE__ */
	if((ix|lx)==0)  {
#endif /* __MATH_NONFINITE__ */
#ifdef __MATH_MATCH_NEWLIB_EXCEPTION__
	  retval=pi_o_2+tiny;
#else /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
	    retval=pi_o_2;
#endif /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
	  goto common_exit;
	}

	/* compute y/x */
#ifdef __MATH_SPEED_OVER_SIZE__
	k = (iy-ix)>>20;
	if(k > 60) {
	  double half=0.5;
	  z=pi_o_2+half*pi_lo; 	/* |y/x| >  2**60 */
	}
	else if(hx<0&&k<-60) {
	  z=0.0; 	/* |y|/x < -2**60 */
	}
	else 
#endif /* __MATH_SPEED_OVER_SIZE__ */
	{
	  z=y/x;		/* safe to do y/x */
#ifdef __MATH_SOFT_FLOAT__
	  z=fabs(z);
#else /* __MATH_SOFT_FLOAT__ */
	  __inst_abs_D (z,z);
#endif /* __MATH_SOFT_FLOAT__ */
	  z=atan(z);
	}

	if (hx >=0) {
	  retval=z;
	}
	else {
	  retval=pi-(z-pi_lo);
	}

common_exit:
	if (hy < 0)
	  retval=-retval;
	return retval;
}

#endif /* defined(_DOUBLE_IS_32BITS) */
