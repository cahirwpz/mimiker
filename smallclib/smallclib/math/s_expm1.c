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
*              file : $RCSfile: s_expm1.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

/* @(#)s_expm1.c 5.1 93/09/24 */
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

/*
FUNCTION
	<<expm1>>, <<expm1f>>---exponential minus 1
INDEX
	expm1
INDEX
	expm1f

ANSI_SYNOPSIS
	#include <math.h>
	double expm1(double <[x]>);
	float expm1f(float <[x]>);

TRAD_SYNOPSIS
	#include <math.h>
	double expm1(<[x]>);
	double <[x]>;

	float expm1f(<[x]>);
	float <[x]>;

DESCRIPTION
	<<expm1>> and <<expm1f>> calculate the exponential of <[x]>
	and subtract 1, that is,
	@ifnottex
	e raised to the power <[x]> minus 1 (where e
	@end ifnottex
	@tex
	$e^x - 1$ (where $e$
	@end tex
	is the base of the natural system of logarithms, approximately
	2.71828).  The result is accurate even for small values of
	<[x]>, where using <<exp(<[x]>)-1>> would lose many
	significant digits.

RETURNS
	e raised to the power <[x]>, minus 1.

PORTABILITY
	Neither <<expm1>> nor <<expm1f>> is required by ANSI C or by
	the System V Interface Definition (Issue 2).
*/

/* expm1(x)
 * Returns exp(x)-1, the exponential of x minus 1.
 *
 * Method
 *   1. Argument reduction:
 *	Given x, find r and integer k such that
 *
 *               x = k*ln2 + r,  |r| <= 0.5*ln2 ~ 0.34658  
 *
 *      Here a correction term c will be computed to compensate 
 *	the error in r when rounded to a floating-point number.
 *
 *   2. Approximating expm1(r) by a special rational function on
 *	the interval [0,0.34658]:
 *	Since
 *	    r*(exp(r)+1)/(exp(r)-1) = 2+ r^2/6 - r^4/360 + ...
 *	we define R1(r*r) by
 *	    r*(exp(r)+1)/(exp(r)-1) = 2+ r^2/6 * R1(r*r)
 *	That is,
 *	    R1(r**2) = 6/r *((exp(r)+1)/(exp(r)-1) - 2/r)
 *		     = 6/r * ( 1 + 2.0*(1/(exp(r)-1) - 1/r))
 *		     = 1 - r^2/60 + r^4/2520 - r^6/100800 + ...
 *      We use a special Reme algorithm on [0,0.347] to generate 
 * 	a polynomial of degree 5 in r*r to approximate R1. The 
 *	maximum error of this polynomial approximation is bounded 
 *	by 2**-61. In other words,
 *	    R1(z) ~ 1.0 + Q1*z + Q2*z**2 + Q3*z**3 + Q4*z**4 + Q5*z**5
 *	where 	Q1  =  -1.6666666666666567384E-2,
 * 		Q2  =   3.9682539681370365873E-4,
 * 		Q3  =  -9.9206344733435987357E-6,
 * 		Q4  =   2.5051361420808517002E-7,
 * 		Q5  =  -6.2843505682382617102E-9;
 *  	(where z=r*r, and the values of Q1 to Q5 are listed below)
 *	with error bounded by
 *	    |                  5           |     -61
 *	    | 1.0+Q1*z+...+Q5*z   -  R1(z) | <= 2 
 *	    |                              |
 *	
 *	expm1(r) = exp(r)-1 is then computed by the following 
 * 	specific way which minimize the accumulation rounding error: 
 *			       2     3
 *			      r     r    [ 3 - (R1 + R1*r/2)  ]
 *	      expm1(r) = r + --- + --- * [--------------------]
 *		              2     2    [ 6 - r*(3 - R1*r/2) ]
 *	
 *	To compensate the error in the argument reduction, we use
 *		expm1(r+c) = expm1(r) + c + expm1(r)*c 
 *			   ~ expm1(r) + c + r*c 
 *	Thus c+r*c will be added in as the correction terms for
 *	expm1(r+c). Now rearrange the term to avoid optimization 
 * 	screw up:
 *		        (      2                                    2 )
 *		        ({  ( r    [ R1 -  (3 - R1*r/2) ]  )  }    r  )
 *	 expm1(r+c)~r - ({r*(--- * [--------------------]-c)-c} - --- )
 *	                ({  ( 2    [ 6 - r*(3 - R1*r/2) ]  )  }    2  )
 *                      (                                             )
 *    	
 *		   = r - E
 *   3. Scale back to obtain expm1(x):
 *	From step 1, we have
 *	   expm1(x) = either 2^k*[expm1(r)+1] - 1
 *		    = or     2^k*[expm1(r) + (1-2^-k)]
 *   4. Implementation notes:
 *	(A). To save one multiplication, we scale the coefficient Qi
 *	     to Qi*2^i, and replace z by (x^2)/2.
 *	(B). To achieve maximum accuracy, we compute expm1(x) by
 *	  (i)   if x < -56*ln2, return -1.0, (raise inexact if x!=inf)
 *	  (ii)  if k=0, return r-E
 *	  (iii) if k=-1, return 0.5*(r-E)-0.5
 *        (iv)	if k=1 if r < -0.25, return 2*((r+0.5)- E)
 *	       	       else	     return  1.0+2.0*(r-E);
 *	  (v)   if (k<-2||k>56) return 2^k(1-(E-r)) - 1 (or exp(x)-1)
 *	  (vi)  if k <= 20, return 2^k((1-2^-k)-(E-r)), else
 *	  (vii) return 2^k(1-((E+2^-k)-r)) 
 *
 * Special cases:
 *	expm1(INF) is INF, expm1(NaN) is NaN;
 *	expm1(-INF) is -1, and
 *	for finite argument, only expm1(0)=0 is exact.
 *
 * Accuracy:
 *	according to an error analysis, the error is always less than
 *	1 ulp (unit in the last place).
 *
 * Misc. info.
 *	For IEEE double 
 *	    if x >  7.09782712893383973096e+02 then expm1(x) overflow
 *
 * Constants:
 * The hexadecimal values are the intended ones for the following 
 * constants. The decimal values may be used, provided that the 
 * compiler will convert from decimal to binary accurately enough
 * to produce the hexadecimal values shown.
 */

#include<errno.h>
#include "low/_math.h"
#include "low/_fpuinst.h"

#ifndef _DOUBLE_IS_32BITS

#ifdef __STDC__
static const double
#else
static double
#endif
huge		= 1.0e+300,
tiny		= 1.0e-300,
o_threshold	= 7.09782712893383973096e+02,/* 0x40862E42, 0xFEFA39EF */
ln2_hi		= 6.93147180369123816490e-01,/* 0x3fe62e42, 0xfee00000 */
ln2_lo		= 1.90821492927058770002e-10,/* 0x3dea39ef, 0x35793c76 */
invln2		= 1.44269504088896338700e+00;/* 0x3ff71547, 0x652b82fe */

	/* scaled coefficients related to expm1 */
#ifdef __MATH_FORCE_EXPAND_POLY__
static const double
Q1  =  -3.33333333333331316428e-02, /* BFA11111 111110F4 */
Q2  =   1.58730158725481460165e-03, /* 3F5A01A0 19FE5585 */
Q3  =  -7.93650757867487942473e-05, /* BF14CE19 9EAADBB7 */
Q4  =   4.00821782732936239552e-06, /* 3ED0CFCA 86E65239 */
Q5  =  -2.01099218183624371326e-07; /* BE8AFDB7 6E09C32D */
#else /* __MATH_FORCE_EXPAND_POLY__ */
static const double Q[] = {
-3.33333333333331316428e-02, /* Q1  =  BFA11111 111110F4 */
 1.58730158725481460165e-03, /* Q2  =  3F5A01A0 19FE5585 */
-7.93650757867487942473e-05, /* Q3  =  BF14CE19 9EAADBB7 */
 4.00821782732936239552e-06, /* Q4  =  3ED0CFCA 86E65239 */
-2.01099218183624371326e-07  /* Q5  =  BE8AFDB7 6E09C32D */
};

static double poly(double w, const double cf[], double retval)
{
  int i=4;
  while (i >= 0)
    retval=w*retval+cf[i--];
  return w*retval;
}
#endif /* __MATH_FORCE_EXPAND_POLY__ */

#ifdef __STDC__
	double expm1(double x)
#else
	double expm1(x)
	double x;
#endif
{
  double y,hi,lo,c,t,e,hxs,hfx,r1,three,six,half,one;
  __int32_t k,xsb,high;
  __uint32_t hx;

  GET_HIGH_WORD(hx,x);
  xsb = hx&0x80000000;		/* sign bit of x */
  hx &= 0x7fffffff;		/* high word of |x| */

#ifdef __MATH_CONST_FROM_MEMORY__
  one=1.0;
#else /* __MATH_CONST_FROM_MEMORY__ */
  __inst_ldi_D_W(one,1);
#endif /* __MATH_CONST_FROM_MEMORY__ */

  /* filter out huge and non-finite argument */
#ifdef __MATH_NONFINITE__    
  if(hx>=0x7ff00000) {
    __uint32_t low;
    GET_LOW_WORD(low,x);
    if(((hx&0xfffff)|low)!=0) 
      return x+x; 	 /* NaN */
    else return (xsb==0)? x:-one;/* exp(+-inf)={inf,-1} */
  }
#endif /* __MATH_NONFINITE__ */
	        
  if(x > o_threshold) {
	  errno=ERANGE;
	  return huge*huge; /* overflow */
  }
  if(hx >= 0x4043687A) {			/* if |x|>=56*ln2 */
    if(xsb!=0) { /* x < -56*ln2, return -1.0 with inexact */
#ifdef __MATH_MATCH_NEWLIB_EXCEPTION__
      if(x+tiny<0.0)		/* raise inexact */
        return tiny-one;	/* return -1 */
#else /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
      return -one;
#endif /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
    }
  }
     
  /* argument reduction */
  if(hx > 0x3fd62e42) {		/* if  |x| > 0.5 ln2 */ 
    if(hx < 0x3FF0A2B2) {	/* and |x| < 1.5 ln2 */
      if(xsb==0)
        {k =  1;}
      else
        {k = -1;}
    } else {
#ifdef __MATH_CONST_FROM_MEMORY__
      half=(xsb==0)?0.5:-0.5;
#else /* __MATH_CONST_FROM_MEMORY__ */
      __inst_ldi_D_H (half,0x3fe00000|xsb);
#endif /* __MATH_CONST_FROM_MEMORY__ */
      k  = invln2*x+half;
    }
    t  = k;
    hi = x - t*ln2_hi;	/* t*ln2_hi is exact here */
    lo = t*ln2_lo;
    x  = hi - lo;
    c  = (hi-x)-lo;
  } 
  else if(hx < 0x3c900000) {  	/* when |x|<2**-54, return x */
#ifdef __MATH_MATCH_NEWLIB_EXCEPTION__
    t = huge+x;	/* return x with inexact flags when x!=0 */
    return x - (t-(huge+x));	
#else /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
	 return x;
#endif /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
  }
  else k = 0;

  /* x is now in primary range */
  hfx = 0.5*x;
  hxs = x*hfx;
#ifdef __MATH_FORCE_EXPAND_POLY__
  r1 = one+hxs*(Q1+hxs*(Q2+hxs*(Q3+hxs*(Q4+hxs*Q5))));
#else  /* __MATH_FORCE_EXPAND_POLY__ */
  r1 = one+poly(hxs,Q,0);
#endif /* __MATH_FORCE_EXPAND_POLY__ */
#ifdef __MATH_CONST_FROM_MEMORY__
  three=3.0;
#else /* __MATH_CONST_FROM_MEMORY__ */
  __inst_ldi_D_W(three,3);
#endif /* __MATH_CONST_FROM_MEMORY__ */
  t  = three-r1*hfx;
#ifdef __MATH_CONST_FROM_MEMORY__
  six=6.0;
#else /* __MATH_CONST_FROM_MEMORY__ */
  __inst_ldi_D_W(six,6);
#endif /* __MATH_CONST_FROM_MEMORY__ */
  e  = hxs*((r1-t)/(six - x*t));
  if(k==0) return x - (x*e-hxs);		/* c is 0 */
  e  = (x*(e-c)-c);
  e -= hxs;
  if(k== -1) {
    return 0.5*(x-e)-0.5;
  }
  if(k==1) {
    double two;
#ifdef __MATH_CONST_FROM_MEMORY__
  two=2.0;
#else /* __MATH_CONST_FROM_MEMORY__ */
  __inst_ldi_D_W(two,2);
#endif /* __MATH_CONST_FROM_MEMORY__ */
    if(x < -0.25) return -two*(e-(x+0.5));
    else return  one+two*(x-e);
  }
  if (k <= -2 || k>56) {   /* suffice to return exp(x)-1 */
    y = one-(e-x);
    GET_HIGH_WORD(high,y);
    SET_HIGH_WORD(y,high+(k<<20));	/* add k to y's exponent */
    return y-one;
  }
  t = 0.0;
  if(k<20) {
    SET_HIGH_WORD(t,0x3ff00000 - (0x200000>>k));  /* t=1-2^-k */
    y = t-(e-x);
  } else {
    SET_HIGH_WORD(t,((0x3ff-k)<<20));	/* 2^-k */
    y = x-(e+t);
    y += one;
  }
  GET_HIGH_WORD(high,y);
  SET_HIGH_WORD(y,high+(k<<20));	/* add k to y's exponent */
  return y;
}

#endif /* _DOUBLE_IS_32BITS */
