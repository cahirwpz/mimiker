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
*              file : $RCSfile: s_ldexp.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

/* @(#)s_ldexp.c 5.1 93/09/24 */
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
       <<ldexp>>, <<ldexpf>>---load exponent

INDEX
	ldexp
INDEX
	ldexpf

ANSI_SYNOPSIS
       #include <math.h>
       double ldexp(double <[val]>, int <[exp]>);
       float ldexpf(float <[val]>, int <[exp]>);

TRAD_SYNOPSIS
       #include <math.h>

       double ldexp(<[val]>, <[exp]>)
              double <[val]>;
              int <[exp]>;

       float ldexpf(<[val]>, <[exp]>)
              float <[val]>;
              int <[exp]>;


DESCRIPTION
<<ldexp>> calculates the value 
@ifnottex
<[val]> times 2 to the power <[exp]>.
@end ifnottex
@tex
$val\times 2^{exp}$.
@end tex
<<ldexpf>> is identical, save that it takes and returns <<float>>
rather than <<double>> values.

RETURNS
<<ldexp>> returns the calculated value.

Underflow and overflow both set <<errno>> to <<ERANGE>>.
On underflow, <<ldexp>> and <<ldexpf>> return 0.0.
On overflow, <<ldexp>> returns plus or minus <<HUGE_VAL>>.

PORTABILITY
<<ldexp>> is ANSI. <<ldexpf>> is an extension.
              
*/   
#include "low/_math.h"
#include <errno.h>

#ifndef _DOUBLE_IS_32BITS

#define isfinite_bits(hx,lx) (hx<0xffe00000)
#define iszero_bits(hx,lx)   ((hx|lx) == 0)

#ifdef __STDC__
	double ldexp(double value, int exp)
#else
	double ldexp(value, exp)
	double value; int exp;
#endif
{
  __uint32_t hx,lx;
  EXTRACT_WORDS(hx,lx,value);
  hx=hx<<1;
  if(!isfinite_bits(hx,lx)||iszero_bits(hx,lx)) return value;
  value = scalbn(value,exp);
  if(!isfinite_bits(hx,lx)||iszero_bits(hx,lx))  errno = ERANGE;
  return value;
}

#endif /* _DOUBLE_IS_32BITS */
