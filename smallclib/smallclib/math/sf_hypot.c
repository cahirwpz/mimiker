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
*              file : $RCSfile: ef_hypot.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

/* ef_hypot.c -- float version of e_hypot.c.
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

#include "low/_math.h"
#include "low/_fpuinst.h"
#include <errno.h>

#ifdef __STDC__
	float hypotf(float x, float y)
//	float __ieee754_hypotf(float x, float y)
#else
	float hypotf(x,y)
//	float __ieee754_hypotf(x,y)
	float x, y;
#endif
{
	float a=x,b=y,t1,t2,y1,y2,w;
	__int32_t j,k,ha,hb;

	GET_FLOAT_WORD(ha,x);
	ha &= 0x7fffffffL;
	GET_FLOAT_WORD(hb,y);
	hb &= 0x7fffffffL;
	if(hb > ha) {a=y;b=x;j=ha; ha=hb;hb=j;} else {a=x;b=y;}
	SET_FLOAT_WORD(a,ha);	/* a <- |a| */
	SET_FLOAT_WORD(b,hb);	/* b <- |b| */
	if((ha-hb)>0xf000000L) {return a+b;} /* x/y > 2**30 */
	k=0;
#ifdef __MATH_NONFINITE__	
	if(!FLT_UWORD_IS_FINITE(ha)) {	/* Inf or NaN */
	       w = a+b;			/* for sNaN */
	       if(FLT_UWORD_IS_INFINITE(ha)) w = a;
	       if(FLT_UWORD_IS_INFINITE(hb)) w = b;
	       return w;
	}
#endif /* __MATH_NONFINITE__ */

	if(ha > 0x58800000L) {	/* a>2**50 */
	   /* scale a and b by 2**-68 */
	   ha -= 0x22000000L; hb -= 0x22000000L;	k += 68;
	   SET_FLOAT_WORD(a,ha);
	   SET_FLOAT_WORD(b,hb);
	}
	if(hb < 0x26800000L) {	/* b < 2**-50 */
	    if(FLT_UWORD_IS_ZERO(hb))
	        return a;
#ifdef __MATH_SUBNORMAL__
	    if(FLT_UWORD_IS_SUBNORMAL(hb)) {
		SET_FLOAT_WORD(t1,0x7e800000L);	/* t1=2^126 */
		b *= t1;
		a *= t1;
		k -= 126;
	    } else {		/* scale a and b by 2^68 */
	        ha += 0x22000000; 	/* a *= 2^68 */
		hb += 0x22000000;	/* b *= 2^68 */
		k -= 68;
		SET_FLOAT_WORD(a,ha);
		SET_FLOAT_WORD(b,hb);
	    }
#else /* __MATH_SUBNORMAL__ */
	    ha += 0x22000000;       /* a *= 2^68 */
            hb += 0x22000000;       /* b *= 2^68 */
            k -= 68;
            SET_FLOAT_WORD(a,ha);
            SET_FLOAT_WORD(b,hb);

#endif /* __MATH_SUBNORMAL__ */
	}
    /* medium size a and b */
	w = a-b;
	if (w>b) {
	    SET_FLOAT_WORD(t1,ha&0xfffff000L);
	    t2 = a-t1;
#ifdef __MATH_SOFT_FLOAT__
	    w  = __ieee754_sqrtf(t1*t1-(b*(-b)-t2*(a+t1)));
#else /* __MATH_SOFT_FLOAT__ */
	    __inst_sqrt_S(w,(t1*t1-(b*(-b)-t2*(a+t1))));
#endif /* __MATH_SOFT_FLOAT__ */
	} else {
	    a  = a+a;
	    SET_FLOAT_WORD(y1,hb&0xfffff000L);
	    y2 = b - y1;
	    SET_FLOAT_WORD(t1,ha+0x00800000L);
	    t2 = a - t1;
#ifdef __MATH_SOFT_FLOAT__
	    w  = __ieee754_sqrtf(t1*y1-(w*(-w)-(t1*y2+t2*b)));
#else /* __MATH_SOFT_FLOAT__ */
	    __inst_sqrt_S(w,(t1*y1-(w*(-w)-(t1*y2+t2*b))));
#endif /* __MATH_SOFT_FLOAT__ */
	}
	if(k!=0) {
	    SET_FLOAT_WORD(t1,0x3f800000L+(k<<23));
	    w = t1*w;
	}
	__int32_t iw;
        GET_FLOAT_WORD(iw,w);
        iw &= 0x7fffffff;
        if(!FLT_UWORD_IS_FINITE(iw))
	    errno=ERANGE;
	return w;
}
