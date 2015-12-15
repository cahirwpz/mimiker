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
* 		  file : $RCSfile: fma.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* Fused multiply-add, double precision with no intermediate rounding.
   Ref: "Accurate Sum and Dot Product with Applications" - Ogita, Rump, Oishi
   http://www.ti3.tu-harburg.de/paper/rump/OgRuOi04a.pdf

   Addition is due to Knuth{Vol:II} and multiplication due to T.J. Dekker[1971]*/

#include <stdlib.h>
#include "low/_math.h"
#include "low/_fpuinst.h"

/* x*y+z - fused implementation */

static const double DoubleSplitter=((double)(1<<27)+1.0);

static void split (double value, double *const upper, double *const lower)
{
  const double scale = DoubleSplitter * value;
  __uint32_t hi,lo;

  EXTRACT_WORDS (hi, lo, scale);
  hi=hi<<1;
  if ((hi==0xffe00000) && (lo==0)) { /* scale == INF */
    *upper = value;
    *lower = 0;
  } 
  else {
    (*upper) = (scale - (scale - value)); 
    (*lower) = value - (*upper);
  }
  return;
}

static void long_product (double a, double b, double *const up, double *const lp)
{
  double ua, ub, la, lb;
  __uint32_t hi, lo;

  (*up) = a * b;
  split (a, &ua, &la);
  split (b, &ub, &lb);

  EXTRACT_WORDS (hi, lo, *up);
  hi=hi<<1;
  if ((hi == 0xffe00000) && (lo == 0))  /* a*b == INF */
    *lp = 0;
  else
    (*lp) = (la * lb - ((( (*up) - ua * ub ) - la * ub) - ua * lb));

  return;
}

static void long_sum (double a, double b, double *const us, double *const ls)
{
  double z;
  (*us) = a + b;
  z = (*us) - a;
  (*ls) = ((a - ((*us) - z)) + (b - z));
  return;
}

double fma (double x, double y, double z)
{
  double up, lp, us2, ls2;
  __uint32_t ix, iy, iz, hi, lo;
  long_product (x, y, &up, &lp);

  EXTRACT_WORDS (hi, lo, z);
  hi=hi<<1;
  if ((hi > 0xffe00000) || ((hi == 0xffe00000) && (lo != 0))) /* isnan(z) */
    return z;

  EXTRACT_WORDS (hi, lo, up);
  hi=hi<<1;
  if ((hi == 0xffe00000) && (lo == 0)) /* isinf (y) */
    return up;

  if (up+lp != up)
    lp = 0;

  GET_HIGH_WORD(ix, z);
  GET_HIGH_WORD(iy, up);
  GET_HIGH_WORD(iz, lp);
  ix&=0x7fffffff;
  iy&=0x7fffffff;
  iz&=0x7fffffff;

  if (abs((int)ix - (int)iz) == abs((int)ix - (int)iy)) {
    GET_LOW_WORD(ix, lp);
    GET_LOW_WORD(iy, up);
    GET_LOW_WORD(iz, lp);
  }

  if (abs((int)ix - (int)iz) < abs((int)ix - (int)iy)) {
    double tmp=lp; lp=up; up=tmp;
  };

  long_sum (z, up, &us2, &ls2);

  GET_HIGH_WORD(ix, lp);
  GET_HIGH_WORD(iy, us2);
  GET_HIGH_WORD(iz, ls2);
  ix&=0x7fffffff;	
  iy&=0x7fffffff;	
  iz&=0x7fffffff;	
			
  if (abs((int)ix - (int)iz) == abs((int)ix - (int)iy)) {
    GET_LOW_WORD(ix, lp);
    GET_LOW_WORD(iy, us2);
    GET_LOW_WORD(iz, ls2);
  }
			
  if (abs((int)ix - (int)iz) < abs((int)ix - (int)iy)){
    double tmp=ls2; ls2=us2; us2=tmp;
  };

  return (lp+us2)+ls2;
}
