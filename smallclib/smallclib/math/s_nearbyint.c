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
*              file : $RCSfile: s_nearbyint.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

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
<<nearbyint>>, <<nearbyintf>>--round to integer
INDEX
	nearbyint
INDEX
	nearbyintf

ANSI_SYNOPSIS
	#include <math.h>
	double nearbyint(double <[x]>);
	float nearbyintf(float <[x]>);

DESCRIPTION
The <<nearbyint>> functions round their argument to an integer value in
floating-point format, using the current rounding direction and
(supposedly) without raising the "inexact" floating-point exception.
See the <<rint>> functions for the same function with the "inexact"
floating-point exception being raised when appropriate.

BUGS
Newlib does not support the floating-point exception model, so that
the floating-point exception control is not present and thereby what may
be seen will be compiler and hardware dependent in this regard.
The Newlib <<nearbyint>> functions are identical to the <<rint>>
functions with respect to the floating-point exception behavior, and
will cause the "inexact" exception to be raised for most targets.

RETURNS
<[x]> rounded to an integral value, using the current rounding direction.

PORTABILITY
ANSI C, POSIX

SEEALSO
<<rint>>, <<round>>
*/

#include <math.h>
#include "low/_math.h"
#include "low/_fpuinst.h"
#include "low/_machine.inc"

#ifndef _DOUBLE_IS_32BITS

#ifdef __STDC__
	double nearbyint(double x)
#else
	double nearbyint(x)
	double x;
#endif
{
#ifdef __MATH_SOFT_FLOAT__
  	return rint(x);
#else
  	double xnr;
        __uint32_t flags;

        __inst_nearint_D(xnr, x, flags);

        /* Argument outside valid range [-2^63,2^63], or x=0 return argument */
        if (flags & FCSR_CAUSE_INVALIDOP_BIT)
        	xnr=x;

        __inst_copysign_D (xnr, x);

        return (xnr);
#endif
}

#endif /* _DOUBLE_IS_32BITS */
