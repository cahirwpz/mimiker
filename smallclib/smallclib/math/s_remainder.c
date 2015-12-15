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
* 		  file : $RCSfile: s_remainder.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* From Newlib 2.0, optimized in place for size.
Wrapper code absorbed in to function. */

/* @(#)e_remainder.c 5.1 93/09/24 */
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

/* remainder(x,p)
 * Return :                  
 * 	returns  x REM p  =  x - [x/p]*p as if in infinite 
 * 	precise arithmetic, where [x/p] is the (infinite bit) 
 *	integer nearest x/p (in half way case choose the even one).
 * Method : 
 *	Based on fmod() return x-[x/p]chopped*p exactlp.
 */

#include "low/_math.h"
#include <errno.h>
#include "low/_fpuinst.h"

#ifndef _DOUBLE_IS_32BITS

#ifdef __STDC__
	double remainder(double x, double p)
#else
	double remainder(x,p)
	double x,p;
#endif
{
	__int32_t hx,hp;
	__uint32_t sx,lp;
	double p_half;

	GET_HIGH_WORD(hx,x);
	EXTRACT_WORDS(hp,lp,p);
	sx = hx&0x80000000;
	hp &= 0x7fffffff;
	hx &= 0x7fffffff;

    /* purge off exception values */
	if((hp|lp)==0) {
		errno=EDOM;
		return (p/p);
	}

#ifdef __MATH_NONFINITE__ 
	if((hx>=0x7ff00000)||			/* x not finite */
	  ((hp>=0x7ff00000)&&			/* p is NaN */
	  (((hp-0x7ff00000)|lp)!=0)))
	{
		return (x*p)/(x*p);
	}
#endif /* __MATH_NONFINITE__ */

	if (hp<=0x7fdfffff) x = fmod(x,p+p);	/* now x < 2p */

#ifdef __MATH_SOFT_FLOAT__
	x  = fabs(x);
	p  = fabs(p);
#else /* __MATH_SOFT_FLOAT__ */
	__inst_abs_D(x,x);
	__inst_abs_D(p,p);
#endif /* __MATH_SOFT_FLOAT__ */

	if (x==p) x=x-x; /* Return 0 with size of x */

#ifndef __MATH_CONST_FROM_MEMORY__
	__inst_ldi_D_H(p_half,0x3fe00000);
#else /* __MATH_CONST_FROM_MEMORY__ */
	p_half=0.5;
#endif /* __MATH_CONST_FROM_MEMORY__ */
	p_half=p*p_half;
	
	if(x>p_half) {
		x-=p;
		if(x>=p_half) x -= p;
	}

	GET_HIGH_WORD(hx,x);
	SET_HIGH_WORD(x,hx^sx);
	return x;
}
#endif /* defined(_DOUBLE_IS_32BITS) */
