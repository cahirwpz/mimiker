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
*              file : $RCSfile: ef_pow.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

/* ef_pow.c -- float version of e_pow.c.
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
#include "low/_gpinst.h"
#include "low/_fpuinst.h"
#include <errno.h>

#ifdef __v810__
#define const 
#endif

#ifdef __STDC__
static const float
#else
static float
#endif
#ifdef __MATH_CONST_FROM_MEMORY__
bp[] = {1.0, 1.5,},
#endif /* __MATH_CONST_FROM_MEMORY__ */
dp_h[] = { 0.0, 5.84960938e-01,}, /* 0x3f15c000 */
dp_l[] = { 0.0, 1.56322085e-06,}, /* 0x35d1cfdc */
zero    =  0.0,
two24	=  16777216.0,	/* 0x4b800000 */
huge	=  1.0e30,
tiny    =  1.0e-30,
lg2  	=  6.9314718246e-01, /* 0x3f317218 */
lg2_h  	=  6.93145752e-01, /* 0x3f317200 */
lg2_l  	=  1.42860654e-06, /* 0x35bfbe8c */
ovt 	=  4.2995665694e-08, /* -(128-log2(ovfl+.5ulp)) */
cp    	=  9.6179670095e-01, /* 0x3f76384f =2/(3ln2) */
cp_h  	=  9.6179199219e-01, /* 0x3f763800 =head of cp */
cp_l  	=  4.7017383622e-06, /* 0x369dc3a0 =tail of cp_h */
ivln2   =  1.4426950216e+00, /* 0x3fb8aa3b =1/ln2 */
ivln2_h =  1.4426879883e+00, /* 0x3fb8aa00 =16b 1/ln2*/
ivln2_l =  7.0526075433e-06; /* 0x36eca570 =1/ln2 tail*/


#ifdef __MATH_FORCE_EXPAND_POLY__
static const float
	/* poly coefs for (3/2)*(log(x)-2s-2/3*s**3 */
L1  =  6.0000002384e-01, /* 0x3f19999a */
L2  =  4.2857143283e-01, /* 0x3edb6db7 */
L3  =  3.3333334327e-01, /* 0x3eaaaaab */
L4  =  2.7272811532e-01, /* 0x3e8ba305 */
L5  =  2.3066075146e-01, /* 0x3e6c3255 */
L6  =  2.0697501302e-01, /* 0x3e53f142 */
P1   =  1.6666667163e-01, /* 0x3e2aaaab */
P2   = -2.7777778450e-03, /* 0xbb360b61 */
P3   =  6.6137559770e-05, /* 0x388ab355 */
P4   = -1.6533901999e-06, /* 0xb5ddea0e */
P5   =  4.1381369442e-08, /* 0x3331bb4c */
#else /* __MATH_FORCE_EXPAND_POLY__ */
static const float
LC[] = {
    6.0000002384e-01, /* 0x3f19999a */
    4.2857143283e-01, /* 0x3edb6db7 */
    3.3333334327e-01, /* 0x3eaaaaab */
    2.7272811532e-01, /* 0x3e8ba305 */
    2.3066075146e-01  /* 0x3e6c3255 */
},
L6  =  2.0697501302e-01, /* 0x3e53f142 */
PC[] = {
    1.6666667163e-01, /* 0x3e2aaaab */
    -2.7777778450e-03, /* 0xbb360b61 */
    6.6137559770e-05, /* 0x388ab355 */
    -1.6533901999e-06, /* 0xb5ddea0e */
    4.1381369442e-08, /* 0x3331bb4c */
};

static float poly(float w, const float cf[], float retval)
{
	int i=4;
	while (i >= 0)
	  retval=w*retval+cf[i--];
	return retval;
}
#endif /* __MATH_FORCE_EXPAND_POLY__ */


#ifdef __STDC__
	float powf(float x, float y)
#else
	float powf(x,y)
	float x, y;
#endif
{
	float z,ax,z_h,z_l,p_h,p_l;
	float y1,t1,t2,r,s,t,u,v,w,two,one;
	__int32_t i,j,k,yisint,n;
	__int32_t hx,hy,ix,iy,is;

	GET_FLOAT_WORD(hx,x);
	GET_FLOAT_WORD(hy,y);
	ix = hx&0x7fffffff;  iy = hy&0x7fffffff;

#ifdef __MATH_CONST_FROM_MEMORY__
	one=1.0f;
#else /* __MATH_CONST_FROM_MEMORY__ */
	__inst_ldi_S_H(one,0x3f80);
#endif /* __MATH_CONST_FROM_MEMORY__ */

    	/* y==zero: x**0 = 1 */
	if(FLT_UWORD_IS_ZERO(iy)) return one; 	

    	/* x==+one: 1**y = 1 even if y is NaN*/
	if(x == one) return x;

#ifdef __MATH_NONFINITE__	
    	/* x|y==NaN return NaN unless x==1 then return 1 */
	if (FLT_UWORD_IS_NAN(ix))
	    return x;
	if (FLT_UWORD_IS_NAN(iy))
	    return y;
#endif /* __MATH_NONFINITE__ */

#ifdef __MATH_NONFINITE__
    	/* special value of y */
	if (FLT_UWORD_IS_INFINITE(iy)) {	/* y is +-inf */
	    if (ix==0x3f800000)
		return one;		/* +-1**+-inf = 1 */
	    else if(((ix >= 0x3f800000) && (hy >= 0)) || (!(ix >= 0x3f800000) && (hy < 0)))
		{SET_FLOAT_WORD(y,iy); return y;}
	    else
		return zero;
	}
#endif /* __MATH_NONFINITE__ */ 
	if(iy==0x3f800000) {	/* y is  +-1 */
	    if(hy<0) {
		if(FLT_UWORD_IS_ZERO(ix))
		    errno=EDOM;
		else if(FLT_UWORD_IS_SUBNORMAL(ix))
		    errno=ERANGE;
		return one/x; 
	    }
	    else return x;
	}

    	/* determine if y is an odd int when x < 0
     	* yisint = 0	... y is not an integer
     	* yisint = 1	... y is an odd int
     	* yisint = 2	... y is an even int
     	*/
	yisint  = 0;
	if(hx<0) {	
	    if(iy>=0x4b800000) yisint = 2; /* even integer y */
	    else if(iy>=0x3f800000) {
		k = (iy>>23)-0x7f;	   /* exponent */
		j = iy>>(23-k);
		if((j<<(23-k))==iy) yisint = 2-(j&1);
	    }		
	} 

#ifdef __MATH_SOFT_FLOAT__
	ax   = fabsf(x);
#else /* __MATH_SOFT_FLOAT__ */
	__inst_abs_S(ax,x);
#endif /* __MATH_SOFT_FLOAT__ */

    	/* special value of x */
	if(FLT_UWORD_IS_INFINITE(ix)||FLT_UWORD_IS_ZERO(ix)||ix==0x3f800000){
	    z = ax;			/*x is +-0,+-inf,+-1*/
	    if(hy<0) {
		if(FLT_UWORD_IS_ZERO(ix)) errno=EDOM;
		z = one/z;	/* z = (1/|x|) */
	    }
	    if(hx<0) {
		if(((ix-0x3f800000)|yisint)==0) {
		    errno=EDOM;
		    return (0.0/0.0); /* (-1)**non-int is NaN */
		} else if(yisint==1) 
		    z = -z;		/* (x<0)**odd = -(|x|**odd) */
	    }
	    return z;
	}	
    
    	/* (x<0)**(non-int) is NaN */
	if(((((__uint32_t)hx>>31)-1)|yisint)==0) {
	    errno=EDOM;
	    return (0.0/0.0);
	}

    	/* |y| is huge */
	if(iy>0x4d000000) { /* if |y| > 2**27 */
	    float half, quart;
	    /* over/underflow if x is not close to one */
	    if(ix<0x3f7ffff8) { errno=ERANGE; return (hy<0)? huge*huge:tiny*tiny; }
	    if(ix>0x3f800007) { errno=ERANGE; return (hy>0)? huge*huge:tiny*tiny; }

#ifdef __MATH_CONST_FROM_MEMORY__
	    half=0.5f;
	    quart=0.25f;
#else /* __MATH_CONST_FROM_MEMORY__ */
	    __inst_ldi_S_H (half, 0x3f00);
	    __inst_ldi_S_H (quart, 0x3e80);
#endif /* __MATH_CONST_FROM_MEMORY__ */

	    /* now |1-x| is tiny <= 2**-20, suffice to compute 
	       log(x) by x-x^2/2+x^3/3-x^4/4 */
	    t = ax-one;		/* t has 20 trailing zeros */
	    w = (t*t)*(half-t*((float)0.333333333333-t*quart));
	    u = ivln2_h*t;	/* ivln2_h has 16 sig. bits */
	    v = t*ivln2_l-w*ivln2;
	    t1 = u+v;
	    GET_FLOAT_WORD(is,t1);
#ifdef __HW_SET_BITS__
	    __inst_clear_lo_bits(is,12);
	    SET_FLOAT_WORD(t1,is);
#else /* __HW_SET_BITS__ */
	    SET_FLOAT_WORD(t1,is&0xfffff000);
#endif /* __HW_SET_BITS__ */
	    t2 = v-(t1-u);
	} else {
	    float s2,s_h,s_l,t_h,t_l,bpval,three;
	    n = 0;
#ifdef __MATH_SUBNORMAL__
	    /* take care subnormal number */
	    if(FLT_UWORD_IS_SUBNORMAL(ix)) {ax *= two24; n -= 24; GET_FLOAT_WORD(ix,ax);}
#endif /* __MATH_SUBNORMAL__ */
	    n  += ((ix)>>23)-0x7f;
	    j  = ix&0x007fffff;
	    /* determine interval */
	    ix = j|0x3f800000;		/* normalize ix */
	    if(j<=0x1cc471) k=0;	/* |x|<sqrt(3/2) */
	    else if(j<0x5db3d7) k=1;	/* |x|<sqrt(3)   */
	    else {k=0;n+=1;ix -= 0x00800000;}
	    SET_FLOAT_WORD(ax,ix);

	    /* compute s = s_h+s_l = (x-1)/(x+1) or (x-1.5)/(x+1.5) */
#ifdef __MATH_CONST_FROM_MEMORY__
	    bpval=bp[k];		/* bp[0]=1.0, bp[1]=1.5 */
#else /* __MATH_CONST_FROM_MEMORY__ */
	    SET_FLOAT_WORD (bpval, 0x3f800000|(k<<22));
#endif /* __MATH_CONST_FROM_MEMORY__ */
	    u = ax-bpval;		/* bp[0]=1.0, bp[1]=1.5 */
	    v = one/(ax+bpval);
	    
	    s = u*v;
	    s_h = s;
	    GET_FLOAT_WORD(is,s_h);
#ifdef __HW_SET_BITS__
	    __inst_clear_lo_bits(is,12);
	    SET_FLOAT_WORD(s_h,is);
#else /* __HW_SET_BITS__ */
	    SET_FLOAT_WORD(s_h,is&0xfffff000);
#endif /* __HW_SET_BITS__ */
	    /* t_h=ax+bp[k] High */
	    SET_FLOAT_WORD(t_h,((ix>>1)|0x20000000)+0x0040000+(k<<21));
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
	    three=3.0f;
#else /* __MATH_CONST_FROM_MEMORY__ */
	__inst_ldi_S_H(three,0x4040);
#endif /* __MATH_CONST_FROM_MEMORY__ */
	    t_h = three+s2+r;
	    GET_FLOAT_WORD(is,t_h);
#ifdef __HW_SET_BITS__
	    __inst_clear_lo_bits(is,12);
	    SET_FLOAT_WORD(t_h,is);
#else /* __HW_SET_BITS__ */
	    SET_FLOAT_WORD(t_h,is&0xfffff000);
#endif /* __HW_SET_BITS__ */
	    t_l = r-((t_h-three)-s2);
	    /* u+v = s*(1+...) */
	    u = s_h*t_h;
	    v = s_l*t_h+t_l*s;
	    /* 2/(3log2)*(s+...) */
	    p_h = u+v;
	    GET_FLOAT_WORD(is,p_h);
#ifdef __HW_SET_BITS__
	    __inst_clear_lo_bits(is,12);
	    SET_FLOAT_WORD(p_h,is);
#else /* __HW_SET_BITS__ */
	    SET_FLOAT_WORD(p_h,is&0xfffff000);
#endif /* __HW_SET_BITS__ */
	    p_l = v-(p_h-u);
	    z_h = cp_h*p_h;		/* cp_h+cp_l = 2/(3*log2) */
	    z_l = cp_l*p_h+p_l*cp+dp_l[k];
	    /* log2(ax) = (s+..)*2/(3*log2) = n + dp_h + z_h + z_l */
	    t = (float)n;
	    t1 = (((z_h+z_l)+dp_h[k])+t);
	    GET_FLOAT_WORD(is,t1);
#ifdef __HW_SET_BITS__
	    __inst_clear_lo_bits(is,12);
	    SET_FLOAT_WORD(t1,is);
#else /* __HW_SET_BITS__ */
	    SET_FLOAT_WORD(t1,is&0xfffff000);
#endif /* __HW_SET_BITS__ */
	    t2 = z_l-(((t1-t)-dp_h[k])-z_h);
	}

	s = one; /* s (sign of result -ve**odd) = -1 else = 1 */
	if(((((__uint32_t)hx>>31)-1)|(yisint-1))==0)
		s = -s;	/* (-ve)**(odd int) */

    	/* split up y into y1+y2 and compute (y1+y2)*(t1+t2) */
	GET_FLOAT_WORD(is,y);
#ifdef __HW_SET_BITS__
	__inst_clear_lo_bits(is,12);
	SET_FLOAT_WORD(y1,is);
#else /* __HW_SET_BITS__ */
	SET_FLOAT_WORD(y1,is&0xfffff000);
#endif /* __HW_SET_BITS__ */
	p_l = (y-y1)*t1+y*t2;
	p_h = y1*t1;
	z = p_l+p_h;
	GET_FLOAT_WORD(j,z);
	i = j&0x7fffffff;
	if (j>0) {
		if (i>FLT_UWORD_EXP_MAX || (i==FLT_UWORD_EXP_MAX && p_l+ovt>z-p_h)) {
			errno=ERANGE; 
	        	return s*huge*huge;			/* overflow */
		}
        } else {
		if (i>FLT_UWORD_EXP_MIN || (i==FLT_UWORD_EXP_MIN && p_l<=z-p_h)) {
			errno=ERANGE; 
	       		return s*tiny*tiny;			/* underflow */
		}
	}
    	/*
     	* compute 2**(p_h+p_l)
     	*/
	n = 0;
	if(i>0x3f000000) {		/* if |z| > 0.5, set n = [z+0.5] */
	    	k = (i>>23)-0x7f;
	    	n = j+(0x00800000>>(k+1));
	    	k = ((n&0x7fffffff)>>23)-0x7f;	/* new k for n */
	    	SET_FLOAT_WORD(t,n&~(0x007fffff>>k));
	    	n = ((n&0x007fffff)|0x00800000)>>(23-k);
	    	if(j<0) n = -n;
	    	p_h -= t;
	} 
	t = p_l+p_h;
	GET_FLOAT_WORD(is,t);
#ifdef __HW_SET_BITS__
	__inst_clear_lo_bits(is,12);
	SET_FLOAT_WORD(t,is);
#else /* __HW_SET_BITS__ */
	SET_FLOAT_WORD(t,is&0xfffff000);
#endif /* __HW_SET_BITS__ */
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
	two=2.0f;
#else /* __MATH_CONST_FROM_MEMORY__ */
	__inst_ldi_S_H(two,0x4000);
#endif /* __MATH_CONST_FROM_MEMORY__ */
	r  = (z*t1)/(t1-two)-(w+z*w);
	z  = one-(r-z);
	GET_FLOAT_WORD(j,z);
	j += (n<<23);
#ifdef __MATH_SUBNORMAL__
	if((j>>23)<=0) z = scalbnf(z,(int)n);	/* subnormal output */
	else
#endif /* __MATH_SUBNORMAL__ */
	SET_FLOAT_WORD(z,j);
	return s*z;
}
