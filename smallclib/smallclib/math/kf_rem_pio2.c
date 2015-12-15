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
* 		  file : $RCSfile: kf_rem_pio2.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* Snarfed from Newlib v2.0, optimized for size 
- Last 2 arguments (prec & two_over_pi) are fixed. 
- Type of array two_over_pi changed from 32-bit integers to 8-bit integers.
- Since prec==2, other cases need not be handled */ 

/* kf_rem_pio2.c -- float version of k_rem_pio2.c
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

/* In the float version, the input parameter x contains 8 bit
   integers, not 24 bit integers.  113 bit precision is not supported.  */

/*
 * Table of constants for 2/pi, 396 Hex digits (476 decimal) of 2/pi 
 */
#ifdef __STDC__
static const __uint8_t ipio2[] = {
#else
static __int8_t ipio2[] = {
#endif
0xA2, 0xF9, 0x83, 0x6E, 0x4E, 0x44, 0x15, 0x29, 0xFC,
0x27, 0x57, 0xD1, 0xF5, 0x34, 0xDD, 0xC0, 0xDB, 0x62, 
0x95, 0x99, 0x3C, 0x43, 0x90, 0x41, 0xFE, 0x51, 0x63,
0xAB, 0xDE, 0xBB, 0xC5, 0x61, 0xB7, 0x24, 0x6E, 0x3A, 
0x42, 0x4D, 0xD2, 0xE0, 0x06, 0x49, 0x2E, 0xEA, 0x09,
0xD1, 0x92, 0x1C, 0xFE, 0x1D, 0xEB, 0x1C, 0xB1, 0x29, 
0xA7, 0x3E, 0xE8, 0x82, 0x35, 0xF5, 0x2E, 0xBB, 0x44,
0x84, 0xE9, 0x9C, 0x70, 0x26, 0xB4, 0x5F, 0x7E, 0x41, 
0x39, 0x91, 0xD6, 0x39, 0x83, 0x53, 0x39, 0xF4, 0x9C,
0x84, 0x5F, 0x8B, 0xBD, 0xF9, 0x28, 0x3B, 0x1F, 0xF8, 
0x97, 0xFF, 0xDE, 0x05, 0x98, 0x0F, 0xEF, 0x2F, 0x11,
0x8B, 0x5A, 0x0A, 0x6D, 0x1F, 0x6D, 0x36, 0x7E, 0xCF, 
0x27, 0xCB, 0x09, 0xB7, 0x4F, 0x46, 0x3F, 0x66, 0x9E,
0x5F, 0xEA, 0x2D, 0x75, 0x27, 0xBA, 0xC7, 0xEB, 0xE5, 
0xF1, 0x7B, 0x3D, 0x07, 0x39, 0xF7, 0x8A, 0x52, 0x92,
0xEA, 0x6B, 0xFB, 0x5F, 0xB1, 0x1F, 0x8D, 0x5D, 0x08, 
0x56, 0x03, 0x30, 0x46, 0xFC, 0x7B, 0x6B, 0xAB, 0xF0,
0xCF, 0xBC, 0x20, 0x9A, 0xF4, 0x36, 0x1D, 0xA9, 0xE3, 
0x91, 0x61, 0x5E, 0xE6, 0x1B, 0x08, 0x65, 0x99, 0x85,
0x5F, 0x14, 0xA0, 0x68, 0x40, 0x8D, 0xFF, 0xD8, 0x80, 
0x4D, 0x73, 0x27, 0x31, 0x06, 0x06, 0x15, 0x56, 0xCA,
0x73, 0xA8, 0xC9, 0x60, 0xE2, 0x7B, 0xC0, 0x8C, 0x6B, 
};

#ifdef __STDC__
static const float PIo2[] = {
#else
static float PIo2[] = {
#endif
  1.5703125000e+00, /* 0x3fc90000 */
  4.5776367188e-04, /* 0x39f00000 */
  2.5987625122e-05, /* 0x37da0000 */
  7.5437128544e-08, /* 0x33a20000 */
  6.0026650317e-11, /* 0x2e840000 */
  7.3896444519e-13, /* 0x2b500000 */
  5.3845816694e-15, /* 0x27c20000 */
  5.6378512969e-18, /* 0x22d00000 */
  8.3009228831e-20, /* 0x1fc40000 */
  3.2756352257e-22, /* 0x1bc60000 */
  6.3331015649e-25, /* 0x17440000 */
};

#ifdef __STDC__
static const float			
#else
static float			
#endif
zero   = 0.0,
one    = 1.0,
two8   =  2.5600000000e+02, /* 0x43800000 */
twon8  =  3.9062500000e-03; /* 0x3b800000 */

#ifdef __STDC__
	int __kernel_rem_pio2f(float *x, float *y, int e0, int nx) 
#else
	int __kernel_rem_pio2f(x,y,e0,nx,ipio2) 	
	float x[], y[]; int e0,nx; __int32_t ipio2[];
#endif
{
	__int32_t jz,jx,jv,jp,carry,n,iq[20],i,j,k,m,q0,ih;
	float z,fw,fw2,f[20],fq[20],q[20];
	const int jk=9; /* Fixed by virtue of precision always = 2 */
	__int32_t iqtmp;

    /* initialize jk*/
	jp = jk;

    /* determine jx,jv,q0, note that 3>q0 */
	jx =  nx-1;
	jv = (e0-3)/8; if(jv<0) jv=0;
	q0 =  e0-8*(jv+1);

    /* set up f[0] to f[jx+jk] where f[jx+jk] = ipio2[jv+jk] */
	j = jv-jx; m = jx+jk;
	for(i=0;i<=m;i++,j++) f[i] = (j<0)? zero : (float) ipio2[j];

    /* compute q[0],q[1],...q[jk] */
	for (i=0;i<=jk;i++) {
	    for(j=0,fw=0.0;j<=jx;j++) fw += x[j]*f[jx+i-j]; q[i] = fw;
	}

	jz = jk;
recompute:
    /* distill q[] into iq[] reversingly */
	for(i=0,j=jz,z=q[jz];j>0;i++,j--) {
	    fw    =  (float)((__int32_t)(twon8* z));
	    iq[i] =  (__int32_t)(z-two8*fw);
	    z     =  q[j-1]+fw;
	}

    /* compute n */
	z  = scalbnf(z,(int)q0);	/* actual value of z */
	z -= (float)8.0*floorf(z*(float)0.125);	/* trim off integer >= 8 */
	n  = (__int32_t) z;
	z -= (float)n;
	ih = 0;

	iqtmp=iq[jz-1];
	if(q0>0) {	/* need iq[jz-1] to determine n */
	    i  = (iqtmp>>(8-q0)); n += i;
	    iqtmp -= i<<(8-q0);
	    ih = iqtmp>>(7-q0);
	    iq[jz-1] = iqtmp;
	} 
	else if(q0==0) ih = iqtmp>>8;
	else if(z>=(float)0.5) ih=2;

	if(ih>0) {	/* q > 0.5 */
	    n += 1; carry = 0;
	    for(i=0;i<jz ;i++) {	/* compute 1-q */
		j = iq[i];
		if(carry==0) {
		    if(j!=0) {
			carry = 1; iq[i] = 0x100- j;
		    }
		} else  iq[i] = 0xff - j;
	    }

	    if(q0>0) {		/* rare case: chance is 1 in 12 */
		    iq[jz-1] &= ((q0&0x1)<<6)|0x3f;
	    }

	    if(ih==2) {
		z = one - z;
		if(carry!=0) z -= scalbnf(one,(int)q0);
	    }
	}

    /* check if recomputation is needed */
	if(z==zero) {
	    j = 0;
	    for (i=jz-1;i>=jk;i--) j |= iq[i];
	    if(j==0) { /* need recomputation */
		for(k=1;iq[jk-k]==0;k++);   /* k = no. of terms needed */

		for(i=jz+1;i<=jz+k;i++) {   /* add q[jz+1] to q[jz+k] */
		    f[jx+i] = (float) ipio2[jv+i];
		    for(j=0,fw=0.0;j<=jx;j++) 
			fw += x[j]*f[jx+i-j];
		    q[i] = fw;
		}
		jz += k;
		goto recompute;
	    }
	}

    /* chop off zero terms */
	if(z==(float)0.0) {
	    jz -= 1; q0 -= 8;
	    while(iq[jz]==0) { jz--; q0-=8;}
	} else { /* break z into 8-bit if necessary */
	    z = scalbnf(z,-(int)q0);
	    if(z>=two8) { 
		fw = (float)((__int32_t)(twon8*z));
		iq[jz] = (__int32_t)(z-two8*fw);
		jz += 1; q0 += 8;
		z = fw;
	    }
	    iq[jz] = (__int32_t) z ;
	}

    /* convert integer "bit" chunk to floating-point value */
	fw = scalbnf(one,(int)q0);
	for(i=jz;i>=0;i--) {
	    q[i] = fw*(float)iq[i]; fw*=twon8;
	}

    /* compute PIo2[0,...,jp]*q[jz,...,0] */
	for(i=jz;i>=0;i--) {
	    for(fw=0.0,k=0;k<=jp&&k<=jz-i;k++) fw += PIo2[k]*q[i+k];
	    fq[jz-i] = fw;
	}

    /* compress fq[] into y[] */
    /* Since prec is always 2 from the caller, remaining cases(prec={0,1,3} can been removed */
	fw = 0.0;
	for (i=jz;i>=0;i--) fw += fq[i];
	fw2 = fq[0]-fw;
	for (i=1;i<=jz;i++) fw2 += fq[i];
	if (ih != 0) {fw=-fw; fw2=-fw2;}

	y[0] = fw; y[1] = fw2;
	return n;
}
