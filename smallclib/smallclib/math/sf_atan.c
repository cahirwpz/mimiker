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
* 		  file : $RCSfile: sf_atan.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* From Newlib 2.0 */

/* sf_atan.c -- float version of s_atan.c.
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
 *
 */

#include "low/_math.h"
#include "low/_fpuinst.h"

#ifdef __STDC__
static const float atanhi[] = {
#else
static float atanhi[] = {
#endif
  4.6364760399e-01, /* atan(0.5)hi 0x3eed6338 */
  7.8539812565e-01, /* atan(1.0)hi 0x3f490fda */
  9.8279368877e-01, /* atan(1.5)hi 0x3f7b985e */
  1.5707962513e+00, /* atan(inf)hi 0x3fc90fda */
};

#ifdef __STDC__
static const float atanlo[] = {
#else
static float atanlo[] = {
#endif
  5.0121582440e-09, /* atan(0.5)lo 0x31ac3769 */
  3.7748947079e-08, /* atan(1.0)lo 0x33222168 */
  3.4473217170e-08, /* atan(1.5)lo 0x33140fb4 */
  7.5497894159e-08, /* atan(inf)lo 0x33a22168 */
};


#ifdef __MATH_FORCE_EXPAND_POLY__ 
/* Original coefficients array for reference */
#ifdef __STDC__
static const float aT[] = {
#else
static float aT[] = {
#endif
  3.3333334327e-01, /* 0x3eaaaaaa */
 -2.0000000298e-01, /* 0xbe4ccccd */
  1.4285714924e-01, /* 0x3e124925 */
 -1.1111110449e-01, /* 0xbde38e38 */
  9.0908870101e-02, /* 0x3dba2e6e */
 -7.6918758452e-02, /* 0xbd9d8795 */
  6.6610731184e-02, /* 0x3d886b35 */
 -5.8335702866e-02, /* 0xbd6ef16b */
  4.9768779427e-02, /* 0x3d4bda59 */
 -3.6531571299e-02, /* 0xbd15a221 */
  1.6285819933e-02, /* 0x3c8569d7 */
};
#else /* Split array for polynomial step */
static const float aT[] = {
  3.3333334327e-01, /* 0x3eaaaaaa */
 -2.0000000298e-01, /* 0xbe4ccccd */
  1.4285714924e-01, /* 0x3e124925 */
 -1.1111110449e-01, /* 0xbde38e38 */
  9.0908870101e-02, /* 0x3dba2e6e */
 -7.6918758452e-02, /* 0xbd9d8795 */
  6.6610731184e-02, /* 0x3d886b35 */
 -5.8335702866e-02, /* 0xbd6ef16b */
  4.9768779427e-02, /* 0x3d4bda59 */
 -3.6531571299e-02  /* 0xbd15a221 */
},
aT10 = 1.6285819933e-02; /* 0x3c8569d7 */

static float poly(float w, float *retptr, const float aT[10])
{
	int i=9;
	float retval1=aT10, retval2 = 0;
	while (i > 0) {
	  retval1=w*retval1+aT[i-1];
	  retval2=w*retval2+aT[i];
	  i-=2;
	}
	*retptr=retval2;
	return retval1;
}
#endif /* __MATH_FORCE_EXPAND_POLY__ */

#ifdef __MATH_MATCH_NEWLIB_EXCEPTION__
#ifdef __STDC__
	static const float 
#else
	static float 
#endif
one   = 1.0,
huge   = 1.0e30;
#endif /* __MATH_MATCH_NEWLIB_EXCEPTION__ */

#ifdef __STDC__
	float atanf(float x)
#else
	float atanf(x)
	float x;
#endif
{
	float w,s1,s2,z;
	__int32_t ix,hx,id;

	GET_FLOAT_WORD(hx,x);
	ix = hx&0x7fffffff;
	if(ix>=0x50800000) {	/* if |x| >= 2^34 */
#ifdef __MATH_NONFINITE__
	  if(FLT_UWORD_IS_NAN(ix))
#ifdef __MATH_MATCH_NEWLIB_EXCEPTION__
	    return x+x;		/* NaN */
#else /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
	  return x;		/* NaN */
#endif /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
#endif /* __MATH_NONFINITE__ */
	  x=0; id=3; /* fall-through to generate +-(hi[3]+lo[3]) */
	} 
	else if (ix < 0x3ee00000) {	/* |x| < 0.4375 */
	  if (ix < 0x31000000) {	/* |x| < 2^-29 */
#ifdef __MATH_MATCH_NEWLIB_EXCEPTION__
	    if(huge+x>one)
#endif /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
	      return x;	/* raise inexact */
	  }
	  id = -1;
	} else {
	  float one;
#ifdef __MATH_CONST_FROM_MEMORY__
	  one=1.0;
#else /* __MATH_CONST_FROM_MEMORY__ */
	  __inst_ldi_S_H(one,0x3f80);
#endif /* __MATH_CONST_FROM_MEMORY__ */

#ifdef __MATH_SOFT_FLOAT__
	  x = fabsf(x);
#else /* __MATH_SOFT_FLOAT__ */
	  __inst_abs_S(x,x);
#endif /* __MATH_SOFT_FLOAT__ */
	  
	  if (ix < 0x3f980000) {		/* |x| < 1.1875 */
	    if (ix < 0x3f300000) {	/* 7/16 <=|x|<11/16 */
	      float two;
#ifdef __MATH_CONST_FROM_MEMORY__
	      two=2.0;
#else /* __MATH_CONST_FROM_MEMORY__ */
	      __inst_ldi_S_H(two,0x4000);
#endif /* __MATH_CONST_FROM_MEMORY__ */
	      id = 0; x = (two*x-one)/(two+x); 
	    } else {			/* 11/16<=|x|< 19/16 */
	      id = 1; x  = (x-one)/(x+one); 
	    }
	  } else {
	    if (ix < 0x401c0000) {	/* |x| < 2.4375 */
	      float onep5;	      
#ifdef __MATH_CONST_FROM_MEMORY__
	      onep5=1.5;
#else /* __MATH_CONST_FROM_MEMORY__ */
	      __inst_ldi_S_H(onep5, 0x3fc0);
#endif /* __MATH_CONST_FROM_MEMORY__ */
	      id = 2; x  = (x-onep5)/(one+onep5*x);
	    } else {			/* 2.4375 <= |x| < 2^66 */
	      id = 3; x  = -(one/x);
	    }
	  }
	}
    /* end of argument reduction */
	z = x*x;
	w = z*z;

    /* break sum from i=0 to 10 aT[i]z**(i+1) into odd and even poly */
#ifdef __MATH_FORCE_EXPAND_POLY__  /* Original polynomial expression for reference */
	s1 = z*(aT[0]+w*(aT[2]+w*(aT[4]+w*(aT[6]+w*(aT[8]+w*aT[10])))));
	s2 = w*(aT[1]+w*(aT[3]+w*(aT[5]+w*(aT[7]+w*aT[9]))));
#else /* __MATH_FORCE_EXPAND_POLY__ */
	s1 = z*poly(w,&s2,aT);
	s2 = w*s2;
#endif /* __MATH_FORCE_EXPAND_POLY__ */

	if (id<0) return x - x*(s1+s2);
	else{
	    z = atanhi[id] - ((x*(s1+s2) - atanlo[id]) - x);
	    return (hx<0)? -z:z;
	}
}

#ifdef _DOUBLE_IS_32BITS

#ifdef __STDC__
	double atan(double x)
#else
	double atan(x)
	double x;
#endif
{
	return (double) atanf((float) x);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
