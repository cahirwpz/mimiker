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
* 		  file : $RCSfile: sf_atan2.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* From Newlib 2.0, disolved switch cases to simple conditions */

/* ef_atan2.c -- float version of e_atan2.c.
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
static const float 
#else
static float 
#endif
tiny  = 1.0e-30,
pi_o_4  = 7.8539818525e-01, /* 0x3f490fdb */
pi_o_2  = 1.5707963705e+00, /* 0x3fc90fdb */
pi      = 3.1415927410e+00,  /* 0x40490fdb */
pi_lo   = -8.7422776573e-08; /* 0xb3bbbd2e */

#ifdef __STDC__
	float atan2f(float y, float x)
#else
	float atan2f(y,x)
	float  y,x;
#endif
{  
	float z, retval;
	__int32_t hx,hy,ix,iy;

#ifdef __MATH_SPEED_OVER_SIZE__
	__int32_t k;
#endif /* __MATH_SPEED_OVER_SIZE__ */

	GET_FLOAT_WORD(hx,x);
	ix = hx&0x7fffffff;
	GET_FLOAT_WORD(hy,y);
	iy = hy&0x7fffffff;

#ifdef __MATH_NONFINITE__
	if(FLT_UWORD_IS_NAN(ix)||
	   FLT_UWORD_IS_NAN(iy))	/* x or y is NaN */
	   return x+y;
#endif /* __MATH_NONFINITE__ */

#ifdef __MATH_NONFINITE__
    /* when x is INF */
	if(FLT_UWORD_IS_INFINITE(ix) && (FLT_UWORD_IS_INFINITE(iy))) {
	  retval=pi_o_4;		/* atan(+INF,+INF) */
	  if (hx<0) {
	    retval*=3.0f;	/*atan(+INF,-INF)*/
	  }

	  goto common_exit;
	}
#endif /* __MATH_NONFINITE__ */

    /* when y = 0 */
#ifdef __MATH_NONFINITE__
	if(FLT_UWORD_IS_ZERO(iy) || FLT_UWORD_IS_INFINITE(ix)) {
#else /* __MATH_NONFINITE__ */
        if(FLT_UWORD_IS_ZERO(iy)) {
#endif /* __MATH_NONFINITE__ */
	  retval = 0.0;   	/* atan(+-0,+anything)=+-0 */
	    if (hx<0)
#ifdef __MATH_MATCH_NEWLIB_EXCEPTION__	      
	      retval=pi+tiny;		/* atan(+0,+-anything) = +-pi */
#else	/* __MATH_MATCH_NEWLIB_EXCEPTION__ */
	      retval=pi;
#endif	/* __MATH_MATCH_NEWLIB_EXCEPTION__ */
	  goto common_exit;
	}

    /* when x = 0 or when y is INF */
#ifdef __MATH_NONFINITE__
	  if((FLT_UWORD_IS_ZERO(ix)) || (FLT_UWORD_IS_INFINITE(iy))) {
#else /* __MATH_NONFINITE__ */
        if(FLT_UWORD_IS_ZERO(ix)) {
#endif /* __MATH_NONFINITE__ */
#ifdef __MATH_MATCH_NEWLIB_EXCEPTION__	      
	    retval=pi_o_2+tiny;
#else	/* __MATH_MATCH_NEWLIB_EXCEPTION__ */
	      retval=pi_o_2;
#endif	/* __MATH_MATCH_NEWLIB_EXCEPTION__ */
	    goto common_exit;
	}

    /* compute y/x */
#ifdef __MATH_SPEED_OVER_SIZE__
	k = (iy-ix)>>23;
	if(k > 60) {
	  z=pi_o_2+0.5*pi_lo; 	/* |y/x| >  2**60 */
	}
	else if(hx<0&&k<-60)
	  z=0.0; 	/* |y|/x < -2**60 */
	else 
#endif /* __MATH_SPEED_OVER_SIZE__ */
	{
	  z=y/x;
#ifdef __MATH_SOFT_FLOAT__
	  z=fabsf(z);
#else /* __MATH_SOFT_FLOAT__ */
	  __inst_abs_S(z,z);
#endif /* __MATH_SOFT_FLOAT__ */
	  z=atanf(z);
	}

	if (hx >=0) {
	    retval=z; 	/* atan(+,+) */
	}
	else {
	    retval=pi-(z-pi_lo); /* atan(+,-) */
	}

 common_exit:
	if (hy < 0) retval=-retval;
	return retval;
}
