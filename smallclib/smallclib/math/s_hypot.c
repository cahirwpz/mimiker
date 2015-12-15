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
*              file : $RCSfile: e_hypot.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

/* @(#)e_hypot.c 5.1 93/09/24 */
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

/* __ieee754_hypot(x,y)
 *
 * Method :                  
 *	If (assume round-to-nearest) z=x*x+y*y 
 *	has error less than sqrt(2)/2 ulp, than 
 *	sqrt(z) has error less than 1 ulp (exercise).
 *
 *	So, compute sqrt(x*x+y*y) with some care as 
 *	follows to get the error below 1 ulp:
 *
 *	Assume x>y>0;
 *	(if possible, set rounding to round-to-nearest)
 *	1. if x > 2y  use
 *		x1*x1+(y*y+(x2*(x+x1))) for x*x+y*y
 *	where x1 = x with lower 32 bits cleared, x2 = x-x1; else
 *	2. if x <= 2y use
 *		t1*y1+((x-y)*(x-y)+(t1*y2+t2*y))
 *	where t1 = 2x with lower 32 bits cleared, t2 = 2x-t1, 
 *	y1= y with lower 32 bits chopped, y2 = y-y1.
 *		
 *	NOTE: scaling may be necessary if some argument is too 
 *	      large or too tiny
 *
 * Special cases:
 *	hypot(x,y) is INF if x or y is +INF or -INF; else
 *	hypot(x,y) is NAN if x or y is NAN.
 *
 * Accuracy:
 * 	hypot(x,y) returns sqrt(x^2+y^2) with error less 
 * 	than 1 ulps (units in the last place) 
 */

#include "low/_math.h"
#include "low/_fpuinst.h"
#include <errno.h>

#ifndef _DOUBLE_IS_32BITS

#ifdef __STDC__
	double hypot(double x, double y)
	//double __ieee754_hypot(double x, double y)
#else
	double hypot(x,y)
	//double __ieee754_hypot(x,y)
	double x, y;
#endif
{
	double a=x,b=y,t1,t2,y1,y2,w;
	__int32_t j,k,ha,hb;

	GET_HIGH_WORD(ha,x);
	ha &= 0x7fffffff;
	GET_HIGH_WORD(hb,y);
	hb &= 0x7fffffff;
	if(hb > ha) {a=y;b=x;j=ha; ha=hb;hb=j;} else {a=x;b=y;}
	SET_HIGH_WORD(a,ha);	/* a <- |a| */
	SET_HIGH_WORD(b,hb);	/* b <- |b| */
	if((ha-hb)>0x3c00000) {return a+b;} /* x/y > 2**60 */
	k=0;
#ifdef __MATH_NONFINITE__	
	if(ha >= 0x7ff00000) {	/* Inf or NaN */
	       __uint32_t low;
	       w = a+b;			/* for sNaN */
	       GET_LOW_WORD(low,a);
	       if(((ha&0xfffff)|low)==0) w = a;
	       GET_LOW_WORD(low,b);
	       if(((hb^0x7ff00000)|low)==0) w = b;
	       return w;
	}
#endif /* __MATH_NONFINITE__ */
	
	if(ha > 0x5f300000) {	/* a>2**500 */
	   /* scale a and b by 2**-600 */
	   ha -= 0x25800000; hb -= 0x25800000;	k += 600;
	   SET_HIGH_WORD(a,ha);
	   SET_HIGH_WORD(b,hb);
	}
	if(hb < 0x20b00000) {	/* b < 2**-500 */
	    __uint32_t low;
            GET_LOW_WORD(low,b);
	    if((hb|low)==0) return a;
#ifdef __MATH_SUBNORMAL__
	    if(hb <= 0x000fffff) {	/* subnormal b or 0 */	
	        t1=0;
		SET_HIGH_WORD(t1,0x7fd00000);	/* t1=2^1022 */
		b *= t1;
		a *= t1;
		k -= 1022;
	    } else {		/* scale a and b by 2^600 */
	        ha += 0x25800000; 	/* a *= 2^600 */
		hb += 0x25800000;	/* b *= 2^600 */
		k -= 600;
		SET_HIGH_WORD(a,ha);
		SET_HIGH_WORD(b,hb);
	    }
#else /* __MATH_SUBNORMAL__ */
	    ha += 0x25800000;       /* a *= 2^600 */
            hb += 0x25800000;       /* b *= 2^600 */
            k -= 600;
            SET_HIGH_WORD(a,ha);
            SET_HIGH_WORD(b,hb);

#endif /* __MATH_SUBNORMAL__ */
	}
    /* medium size a and b */
	w = a-b;
	if (w>b) {
	    t1 = 0;
	    SET_HIGH_WORD(t1,ha);
	    t2 = a-t1;
#ifdef __MATH_SOFT_FLOAT__
	    w  = __ieee754_sqrt(t1*t1-(b*(-b)-t2*(a+t1)));
#else /* __MATH_SOFT_FLOAT__ */
	    __inst_sqrt_D(w,(t1*t1-(b*(-b)-t2*(a+t1))));
#endif /* __MATH_SOFT_FLOAT__ */
	} else {
	    a  = a+a;
	    y1 = 0;
	    SET_HIGH_WORD(y1,hb);
	    y2 = b - y1;
	    t1 = 0;
	    SET_HIGH_WORD(t1,ha+0x00100000);
	    t2 = a - t1;
#ifdef __MATH_SOFT_FLOAT__
	    w  = __ieee754_sqrt(t1*y1-(w*(-w)-(t1*y2+t2*b)));
#else /* __MATH_SOFT_FLOAT__ */
	    __inst_sqrt_D(w,(t1*y1-(w*(-w)-(t1*y2+t2*b))));
#endif /* __MATH_SOFT_FLOAT__ */
	}
	if(k!=0) {
	    __uint32_t high = 0x3ff00000+(k<<20);
	    t1 = 0;
	    SET_HIGH_WORD(t1,high);    
	    w = t1*w;
	}
	__int32_t hw;
        __uint32_t iw;
        GET_HIGH_WORD(hw,w);
        iw=hw&0x7fffffff;
        if(!((iw-0x7ff00000)>>31))	/* overflow */
	    errno=ERANGE;
	return w; 
}

#endif /* defined(_DOUBLE_IS_32BITS) */
