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
* 		  file : $RCSfile: k_tan.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* From Newlib 2.0, make inexact exception optional.
   fabsf call replaced with instruction */

/* @(#)k_tan.c 5.1 93/09/24 */
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

/* __kernel_tan( x, y, k )
 * kernel tan function on [-pi/4, pi/4], pi/4 ~ 0.7854
 * Input x is assumed to be bounded by ~pi/4 in magnitude.
 * Input y is the tail of x.
 * Input k indicates whether tan (if k=1) or 
 * -1/tan (if k= -1) is returned.
 *
 * Algorithm
 *	1. Since tan(-x) = -tan(x), we need only to consider positive x. 
 *	2. if x < 2^-28 (hx<0x3e300000 0), return x with inexact if x!=0.
 *	3. tan(x) is approximated by a odd polynomial of degree 27 on
 *	   [0,0.67434]
 *		  	         3             27
 *	   	tan(x) ~ x + T1*x + ... + T13*x
 *	   where
 *	
 * 	        |tan(x)         2     4            26   |     -59.2
 * 	        |----- - (1+T1*x +T2*x +.... +T13*x    )| <= 2
 * 	        |  x 					| 
 * 
 *	   Note: tan(x+y) = tan(x) + tan'(x)*y
 *		          ~ tan(x) + (1+x*x)*y
 *	   Therefore, for better accuracy in computing tan(x+y), let 
 *		     3      2      2       2       2
 *		r = x *(T2+x *(T3+x *(...+x *(T12+x *T13))))
 *	   then
 *		 		    3    2
 *		tan(x+y) = x + (T1*x + (x *(r+y)+y))
 *
 *      4. For x in [0.67434,pi/4],  let y = pi/4 - x, then
 *		tan(x) = tan(pi/4-y) = (1-tan(y))/(1+tan(y))
 *		       = 1 - 2*(tan(y) - (tan(y)^2)/(1+tan(y)))
 */

#include "low/_math.h"
#include "low/_fpuinst.h"

#ifndef _DOUBLE_IS_32BITS

#ifdef __STDC__
static const double 
#else
static double 
#endif
pio4  =  7.85398163397448278999e-01, /* 0x3FE921FB, 0x54442D18 */
pio4lo=  3.06161699786838301793e-17; /* 0x3C81A626, 0x33145C07 */

#ifdef __MATH_FORCE_EXPAND_POLY__ /* Original coefficient array, for reference */
static const double
T[] =  {
  3.33333333333334091986e-01, /* 0x3FD55555, 0x55555563 */
  1.33333333333201242699e-01, /* 0x3FC11111, 0x1110FE7A */
  5.39682539762260521377e-02, /* 0x3FABA1BA, 0x1BB341FE */
  2.18694882948595424599e-02, /* 0x3F9664F4, 0x8406D637 */
  8.86323982359930005737e-03, /* 0x3F8226E3, 0xE96E8493 */
  3.59207910759131235356e-03, /* 0x3F6D6D22, 0xC9560328 */
  1.45620945432529025516e-03, /* 0x3F57DBC8, 0xFEE08315 */
  5.88041240820264096874e-04, /* 0x3F4344D8, 0xF2F26501 */
  2.46463134818469906812e-04, /* 0x3F3026F7, 0x1A8D1068 */
  7.81794442939557092300e-05, /* 0x3F147E88, 0xA03792A6 */
  7.14072491382608190305e-05, /* 0x3F12B80F, 0x32F0A7E9 */
 -1.85586374855275456654e-05, /* 0xBEF375CB, 0xDB605373 */
  2.59073051863633712884e-05, /* 0x3EFB2A70, 0x74BF7AD4 */
};
#else /* __MATH_FORCE_EXPAND_POLY__ */
static const double
T0 = 3.33333333333334091986e-01, /* 0x3FD55555, 0x55555563 */
T11 = -1.85586374855275456654e-05, /* 0xBEF375CB, 0xDB605373 */
T12 =  2.59073051863633712884e-05, /* 0x3EFB2A70, 0x74BF7AD4 */
T[] =  {
  1.33333333333201242699e-01, /* 0x3FC11111, 0x1110FE7A */
  5.39682539762260521377e-02, /* 0x3FABA1BA, 0x1BB341FE */
  2.18694882948595424599e-02, /* 0x3F9664F4, 0x8406D637 */
  8.86323982359930005737e-03, /* 0x3F8226E3, 0xE96E8493 */
  3.59207910759131235356e-03, /* 0x3F6D6D22, 0xC9560328 */
  1.45620945432529025516e-03, /* 0x3F57DBC8, 0xFEE08315 */
  5.88041240820264096874e-04, /* 0x3F4344D8, 0xF2F26501 */
  2.46463134818469906812e-04, /* 0x3F3026F7, 0x1A8D1068 */
  7.81794442939557092300e-05, /* 0x3F147E88, 0xA03792A6 */
  7.14072491382608190305e-05, /* 0x3F12B80F, 0x32F0A7E9 */
};

static double poly(double w, double *retptr, const double T[10])
{
	int i=9;
	double retval1=T11, retval2 = T12;
	while (i > 0) {
	  retval2=w*retval2+T[i];
	  retval1=w*retval1+T[i-1];
	  i-=2;
	}
	*retptr=retval1;
	return retval2;
}
#endif /* __MATH_FORCE_EXPAND_POLY__ */


#ifdef __STDC__
	double __kernel_tan(double x, double y, int iy)
#else
	double __kernel_tan(x, y, iy)
	double x,y; int iy;
#endif
{
	double z,r,v,w,s,one;
	__int32_t ix,hx;
	GET_HIGH_WORD(hx,x);
	ix = hx&0x7fffffff;	/* high word of |x| */

#ifdef __MATH_CONST_FROM_MEMORY__
	one=1.0;
#else /* __MATH_CONST_FROM_MEMORY__ */
	__inst_ldi_D_W(one,1);
#endif /* __MATH_CONST_FROM_MEMORY__ */

	if(ix<0x3e300000)			/* x < 2**-28 */
	    {
#ifdef __MATH_MATCH_NEWLIB_EXCEPTION__
	/* Not needed for tan as per ISO standard */
		if((int)x==0)  			/* generate inexact */
#endif /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
		    {
			__uint32_t low;
			GET_LOW_WORD(low,x);
			if(((ix|low)|(iy+1))==0) {
#ifdef __MATH_SOFT_FLOAT__
			    return one/fabs(x);
#else /* __MATH_SOFT_FLOAT__ */
			    __inst_abs_D(x,x);
			    return (one/x);
#endif /* __MATH_SOFT_FLOAT__ */
			}
			else return (iy==1)? x: -one/x;
		    }
	    }
	if(ix>=0x3FE59428) { 			/* |x|>=0.6744 */
	    if(hx<0) {x = -x; y = -y;}
	    z = pio4-x;
	    w = pio4lo-y;
	    x = z+w;
	    y= x-x;
	}
	z	=  x*x;
	w 	=  z*z;
    /* Break x^5*(T[1]+x^2*T[2]+...) into
     *	  x^5(T[1]+x^4*T[3]+...+x^20*T[11]) +
     *	  x^5(x^2*(T[2]+x^4*T[4]+...+x^22*[T12]))
     */
#ifdef __MATH_FORCE_EXPAND_POLY__
	r = T[1]+w*(T[3]+w*(T[5]+w*(T[7]+w*(T[9]+w*T[11]))));
	v = z*(T[2]+w*(T[4]+w*(T[6]+w*(T[8]+w*(T[10]+w*T[12])))));
	s = z*x;
	r = y + z*(s*(r+v)+y);
	r += T[0]*s;
#else /* __MATH_FORCE_EXPAND_POLY__ */
	v = z*poly(w,&r,T);
	s = z*x;
	r = y + z*(s*(r+v)+y);
	r += T0*s;
#endif /* __MATH_FORCE_EXPAND_POLY__ */
	w = x+r;
	if(ix>=0x3FE59428) {
	    v = (double)iy;
	    return (double)(1-((hx>>30)&2))*(v-2.0*(x-(w*w/(w+v)-r)));
	}
	if(iy==1) return w;
	else {		/* if allow error up to 2 ulp, 
			   simply return -1.0/(x+r) here */
     /*  compute -1.0/(x+r) accurately */
	    double a,t;
	    z  = w;
#ifdef __MATH_SOFT_FLOAT__
	    SET_LOW_WORD(z,0);
#else /* __MATH_SOFT_FLOAT__ */
	    __inst_reset_low_D(z);
#endif /* __MATH_SOFT_FLOAT__ */
	    v  = r-(z - x); 	/* z+v = r+x */
	    t = a  = -one/w;	/* a = -1.0/w */
#ifdef __MATH_SOFT_FLOAT__
	    SET_LOW_WORD(t,0);
#else /* __MATH_SOFT_FLOAT__ */
	    __inst_reset_low_D(t);
#endif /* __MATH_SOFT_FLOAT__ */
	    s  = one+t*z;
	    return t+a*(s+t*v);
	}
}

#endif /* defined(_DOUBLE_IS_32BITS) */
