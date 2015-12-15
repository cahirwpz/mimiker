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
*              file : $RCSfile: s_pow10.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

/* @(#)s_pow10.c 5.1 93/09/24 */
/* Modification from s_exp10.c Yaakov Selkowitz 2007.  */

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
	<<pow10>>, <<pow10f>>---exponential
INDEX
	pow10
INDEX
	pow10f

ANSI_SYNOPSIS
	#include <math.h>
	double pow10(double <[x]>);
	float pow10f(float <[x]>);

TRAD_SYNOPSIS
	#include <math.h>
	double pow10(<[x]>);
	double <[x]>;

	float pow10f(<[x]>);
	float <[x]>;

DESCRIPTION
	<<pow10>> and <<pow10f>> calculate 10 ^ <[x]>, that is, 
	@ifnottex
	10 raised to the power <[x]>.
	@end ifnottex
	@tex
	$10^x$
	@end tex

	You can use the (non-ANSI) function <<matherr>> to specify
	error handling for these functions.

RETURNS
	On success, <<pow10>> and <<pow10f>> return the calculated value.
	If the result underflows, the returned value is <<0>>.  If the
	result overflows, the returned value is <<HUGE_VAL>>.  In
	either case, <<errno>> is set to <<ERANGE>>.

PORTABILITY
	<<pow10>> and <<pow10f>> are GNU extensions.
*/

/*
 * wrapper pow10(x)
 */

#undef pow10
#include "low/_math.h"
#include <errno.h>
#include <math.h>

#ifndef _DOUBLE_IS_32BITS

#ifdef __STDC__
	double pow10(double x)		/* wrapper pow10 */
#else
	double pow10(x)			/* wrapper pow10 */
	double x;
#endif
{
  return pow(10.0, x);
}

#endif /* defined(_DOUBLE_IS_32BITS) */
