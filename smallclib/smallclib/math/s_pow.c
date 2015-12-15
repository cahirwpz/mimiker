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
*              file : $RCSfile: e_pow.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

/* @(#)e_pow.c 5.1 93/09/24 */
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

/* __ieee754_pow(x,y) return x**y
 *
 *		      n
 * Method:  Let x =  2   * (1+f)
 *	1. Compute and return log2(x) in two pieces:
 *		log2(x) = w1 + w2,
 *	   where w1 has 53-24 = 29 bit trailing zeros.
 *	2. Perform y*log2(x) = n+y' by simulating multi-precision 
 *	   arithmetic, where |y'|<=0.5.
 *	3. Return x**y = 2**n*exp(y'*log2)
 *
 * Special cases:
 *	1.  (anything) ** 0  is 1
 *	2.  (anything) ** 1  is itself
 *	3a. (anything) ** NAN is NAN except
 *	3b. +1         ** NAN is 1
 *	4.  NAN ** (anything except 0) is NAN
 *	5.  +-(|x| > 1) **  +INF is +INF
 *	6.  +-(|x| > 1) **  -INF is +0
 *	7.  +-(|x| < 1) **  +INF is +0
 *	8.  +-(|x| < 1) **  -INF is +INF
 *	9.  +-1         ** +-INF is 1
 *	10. +0 ** (+anything except 0, NAN)               is +0
 *	11. -0 ** (+anything except 0, NAN, odd integer)  is +0
 *	12. +0 ** (-anything except 0, NAN)               is +INF
 *	13. -0 ** (-anything except 0, NAN, odd integer)  is +INF
 *	14. -0 ** (odd integer) = -( +0 ** (odd integer) )
 *	15. +INF ** (+anything except 0,NAN) is +INF
 *	16. +INF ** (-anything except 0,NAN) is +0
 *	17. -INF ** (anything)  = -0 ** (-anything)
 *	18. (-anything) ** (integer) is (-1)**(integer)*(+anything**integer)
 *	19. (-anything except 0 and inf) ** (non-integer) is NAN
 *
 * Accuracy:
 *	pow(x,y) returns x**y nearly rounded. In particular
 *			pow(integer,integer)
 *	always returns the correct integer provided it is 
 *	representable.
 *
 * Constants :
 * The hexadecimal values are the intended ones for the following 
 * constants. The decimal values may be used, provided that the 
 * compiler will convert from decimal to binary accurately enough 
 * to produce the hexadecimal values shown.
 */

#include "low/_math.h"
#include "low/_fpuinst.h"
#include <errno.h>

#define isnan_bits(hx,lx) (((int)(hx-0x7ff00000)>=0) && (((hx-0x7ff00000)|lx)!=0))
#define isinf_bits(hx,lx) (((hx-0x7ff00000)|lx)==0)

#ifndef _DOUBLE_IS_32BITS

#ifdef __STDC__
static const double 
#else
static double 
#endif
bp[] = {1.0, 1.5,},
dp_h[] = { 0.0, 5.84962487220764160156e-01,}, /* 0x3FE2B803, 0x40000000 */
dp_l[] = { 0.0, 1.35003920212974897128e-08,}, /* 0x3E4CFDEB, 0x43CFD006 */
zero    =  0.0,
huge	=  1.0e300,
tiny    =  1.0e-300,
	/* poly coefs for (3/2)*(log(x)-2s-2/3*s**3 */
lg2  =  6.93147180559945286227e-01, /* 0x3FE62E42, 0xFEFA39EF */
lg2_h  =  6.93147182464599609375e-01, /* 0x3FE62E43, 0x00000000 */
lg2_l  = -1.90465429995776804525e-09, /* 0xBE205C61, 0x0CA86C39 */
ovt =  8.0085662595372944372e-0017, /* -(1024-log2(ovfl+.5ulp)) */
cp    =  9.61796693925975554329e-01, /* 0x3FEEC709, 0xDC3A03FD =2/(3ln2) */
cp_h  =  9.61796700954437255859e-01, /* 0x3FEEC709, 0xE0000000 =(float)cp */
cp_l  = -7.02846165095275826516e-09, /* 0xBE3E2FE0, 0x145B01F5 =tail of cp_h*/
ivln2    =  1.44269504088896338700e+00, /* 0x3FF71547, 0x652B82FE =1/ln2 */
ivln2_h  =  1.44269502162933349609e+00, /* 0x3FF71547, 0x60000000 =24b 1/ln2*/
ivln2_l  =  1.92596299112661746887e-08; /* 0x3E54AE0B, 0xF85DDF44 =1/ln2 tail*/

#ifdef __MATH_FORCE_EXPAND_POLY__
static const double
L1  =  5.99999999999994648725e-01, /* 0x3FE33333, 0x33333303 */
L2  =  4.28571428578550184252e-01, /* 0x3FDB6DB6, 0xDB6FABFF */
L3  =  3.33333329818377432918e-01, /* 0x3FD55555, 0x518F264D */
L4  =  2.72728123808534006489e-01, /* 0x3FD17460, 0xA91D4101 */
L5  =  2.30660745775561754067e-01, /* 0x3FCD864A, 0x93C9DB65 */
L6  =  2.06975017800338417784e-01, /* 0x3FCA7E28, 0x4A454EEF */
P1   =  1.66666666666666019037e-01, /* 0x3FC55555, 0x5555553E */
P2   = -2.77777777770155933842e-03, /* 0xBF66C16C, 0x16BEBD93 */
P3   =  6.61375632143793436117e-05, /* 0x3F11566A, 0xAF25DE2C */
P4   = -1.65339022054652515390e-06, /* 0xBEBBBD41, 0xC5D26BF1 */
P5   =  4.13813679705723846039e-08; /* 0x3E663769, 0x72BEA4D0 */
#else /* __MATH_FORCE_EXPAND_POLY__ */
static const double 
LC[] = {
    5.99999999999994648725e-01, /* 0x3FE33333, 0x33333303 */
    4.28571428578550184252e-01, /* 0x3FDB6DB6, 0xDB6FABFF */
    3.33333329818377432918e-01, /* 0x3FD55555, 0x518F264D */
    2.72728123808534006489e-01, /* 0x3FD17460, 0xA91D4101 */
    2.30660745775561754067e-01 /* 0x3FCD864A, 0x93C9DB65 */
},
L6  =  2.06975017800338417784e-01, /* 0x3FCA7E28, 0x4A454EEF */
PC[] = {
    1.66666666666666019037e-01, /* 0x3FC55555, 0x5555553E */
    -2.77777777770155933842e-03, /* 0xBF66C16C, 0x16BEBD93 */
    6.61375632143793436117e-05, /* 0x3F11566A, 0xAF25DE2C */
    -1.65339022054652515390e-06, /* 0xBEBBBD41, 0xC5D26BF1 */
    4.13813679705723846039e-08 /* 0x3E663769, 0x72BEA4D0 */
};

static double poly(double w, const double cf[], double retval)
{
	int i=4;
	while (i >= 0)
	  retval=w*retval+cf[i--];
	return retval;
}
#endif /* __MATH_FORCE_EXPAND_POLY__ */

#ifdef __STDC__
	double pow(double x, double y)
#else
	double pow(x,y)
	double x, y;
#endif
{
	double z,ax,z_h,z_l,p_h,p_l;
	double y1,t1,t2,r,s,t,u,v,w;
	__int32_t i,j,k,yisint,n;
	__int32_t hx,hy,ix,iy;
	__uint32_t lx,ly;
	double one,two;

	EXTRACT_WORDS(hx,lx,x);
	EXTRACT_WORDS(hy,ly,y);
	ix = hx&0x7fffffff;  iy = hy&0x7fffffff;

#ifdef __MATH_CONST_FROM_MEMORY__
	one=1.0;
#else /* __MATH_CONST_FROM_MEMORY__ */
	__inst_ldi_D_W(one,1);
#endif /* __MATH_CONST_FROM_MEMORY__ */

    	/* y==zero: x**0 = 1 */
	if((iy|ly)==0) return one; 	
	
	/* x==+one: 1**y = 1 even if y is NaN */
	if(x == one) return x;
	
#ifdef __MATH_NONFINITE__
    	/* x|y==NaN return NaN */
	if (isnan_bits(ix,lx) || isnan_bits(iy,ly))
	    return x+y;
#endif /* __MATH_NONFINITE__ */

    	/* special value of y */
	if(ly==0) {
#ifdef __MATH_NONFINITE__ 
	    if (iy==0x7ff00000) {	/* y is +-inf */
		if(((ix-0x3ff00000)|lx)==0) return one; /*  -1 **+=INF return one */
		else if(((ix >= 0x3ff00000) && (hy >= 0)) || (!(ix >= 0x3ff00000) && (hy < 0)))
		    {SET_HIGH_WORD(y,iy); return y;}
		else
		    return zero;     	
	    } 
#endif /* __MATH_NONFINITE__ */
	    if(iy==0x3ff00000) {	/* y is  +-1 */
		if(hy<0) {
		    if ((ix|lx)==0)
			errno=EDOM;
		    else if(ix<0x00100000)
			errno=ERANGE;
		    return one/x; 
		}
		else return x;
	    }
	}
	
	/* determine if y is an odd int when x < 0
     	* yisint = 0	... y is not an integer
     	* yisint = 1	... y is an odd int
     	* yisint = 2	... y is an even int
     	*/
	yisint  = 0;
	if(hx<0) {	
	    if(iy>=0x43400000) yisint = 2; /* even integer y */
	    else if(iy>=0x3ff00000) {
		int cnt=20, flag=0;
		__int32_t yy;
		k = (iy>>20)-0x3ff;	   /* exponent */
		if(k>20) {cnt = 52; yy = ly; flag=1;}
		else if(ly==0) {cnt = 20; yy = iy; flag=1;}
		if(flag==1) {
		    j = yy>>(cnt-k);
		    if((j<<(cnt-k))==yy) yisint = 2-(j&1);
		}
	    }
	}

#ifdef __MATH_SOFT_FLOAT__
	ax  = fabs(x);
#else /* __MATH_SOFT_FLOAT__ */
	__inst_abs_D(ax,x);
#endif /* __MATH_SOFT_FLOAT__ */

	/* special value of x */
	if((lx==0) && (ix==0x7ff00000||ix==0||ix==0x3ff00000)) {	/*x is +-0,+-inf,+-1*/
	    z = ax;	
	    if(hy<0) {
		if(ix==0) errno=EDOM; 
		z = one/z;	 /* z = (1/|x|) */
	    }
	    if(hx<0) {
		if(((ix-0x3ff00000)|yisint)==0) {
		    errno = EDOM;
		    return 0.0/0.0; /* (-1)**non-int is NaN */
		} else if(yisint==1) 
		    z = -z;		/* (x<0)**odd = -(|x|**odd) */
	    }
	    return z;
	}
    
	/* (x<0)**(non-int) is NaN */
    	/* REDHAT LOCAL: This used to be
	if((((hx>>31)+1)|yisint)==0) return (x-x)/(x-x);
       	but ANSI C says a right shift of a signed negative quantity is
       	implementation defined.  */
	if(((((__uint32_t)hx>>31)-1)|yisint)==0) {
	    errno = EDOM; 
	    return 0.0/0.0;
	}

    	/* |y| is huge */
	if(iy>0x41e00000) { /* if |y| > 2**31 */
	    double half, quart;
	    /* over/underflow if x is not close to one */
	    if(ix<0x3fefffff){ errno=ERANGE; return (hy<0)? huge*huge:tiny*tiny; }
	    if(ix>0x3ff00000){ errno=ERANGE; return (hy>0)? huge*huge:tiny*tiny; }

	    if(iy>0x43f00000){	/* if |y| > 2**64, must o/uflow */
		if(ix==0x3fefffff){ errno=ERANGE; return (hy<0)? huge*huge:tiny*tiny; }
		if(ix==0x3ff00000){ errno=ERANGE; return (hy>0)? huge*huge:tiny*tiny; }
	    }

#ifdef __MATH_CONST_FROM_MEMORY__
	    half=0.5;
	    quart=0.25;
#else /* __MATH_CONST_FROM_MEMORY__ */
	    __inst_ldi_D_H (half, 0x3fe00000);
	    __inst_ldi_D_H (quart, 0x3fd00000);
#endif /* __MATH_CONST_FROM_MEMORY__ */

	    /* now |1-x| is tiny <= 2**-20, suffice to compute
	       log(x) by x-x^2/2+x^3/3-x^4/4 */
	    t = ax-one;		/* t has 20 trailing zeros */
	    w = (t*t)*(half-t*(0.3333333333333333333333-t*quart));
	    u = ivln2_h*t;	/* ivln2_h has 21 sig. bits */
	    v = t*ivln2_l-w*ivln2;
	    t1 = u+v;
#ifdef __MATH_SOFT_FLOAT__
	    SET_LOW_WORD(t1,0);
#else  /* __MATH_SOFT_FLOAT__ */
	    __inst_reset_low_D(t1);
#endif /* __MATH_SOFT_FLOAT__ */
	    t2 = v-(t1-u);
	} else {
	    double s2,s_h,s_l,t_h,t_l,bpval,three;
	    n = 0;
#ifdef __MATH_SUBNORMAL__
	    /* take care subnormal number */
	    if(ix<0x00100000) {
		double two53;
#ifdef __MATH_CONST_FROM_MEMORY__
		two53 = 9007199254740992.0;
#else /* __MATH_CONST_FROM_MEMORY__ */
		__inst_ldi_D_H(two53, 0x43400000);
#endif /* __MATH_CONST_FROM_MEMORY__ */
		ax *= two53; n -= 53; GET_HIGH_WORD(ix,ax); 
	    }
#endif /* __MATH_SUBNORMAL__ */
	    n  += ((ix)>>20)-0x3ff;
	    j  = ix&0x000fffff;
	    /* determine interval */
	    ix = j|0x3ff00000;		/* normalize ix */
	    if(j<=0x3988E) k=0;		/* |x|<sqrt(3/2) */
	    else if(j<0xBB67A) k=1;	/* |x|<sqrt(3)   */
	    else {k=0;n+=1;ix -= 0x00100000;}
	    SET_HIGH_WORD(ax,ix);

	    /* compute s = s_h+s_l = (x-1)/(x+1) or (x-1.5)/(x+1.5) */
#ifdef __MATH_CONST_FROM_MEMORY__
	    bpval=bp[k];
#else /* __MATH_CONST_FROM_MEMORY__ */
	    __inst_ldi_D_H (bpval, 0x3ff00000 | (k << 19));
#endif /* __MATH_CONST_FROM_MEMORY__ */
	    u = ax-bpval;		/* bp[0]=1.0, bp[1]=1.5 */
	    v = one/(ax+bpval);
	    s = u*v;
	    s_h = s;
#ifdef __MATH_SOFT_FLOAT__
	    SET_LOW_WORD(s_h,0);
#else  /* __MATH_SOFT_FLOAT__ */
	    __inst_reset_low_D(s_h);
#endif /* __MATH_SOFT_FLOAT__ */

	    /* t_h=ax+bp[k] High */
#ifdef __MATH_SOFT_FLOAT__ 
	    t_h = zero;
	    SET_HIGH_WORD(t_h,((ix>>1)|0x20000000)+0x00080000+(k<<18));
#else /* __MATH_SOFT_FLOAT__ */
	    __inst_ldi_D_H (t_h, (((ix>>1)|0x20000000)+0x00080000+(k<<18)));
#endif /* __math_soft_float__ */

	    t_l = ax - (t_h-bpval);
	    s_l = v*((u-s_h*t_h)-s_h*t_l);
	    /* compute log(ax) */
	    s2 = s*s;
#ifdef __MATH_FORCE_EXPAND_POLY__
	    r = s2*s2*(L1+s2*(L2+s2*(L3+s2*(L4+s2*(L5+s2*L6)))));
#else /* __MATH_FORCE_EXPAND_POLY__ */
	    r = s2*s2*poly(s2,LC,L6);
#endif /* __MATH_FORCE_EXPAND_POLY__ */
	    r += s_l*(s_h+s);
	    s2  = s_h*s_h;

#ifdef __MATH_CONST_FROM_MEMORY__
	    three = 3.0;
#else  /* __MATH_CONST_FROM_MEMORY__ */
	    __inst_ldi_D_W (three, 3);
#endif /* __MATH_CONST_FROM_MEMORY__ */

	    t_h = three+s2+r;
#ifdef __MATH_SOFT_FLOAT__
	    SET_LOW_WORD(t_h,0);
#else  /* __MATH_SOFT_FLOAT__ */
	    __inst_reset_low_D(t_h);
#endif /* __MATH_SOFT_FLOAT__ */
	    t_l = r-((t_h-three)-s2);
	    /* u+v = s*(1+...) */
	    u = s_h*t_h;
	    v = s_l*t_h+t_l*s;
	    /* 2/(3log2)*(s+...) */
	    p_h = u+v;
#ifdef __MATH_SOFT_FLOAT__
	    SET_LOW_WORD(p_h,0);
#else  /* __MATH_SOFT_FLOAT__ */
	    __inst_reset_low_D(p_h);
#endif /* __MATH_SOFT_FLOAT__ */
	    p_l = v-(p_h-u);
	    z_h = cp_h*p_h;		/* cp_h+cp_l = 2/(3*log2) */
	    z_l = cp_l*p_h+p_l*cp+dp_l[k];
	    /* log2(ax) = (s+..)*2/(3*log2) = n + dp_h + z_h + z_l */
	    t = (double)n;
	    t1 = (((z_h+z_l)+dp_h[k])+t);
#ifdef __MATH_SOFT_FLOAT__
	    SET_LOW_WORD(t1,0);
#else  /* __MATH_SOFT_FLOAT__ */
	    __inst_reset_low_D(t1);
#endif /* __MATH_SOFT_FLOAT__ */
	    t2 = z_l-(((t1-t)-dp_h[k])-z_h);
	}

	s = one; /* s (sign of result -ve**odd) = -1 else = 1 */
	if(((((__uint32_t)hx>>31)-1)|(yisint-1))==0)
	    	s = -s;/* (-ve)**(odd int) */

    	/* split up y into y1+y2 and compute (y1+y2)*(t1+t2) */
	y1  = y;
#ifdef __MATH_SOFT_FLOAT__
	    SET_LOW_WORD(t1,0);
#else  /* __MATH_SOFT_FLOAT__ */
	    __inst_reset_low_D(y1);
#endif /* __MATH_SOFT_FLOAT__ */
	p_l = (y-y1)*t1+y*t2;
	p_h = y1*t1;
	z = p_l+p_h;
	EXTRACT_WORDS(j,i,z);
	if (j>=0x40900000) {				/* z >= 1024 */
	    if((((j-0x40900000)|i)!=0) || (p_l+ovt>z-p_h)) {		/* if z > 1024 */
		errno=ERANGE;
		return s*huge*huge;			/* overflow */
	    }
	} else if((j&0x7fffffff)>=0x4090cc00 ) {	/* z <= -1075 */
	    if((((j-0xc090cc00)|i)!=0) || (p_l<=z-p_h)) {		/* z < -1075 */
		errno=ERANGE;
		return s*tiny*tiny;		/* underflow */
	    }
	}
    	/*
     	* compute 2**(p_h+p_l)
    	*/
	i = j&0x7fffffff;

	n = 0;
	if(i>0x3fe00000) {		/* if |z| > 0.5, set n = [z+0.5] */
	    k = (i>>20)-0x3ff;
	    n = j+(0x00100000>>(k+1));
	    k = ((n&0x7fffffff)>>20)-0x3ff;	/* new k for n */
#ifdef __MATH_SOFT_FLOAT__
	    t = zero;
	    SET_HIGH_WORD(t,n&~(0x000fffff>>k));
#else /* __MATH_SOFT_FLOAT__ */
	    __inst_ldi_D_H (t, n&~(0x000fffff>>k));
#endif /* __MATH_SOFT_FLOAT__ */
	    n = ((n&0x000fffff)|0x00100000)>>(20-k);
	    if(j<0) n = -n;
	    p_h -= t;
	} 
	t = p_l+p_h;
#ifdef __MATH_SOFT_FLOAT__
	    SET_LOW_WORD(t,0);
#else  /* __MATH_SOFT_FLOAT__ */
	    __inst_reset_low_D(t);
#endif /* __MATH_SOFT_FLOAT__ */
	u = t*lg2_h;
	v = (p_l-(t-p_h))*lg2+t*lg2_l;
	z = u+v;
	w = v-(z-u);
	t  = z*z;
#ifdef __MATH_FORCE_EXPAND_POLY__
	t1  = z - t*(P1+t*(P2+t*(P3+t*(P4+t*P5))));
#else /* __MATH_FORCE_EXPAND_POLY__ */
	t1 = z - t * poly(t,PC,0);
#endif /* __MATH_FORCE_EXPAND_POLY__ */

#ifdef __MATH_CONST_FROM_MEMORY__
	two=2.0;
#else /* __MATH_CONST_FROM_MEMORY__ */
	__inst_ldi_D_W(two,2);
#endif /* __MATH_CONST_FROM_MEMORY__ */

	r  = (z*t1)/(t1-two)-(w+z*w);
	z  = one-(r-z);
	GET_HIGH_WORD(j,z);
	j += (n<<20);
#ifdef __MATH_SUBNORMAL__
	if((j>>20)<=0) z = scalbn(z,(int)n);	/* subnormal output */
	else
#endif /* __MATH_SUBNORMAL__ */
	SET_HIGH_WORD(z,j);
	return s*z;
}

#endif /* defined(_DOUBLE_IS_32BITS) */
