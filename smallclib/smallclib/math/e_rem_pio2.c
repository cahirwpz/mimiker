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
* 		  file : $RCSfile: e_rem_pio2.c,v $ 
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
Fused multiply-add is implemented using sequences of partial products(___fma_hp.c)  */ 

/* @(#)e_rem_pio2.c 5.1 93/09/24 */
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

/* __ieee754_rem_pio2(x,y)
 * 
 * return the remainder of x rem pi/2 in y[0]+y[1] 
 * use __kernel_rem_pio2()
 */

#include "low/_math.h"
#include "low/_fpuinst.h"

#ifndef _DOUBLE_IS_32BITS

#ifdef __MATH_FULLRANGE_REDUCTION__

#ifdef __STDC__
static const double
#else
static double
#endif
two24 =  1.67772160000000000000e+07; /* 0x41700000, 0x00000000 */

#else /* __MATH_FULLRANGE_REDUCTION__ */
/*
 * Table of constants for 2/pi & pi/2
 */
static const double two_over_pi = 6.36619772367581382e-01; /* 0x3fe45f306dc9c883 */

static  const double top[] = {
 0.63661977236915845, /*  0x3fe45f30:6dca0000 */
 -1.5771111638333853e-12, /*  0xbd7bbead:603d8a83 */
};


static const double pi_over_two[] = {
0.57079632679233328, /*  0x3fe243f6:a8880000 */
2.5633441515839558e-12, /*  0x3d868c23:4c4c0000 */
1.0562999066944068e-23, /*  0x3b298a2e:03700000 */
4.3359050650607218e-35, /*  0x38ccd129:024e0000 */
1.1687494729361857e-47, /*  0x363114cf:98e80000 */
};

static const double DoubleSplitter=((double)(1<<27)+1.0);

static void split (double value, double *upper, double *lower)
{
  const double scale = DoubleSplitter * value;
  if (isinf (scale))
  {
	  *upper = value;
	  *lower = 0;
  }
  else
  {
	  (*upper) = (scale - (scale - value)); 
	  (*lower) = value - (*upper);
  }
  return;
}



static void long_product (double a, double ub, double lb, double *const up, double *const lp)
{
  double ua, la;
  __uint32_t hi, lo;

  (*up) = a * (ub+lb);
  split (a, &ua, &la);

  EXTRACT_WORDS (hi, lo, *up);
  hi=hi<<1;
  if ((hi == 0xffe00000) && (lo == 0))  /* a*b == INF */
    *lp = 0;
  else
    (*lp) = (la * lb - ((( (*up) - ua * ub ) - la * ub) - ua * lb));

  return;
}


#endif /* __MATH_FULLRANGE_REDUCTION__ */

#ifdef __STDC__
	__int32_t __ieee754_rem_pio2(double x, double *y)
#else
	__int32_t __ieee754_rem_pio2(x,y)
	double x,y[];
#endif
{
#ifdef __MATH_FULLRANGE_REDUCTION__
	double z = 0.0;
	double tx[3];
	__int32_t i,n,ix,hx;
	int e0,nx;
	__uint32_t low;

	GET_HIGH_WORD(hx,x);		/* high word of x */
	ix = hx&0x7fffffff;
    /* set z = scalbn(|x|,ilogb(x)-23) */
	GET_LOW_WORD(low,x);
	SET_LOW_WORD(z,low);
	e0 	= (int)((ix>>20)-1046);	/* e0 = ilogb(z)-23; */
	SET_HIGH_WORD(z, ix - ((__int32_t)e0<<20));

	for(i=0;i<2;i++) {
		tx[i] = (double)((__int32_t)(z));
		z     = (z-tx[i])*two24;
	}
	tx[2] = z;
	nx = 3;
#ifdef __MATH_SPEED_OVER_SIZE__
	while(tx[nx-1]==0.0) nx--;	/* skip zero term */
#endif /* __MATH_SPEED_OVER_SIZE__ */

	n  =  __kernel_rem_pio2(tx,y,e0,nx,hx);
	return n&3;
#else /* __MATH_FULLRANGE_REDUCTION__ */

  double N0;
  __int64_t N_sum;
  double uN, lN;
  
  long_product(x,top[0], top[1], &uN, &lN);
  N0 = uN + lN;

#ifdef __MATH_SOFT_FLOAT__
  N_sum = (long long int)round(N0);
  N0 = (double) N_sum;
#else /* __MATH_SOFT_FLOAT__ */
  __inst_round_D_prim(N_sum,N0,N0);
#endif /* __MATH_SOFT_FLOAT__ */  

  y[0] = x - N0;
  y[0] = fma (-N0, pi_over_two[0], y[0]); /* x - N - n1*C1 */
  y[0] = fma (-N0, pi_over_two[1], y[0]); /* x - N - n1*C1 - n1*C2 */
  y[0] = fma (-N0, pi_over_two[2], y[0]); /* x - N - n1*C1 - n1*C2 - n1*C3 */

  y[1] = - fma (-N0, pi_over_two[2], N0*pi_over_two[2]); /* y[1] = - low(n1*C3) */
  y[1] = fma (-N0, pi_over_two[3], y[1]); /* y[1] = y[1] - n1*C4 */
  y[1] = fma (-N0, pi_over_two[4], y[1]); /* y[1] = y[1] - n1*C5 */

  return (int)(N_sum & 0x3);
#endif /* __MATH_FULLRANGE_REDUCTION__ */
}

#endif /* defined(_DOUBLE_IS_32BITS) */
