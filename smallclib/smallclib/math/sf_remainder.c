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
* 		  file : $RCSfile: sf_remainder.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* From Newlib 2.0, optimized in place for size.
Wrapper code absorbed in to function. */

/* ef_remainder.c -- float version of e_remainder.c.
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
#include <errno.h>
#include "low/_fpuinst.h"

#ifdef __STDC__
	float remainderf(float x, float p)
#else
	float remainderf(x,p)
	float x,p;
#endif
{
	__int32_t hx,hp;
	__uint32_t sx;
	float p_half;

	GET_FLOAT_WORD(hx,x);
	GET_FLOAT_WORD(hp,p);
	sx = hx&0x80000000;
	hp &= 0x7fffffff;
	hx &= 0x7fffffff;

    /* purge off exception values */
	if(FLT_UWORD_IS_ZERO(hp))
	{
		errno=EDOM;
		return (p/p);
	}
#ifdef __MATH_NONFINITE__	
	if (!FLT_UWORD_IS_FINITE(hx)||
	   FLT_UWORD_IS_NAN(hp))
	    return (x*p)/(x*p);
#endif /* __MATH_NONFINITE__ */

	if (hp<=FLT_UWORD_HALF_MAX) x = fmodf(x,p+p); /* now x < 2p */

#ifdef __MATH_SOFT_FLOAT__
	x  = fabsf(x);
	p  = fabsf(p);
#else /* __MATH_SOFT_FLOAT__ */
	__inst_abs_S(x,x);
	__inst_abs_S(p,p);
#endif /* __MATH_SOFT_FLOAT__ */

	if (x==p) x=0;

#ifdef __MATH_CONST_FROM_MEMORY__
	p_half=0.5;
#else /* __MATH_CONST_FROM_MEMORY__ */
	__inst_ldi_S_H (p_half, 0x3f00);
#endif
	p_half = p_half*p;
	if(x>p_half) {
		x-=p;
		if(x>=p_half) x -= p;
	}

	GET_FLOAT_WORD(hx,x);
	SET_FLOAT_WORD(x,hx^sx);
	return x;
}
