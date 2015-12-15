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
* 		  file : $RCSfile: kf_tan.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* From Newlib 2.0, skip generation of inexact. Replaced call to fabsf with
   instruction */

/* kf_tan.c -- float version of k_tan.c
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
#include "low/_gpinst.h"

#ifdef __STDC__
static const float 
#else
static float 
#endif
pio4  =  7.8539812565e-01, /* 0x3f490fda */
pio4lo=  3.7748947079e-08; /* 0x33222168 */

#ifdef __MATH_FORCE_EXPAND_POLY__ /* Original coefficient array, for reference */
static const float
T[] =  {
  3.3333334327e-01, /* 0x3eaaaaab */
  1.3333334029e-01, /* 0x3e088889 */
  5.3968254477e-02, /* 0x3d5d0dd1 */
  2.1869488060e-02, /* 0x3cb327a4 */
  8.8632395491e-03, /* 0x3c11371f */
  3.5920790397e-03, /* 0x3b6b6916 */
  1.4562094584e-03, /* 0x3abede48 */
  5.8804126456e-04, /* 0x3a1a26c8 */
  2.4646313977e-04, /* 0x398137b9 */
  7.8179444245e-05, /* 0x38a3f445 */
  7.1407252108e-05, /* 0x3895c07a */
 -1.8558637748e-05, /* 0xb79bae5f */
  2.5907305826e-05, /* 0x37d95384 */
};
#else /* __MATH_FORCE_EXPAND_POLY__ */
static const float
T0= 3.3333334327e-01, /* 0x3eaaaaab */
T11= -1.8558637748e-05, /* 0xb79bae5f */
T12= 2.5907305826e-05, /* 0x37d95384 */
T[] =  {
  1.3333334029e-01, /* 0x3e088889 */
  5.3968254477e-02, /* 0x3d5d0dd1 */
  2.1869488060e-02, /* 0x3cb327a4 */
  8.8632395491e-03, /* 0x3c11371f */
  3.5920790397e-03, /* 0x3b6b6916 */
  1.4562094584e-03, /* 0x3abede48 */
  5.8804126456e-04, /* 0x3a1a26c8 */
  2.4646313977e-04, /* 0x398137b9 */
  7.8179444245e-05, /* 0x38a3f445 */
  7.1407252108e-05, /* 0x3895c07a */
};

static float poly(float w, float *retptr, const float T[10])
{
	int i=9;
	float retval1=T11, retval2 = T12;
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
	float __kernel_tanf(float x, float y, int iy)
#else
	float __kernel_tanf(x, y, iy)
	float x,y; int iy;
#endif
{
	float z,r,v,w,s,one;
	__int32_t ix,hx;
	GET_FLOAT_WORD(hx,x);
	ix = hx&0x7fffffff;	/* high word of |x| */

#ifdef __MATH_CONST_FROM_MEMORY__
	one=1.0f;
#else /* __MATH_CONST_FROM_MEMORY__ */
	__inst_ldi_S_H(one,0x3f80);
#endif /* __MATH_CONST_FROM_MEMORY__ */

	if(ix<0x31800000)			/* x < 2**-28 */
	{
#ifdef __MATH_MATCH_NEWLIB_EXCEPTION__
	/* Inexact not needed for tanf as per ISO standard */
		if((int)x==0)			/* generate inexact */
#endif /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
		{
		    if((ix|(iy+1))==0) {
#ifdef __MATH_SOFT_FLOAT__
			return one/fabsf(x);
#else /* __MATH_SOFT_FLOAT__ */
			__inst_abs_S(x,x);
			return one/x;
#endif /* __MATH_SOFT_FLOAT__ */
		    }
		    else return (iy==1)? x: -one/x;
		}
	}
	if(ix>=0x3f2ca140) { 			/* |x|>=0.6744 */
	    if(hx<0) {x = -x; y = -y;}
	    z = pio4-x;
	    w = pio4lo-y;
	    x = z+w; y = x-x;
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
	if(ix>=0x3f2ca140) {
	    v = (float)iy;
	    return (float)(1-((hx>>30)&2))*(v-(float)2.0*(x-(w*w/(w+v)-r)));
	}
	if(iy==1) return w;
	else {		/* if allow error up to 2 ulp, 
			   simply return -1.0/(x+r) here */
     /*  compute -1.0/(x+r) accurately */
	    float a,t;
	    __int32_t i;
	    z  = w;
	    GET_FLOAT_WORD(i,z);
#ifdef __HW_SET_BITS__
	    __inst_clear_lo_bits(i,12);
	    SET_FLOAT_WORD(z,i);
#else /* __HW_SET_BITS__ */
	    SET_FLOAT_WORD(z,i&0xfffff000);
#endif /* __HW_SET_BITS__ */
	    v  = r-(z - x); 	/* z+v = r+x */
	    t = a  = -one/w;	/* a = -1.0/w */
	    GET_FLOAT_WORD(i,t);
#ifdef __HW_SET_BITS__
	    __inst_clear_lo_bits(i,12);
	    SET_FLOAT_WORD(t,i);
#else /* __HW_SET_BITS__ */
	    SET_FLOAT_WORD(t,i&0xfffff000);
#endif /* __HW_SET_BITS__ */
	    s  = one+t*z;
	    return t+a*(s+t*v);
	}
}
