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
* 		  file : $RCSfile: ef_rem_pio2.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/*  Full range reduction borrowed from Newlib 2.8 - uses Payne-Hanek reduction for
good accuracy over a large range of inputs.

Limited reduction strategy is an extension of Cody-Waite reduction using fused 
multipy-add for improved accuracy.. Refer: Extending the Range Reduction of the 
Cody and Waite Range Reduction method - Kornerup & Muller
URL: http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.123.9012&rep=rep1&type=pdf
Single-precision fused multiply-add is approximated by dual precision multiply followed
by dual-precision add - gives reasonably accurate results for this application */ 


/* ef_rem_pio2.c -- float version of e_rem_pio2.c
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

/* __ieee754_rem_pio2f(x,y)
 * 
 * return the remainder of x rem pi/2 in y[0]+y[1] 
 * use __kernel_rem_pio2f()
 */

#include "low/_math.h"
#include "low/_fpuinst.h"
#include "low/_gpinst.h"

#undef __MATH_FULLRANGE_REDUCTION__

#ifdef __MATH_FULLRANGE_REDUCTION__
#ifdef __STDC__
static const float 
#else
static float 
#endif
zero =  0.0000000000e+00, /* 0x00000000 */
two8 =  2.5600000000e+02; /* 0x43800000 */

#else /* !__MATH_FULLRANGE_REDUCTION__ */
static const float two_over_pi = 6.3661975e-01; /* 0x3F22F983 */

static const float pi_over_two[] = {5.7079631e-01,   /* 0x3F121FB5 */
				    1.5893255e-08	 /* 0x328885A3 */
};

static float fmaf_hp (float x, float y, float z)
{
	return ((double)x * (double)y + (double)z);
}
#endif /* __MATH_FULLRANGE_REDUCTION__ */


#ifdef __STDC__
	__int32_t __ieee754_rem_pio2f(float x, float *y)
#else
	__int32_t __ieee754_rem_pio2f(x,y)
	float x,y[];
#endif
{
#ifdef __MATH_FULLRANGE_REDUCTION__ 
/* Use Payne-Hanek range reduction - good precision over full range of
   single precision inputs */
  float z;
  float tx[3];
  __int32_t i,n,ix,hx;
  int e0,nx;

  GET_FLOAT_WORD(hx,x);
  ix = hx&0x7fffffff;

  /* set z = scalbn(|x|,ilogb(x)-7) */
  e0 	= (int)((ix>>23)-134);	/* e0 = ilogb(z)-7; */
  SET_FLOAT_WORD(z, ix - ((__int32_t)e0<<23));

  for(i=0;i<2;i++) {
    tx[i] = (float)((__int32_t)(z));
    z     = (z-tx[i])*two8;
  }

  tx[2] = z;
  nx = 3;
#ifdef __MATH_SPEED_OVER_SIZE__
  while(tx[nx-1]==zero) nx--;	/* skip zero term */
#endif /* __MATH_SPEED_OVER_SIZE__ */

  n = __kernel_rem_pio2f(tx,y,e0,nx);

#ifdef __HW_CLEAR_BITS__
  __inst_clear_lo_bits(hx,31);
#else /* __HW_CLEAR_BITS__ */
  hx=hx&0x80000000;
#endif /* __HW_CLEAR_BITS__ */

  __inst_togglesign_S(y[0],hx);
  __inst_togglesign_S(y[1],hx);

  if ((int)hx<0) n=-n;

  /* Only last 2 bits are ever relevant for the caller */
  return (n&3);
#else /* __MATH_FULLRANGE_REDUCTION__ */ 
/* Improvisation over Cody-Waite Range reduction. Accurate in a limited
range of 2^23, accuracy goes on decreasing till 2^32 and then goes 
completely hay-wire */
  float N0, N0_neg;
  __int32_t N_sum;
  N0=two_over_pi*x;
#ifdef __MATH_SOFT_FLOAT__
  N_sum = (int)roundf(N0);
  N0 = (float) N_sum;
#else
  __inst_round_S_prim(N_sum,N0,N0);
#endif  

  N0_neg = -N0;
  y[0] = x - N0;
  y[0] = fmaf_hp (N0_neg, pi_over_two[0], y[0]);
  y[1] = y[0];
  y[0] = fmaf_hp (N0_neg, pi_over_two[1], y[0]);

  y[1] = y[1] - y[0];
  y[1] = fmaf_hp (N0_neg, pi_over_two[1], y[1]);

  /* Only last 2 bits are ever relevant for the caller */
  return (N_sum&0x3);
#endif /* __MATH_FULLRANGE_REDUCTION__ */
}
