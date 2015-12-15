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
* 		  file : $RCSfile: wf_jn.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/


/* wf_jn.c -- float version of w_jn.c.
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


#ifdef __STDC__
	float jnf(int n, float x)	/* wrapper jnf */
#else
	float jnf(n,x)			/* wrapper jnf */
	float x; int n;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_jnf(n,x);
#else
	float z;
	z = __ieee754_jnf(n,x);
	if(fabsf(x)>(float)X_TLOSS)
  {
	    /* jnf(|x|>X_TLOSS) */
    errno = ERANGE;
    return 0.0; 
	}
  else
	  return z;
#endif
}

#ifdef __STDC__
	float ynf(int n, float x)	/* wrapper ynf */
#else
	float ynf(n,x)			/* wrapper ynf */
	float x; int n;
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_ynf(n,x);
#else
	float z;
	z = __ieee754_ynf(n,x);
        if(x <= (float)0.0){
	    /* ynf(n,0) = -inf or ynf(x<0) = NaN */
#ifndef HUGE_VAL 
#define HUGE_VAL inf
	    double inf = 0.0;

	    SET_HIGH_WORD(inf,0x7ff00000);	/* set inf to infinite */
#endif
        errno = EDOM;
        return (float)0.0; 
        }
	if(x>(float)X_TLOSS) 
  {
	    /* ynf(x>X_TLOSS) */
    errno = ERANGE;
    return (float)0.0; 
	} 
  else
	  return z;
#endif
}

#ifdef _DOUBLE_IS_32BITS

#ifdef __STDC__
	double jn(int n, double x)
#else
	double jn(n,x)
	double x; int n;
#endif
{
	return (double) jnf(n, (float) x);
}

#ifdef __STDC__
	double yn(int n, double x)
#else
	double yn(n,x)
	double x; int n;
#endif
{
	return (double) ynf(n, (float) x);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
