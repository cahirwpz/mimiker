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
*              file : $RCSfile: s_nextafter.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

/* @(#)s_nextafter.c 5.1 93/09/24 */
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
       <<nextafter>>, <<nextafterf>>---get next number

INDEX
	nextafter
INDEX
	nextafterf

ANSI_SYNOPSIS
       #include <math.h>
       double nextafter(double <[val]>, double <[dir]>);
       float nextafterf(float <[val]>, float <[dir]>);

TRAD_SYNOPSIS
       #include <math.h>

       double nextafter(<[val]>, <[dir]>)
              double <[val]>;
              double <[exp]>;

       float nextafter(<[val]>, <[dir]>)
              float <[val]>;
              float <[dir]>;


DESCRIPTION
<<nextafter>> returns the double-precision floating-point number
closest to <[val]> in the direction toward <[dir]>.  <<nextafterf>>
performs the same operation in single precision.  For example,
<<nextafter(0.0,1.0)>> returns the smallest positive number which is
representable in double precision.

RETURNS
Returns the next closest number to <[val]> in the direction toward
<[dir]>.

PORTABILITY
	Neither <<nextafter>> nor <<nextafterf>> is required by ANSI C
	or by the System V Interface Definition (Issue 2).
*/

/* IEEE functions
 *	nextafter(x,y)
 *	return the next machine floating-point number of x in the
 *	direction toward y.
 *   Special cases:
 */

#include "low/_math.h"

#ifndef _DOUBLE_IS_32BITS

#ifdef __STDC__
	double nextafter(double x, double y)
#else
	double nextafter(x,y)
	double x,y;
#endif
{
  __int32_t	hx,hy,ix,iy;
  __uint32_t lx,ly;
  EXTRACT_WORDS(hx,lx,x);
  EXTRACT_WORDS(hy,ly,y);
  ix = hx&0x7fffffff;		/* |x| */
  iy = hy&0x7fffffff;		/* |y| */
  if(((ix>=0x7ff00000)&&((ix-0x7ff00000)|lx)!=0) ||   /* x is nan */ 
    ((iy>=0x7ff00000)&&((iy-0x7ff00000)|ly)!=0))     /* y is nan */ 
      return x+y;			
  if(x==y) return y;		/* x=y, return y */
  if((ix|lx)==0) {			/* x == 0 */
    INSERT_WORDS(x,hy&0x80000000,1);	/* return +-minsubnormal */
#ifdef __MATH_EXCEPTION__ 	    
    y = x*x;
    if(y==x)  return y;
    else return x;	/* raise underflow flag */
#else
    return x;
#endif	    
  } 
  if((hx>hy||((hx==hy)&&(lx>ly))) || (hx<0 && hy>=0)) {
    if(lx==0) hx -= 1;
      lx -= 1;
  } else {
    lx += 1;
    if(lx==0) hx += 1;
  }
#ifdef __MATH_EXCEPTION__	
  hy = hx&0x7ff00000;
  if(hy>=0x7ff00000) return x+x;	/* overflow  */
  if(hy<0x00100000) {		/* underflow */
    y = x*x;
    if(y!=x) {	/* raise underflow flag */
      INSERT_WORDS(y,hx,lx);
      return y;
    }
  }
#endif
  INSERT_WORDS(x,hx,lx);
  return x;
}

#endif /* _DOUBLE_IS_32BITS */
