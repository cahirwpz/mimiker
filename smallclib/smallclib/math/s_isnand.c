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
* 		  file : $RCSfile: s_isnand.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* From Newlib 2.0, no change */

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
	<<isnan>>, <<isnanf>>, <<isinf>>, <<isinff>>, <<finite>>, <<finitef>>---test for exceptional numbers

INDEX
	isnan
INDEX
	isinf
INDEX
	finite

INDEX
	isnanf
INDEX
	isinff
INDEX
	finitef

ANSI_SYNOPSIS
	#include <ieeefp.h>
	int isnan(double <[arg]>);
	int isinf(double <[arg]>);
	int finite(double <[arg]>);
	int isnanf(float <[arg]>);
	int isinff(float <[arg]>);
	int finitef(float <[arg]>);

TRAD_SYNOPSIS
	#include <ieeefp.h>
	int isnan(<[arg]>)
	double <[arg]>;
	int isinf(<[arg]>)
	double <[arg]>;
	int finite(<[arg]>);
	double <[arg]>;
	int isnanf(<[arg]>);
	float <[arg]>;
	int isinff(<[arg]>);
	float <[arg]>;
	int finitef(<[arg]>);
	float <[arg]>;


DESCRIPTION
	These functions provide information on the floating-point
	argument supplied.

	There are five major number formats:
	o+
	o zero
	  A number which contains all zero bits.
	o subnormal
	  A number with a zero exponent but a nonzero fraction.
	o normal
	  A number with an exponent and a fraction.
     	o infinity
	  A number with an all 1's exponent and a zero fraction.
	o NAN
	  A number with an all 1's exponent and a nonzero fraction.

	o-

	<<isnan>> returns 1 if the argument is a nan. <<isinf>>
	returns 1 if the argument is infinity.  <<finite>> returns 1 if the
	argument is zero, subnormal or normal.

	Note that by the C99 standard, <<isnan>> and <<isinf>> are macros
	taking any type of floating-point and are declared in
	<<math.h>>.  Newlib has chosen to declare these as macros in
	<<math.h>> and as functions in <<ieeefp.h>>.
	
	The <<isnanf>>, <<isinff>> and <<finitef>> functions perform the same
	operations as their <<isnan>>, <<isinf>> and <<finite>>
	counterparts, but on single-precision floating-point numbers.

QUICKREF
	isnan - pure
QUICKREF
	isinf - pure
QUICKREF
	finite - pure
QUICKREF
	isnan - pure
QUICKREF
	isinf - pure
QUICKREF
	finite - pure
*/

/*
 * __isnand(x) returns 1 if x is nan, else 0;
 * no branching!
 */

#include "low/_math.h"

#ifndef _DOUBLE_IS_32BITS

int
_DEFUN (__isnand, (x),
	double x)
{
	__int32_t hx,lx;
	EXTRACT_WORDS(hx,lx,x);
  	hx &= 0x7fffffff;
	hx |= (__uint32_t)(lx|(-lx))>>31;	
	hx = 0x7ff00000 - hx;
	return (int)(((__uint32_t)(hx))>>31);
}

#endif /* _DOUBLE_IS_32BITS */
