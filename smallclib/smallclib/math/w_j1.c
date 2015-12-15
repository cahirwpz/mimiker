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
*       file : $RCSfile: w_j1.c,v $ 
*         author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/


/* @(#)w_j1.c 5.1 93/09/24 */
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
 * wrapper of j1,y1 
 */

#include "low/_math.h"
#include <errno.h>

#ifndef _DOUBLE_IS_32BITS

#ifdef __STDC__
  double j1(double x)   /* wrapper j1 */
#else
  double j1(x)      /* wrapper j1 */
  double x;
#endif
{
#ifdef _IEEE_LIBM
  return __ieee754_j1(x);
#else
  double z;
  z = __ieee754_j1(x);
  if(fabs(x)>X_TLOSS) 
  {
      /* j1(|x|>X_TLOSS) */
    errno = ERANGE;
    return 0.0; 
  } 
  else
    return z;
#endif
}

#ifdef __STDC__
  double y1(double x)   /* wrapper y1 */
#else
  double y1(x)      /* wrapper y1 */
  double x;
#endif
{
#ifdef _IEEE_LIBM
  return __ieee754_y1(x);
#else
  double z;
  z = __ieee754_y1(x);
  if(x <= 0.0)
  {
#ifndef HUGE_VAL 
#define HUGE_VAL inf
    double inf = 0.0;

    SET_HIGH_WORD(inf,0x7ff00000);  /* set inf to infinite */
#endif
      /* y1(0) = -inf  or y1(x<0) = NaN */
    errno = EDOM;
    return -HUGE_VAL;              
  }
  if(x>X_TLOSS) 
  {
      /* y1(x>X_TLOSS) */
    errno = ERANGE;
    return 0.0; 
  } 
  else
    return z;
#endif
}

#endif /* defined(_DOUBLE_IS_32BITS) */

