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
*                 file : $RCSfile: strtod.c,v $ 
*               author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

#include <low/_flavour.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <low/_math.h>

#ifdef __USE_LOCALE__
#include <locale.h>
#endif

struct const_exp
{
   double fastPos[16];
   double binPos[5];
   double binNeg[5];
};

double
strtod (const char *nptr, char **endptr)
{
  const char *s = nptr;
  int flag_hex, flag_sign, flag_isdigit, flag_inf;
  int n, exponent, sign, nconv, i;
  int save_sign =0;
#ifndef __NO_HEX_FP__
  double p;
#endif	
	double number;
  
#ifdef __IEEE754__  
  unsigned long long ni;
#endif

#ifdef __IEEE754__
  char *str[] = {"INF", "INFINITY", "NAN"};
#endif /* __IEEE754__ */

  struct const_exp *exp_tab;

  struct const_exp tens = {
    {1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7,
    1e8, 1e9, 1e10, 1e11, 1e12, 1e13, 1e14, 1e15},
    {1e16, 1e32, 1e64, 1e128, 1e256},
    {1e-16, 1e-32, 1e-64, 1e-128, 1e-256}
  };
	
#ifndef __NO_HEX_FP__
  struct const_exp twos = {
    {1.0, 2.0, 4.0, 8.0, 16.0, 32.0, 64.0, 128.0, 256.0, 512.0,
    1024.0, 2048.0, 4096.0, 8192.0, 16384.0, 32768.0},
    {65536.0, 4294967296.0, 18446744073709551616.0,
    3.4028236692093846346337460743177e+38, 1.1579208923731619542357098500869e+77},
    {0.0000152587890625, 0.00000000023283064365386962890625,
    5.4210108624275221700372640043497e-20, 2.9387358770557187699218413430556e-39,
    8.6361685550944446253863518628004e-78}
  };
#endif

  flag_isdigit = flag_sign = sign = flag_hex = flag_inf= nconv = 0;
  number = 0.0;
  n = exponent = 0;

  (void) save_sign;
  
  while (isspace ((int)*s)) s++;

  /* This code is executed twice
  1) to check the sign of input number
  2) to check sign of exponent if input number is in scientific notation
  flag_sign is used to do appropriate processing
  */
Handle_sign:
  switch (*s)
    {
      case '-':
        sign = 1;
        /* deliberate fall through */
      case '+':
        s++;
    }
  if (flag_sign)
    goto EndSign;

#ifdef __IEEE754__
  for (i = 0; i < 3; i++)
    if (!strcasecmp(str[i], s))
      {
        nconv = 1;
        flag_inf = 1;
        s += i != 1 ? 3 : 8;

	if (i != 2)
    INSERT_WORDS (number, 0x7ff00000, 0);
	else
	  INSERT_WORDS (number, 0xfff80000, 0);
      }
#endif

#ifndef __NO_HEX_FP__
  if (*s == '0' && (s[1]|32) == 'x')
    {
      flag_hex = 1; // flag for hex input
      s += 2;
      p = 16.0;

      /* This code is executed twice
      1) to process the digits before decimal point
      2) to process the digits after decimal point
      flag_isdigit is used to do appropriate processing
      */
have_hex_dig:
      while (isxdigit ((int)*s))
        {
          nconv = 1;
          n++;

          if (flag_isdigit)
            {
              number += p* (isdigit((int)*s)?*s++-'0':(*s++|32)-'a'+10);
              p *= 1.0/16.0;
              exponent++;
            }
          else
		  number = number * p + (isdigit((int)*s) ? (*s++ - '0') : ((*s++ | 32) - 'a' + 10));
	}
      goto ProcessDecimalPt;
    }
#endif

  /* This code is executed twice
  1) to process the digits before decimal point
  2) to process the digits after decimal point
  flag_isdigit is used to do appropriate processing
  */
have_dig:
  while (isdigit ((int)*s))
    {
      nconv = 1;
      number = number * 10.0 + *s++ -'0' ;
      n++;

      if (flag_isdigit)
        exponent++;
    }

#ifndef __NO_HEX_FP__
ProcessDecimalPt:
#endif

  if (!flag_isdigit)
    {
      flag_isdigit = 1;
#ifndef __USE_LOCALE__
      if (*s == '.')
#else
      if (*s == *localeconv()->decimal_point)
#endif
        s++;

#ifndef __NO_HEX_FP__
      if (flag_hex == 1)
        {
          p = 1.0/16.0;
	  goto have_hex_dig;
        }
#endif
      goto have_dig;
    }

  exponent = -exponent;

#ifdef __IEEE754__
  if (n == 0 && !flag_inf)
    {
      number = 0.0;
      goto ret;
    }
#endif

  if (sign == 1)
    number = -number;
  if (
#ifndef __NO_HEX_FP__
    (flag_hex == 1 && (*s | 32) == 'p') ||
#endif
    (*s | 32) == 'e')
      {
	save_sign = sign;
        sign = 0;
        flag_sign = 1;
	s++;
        goto Handle_sign;

EndSign:
  n = 0;
  while (isdigit ((int)*s))
    {
      n = n * 10 + (*s++ -'0');
      if (n > 19999)
        n = 19999;
    }

#ifndef __NO_HEX_FP__
  if (flag_hex )
    {
      exponent = sign ? -n : n;
      p = 2.0;
    }
  else
    {
#endif
      if ((flag_sign = 1) && sign)
        exponent -= n;
      else
        exponent += n;
    }
#ifndef __NO_HEX_FP__
 }
#endif

#ifdef __IEEE754__
  EXTRACT_LLWORD (ni, number);
  if (exponent <= -324 )
    {
      number = number < 0.0 ? -0.0 : 0.0;
      goto SetErrno;
    }
  else if (exponent > 308 && ni << 1 == 0x0000000000000000ULL)
    goto ret;
  else if (exponent > 308)
    {
      if( save_sign == 1)
	INSERT_WORDS (number, 0xfff00000, 0);
      else
        INSERT_WORDS (number, 0x7ff00000, 0);
      goto SetErrno;
    }
#endif

  n = exponent >= 0 ? exponent : -exponent;
#ifndef __NO_HEX_FP__
  if (flag_hex)
    exp_tab = &twos;
  else
#endif
    exp_tab = &tens;

/* Use array of multiplicands to reduce number of inexact operations (mul/div) */
  if ((i = n) & 0xf)
   number = exponent > 0 ? number * (*exp_tab).fastPos[i] : number / (*exp_tab).fastPos[i];
 if ( n >>= 4)
  {
    for(i = 0; n > 0; i++, n >>= 1)
      if (n & 1)
	/* Right to left binary exponentiation */
        number = exponent > 0 ? number * (*exp_tab).binPos[i] : number * (*exp_tab).binNeg[i];
  }

#ifdef __IEEE754__
  EXTRACT_LLWORD (ni, number);
  if (!flag_inf && ni == HUGE_VAL)
SetErrno:
    errno = ERANGE;
#endif

#ifdef __IEEE754__
ret:
#endif /* __IEEE754__ */
  if (endptr)
    *endptr = nconv ? (char *)s : (char *)nptr;
  return number ;
}

float
strtof (const char *s, char **endptr)
{
  return (float) strtod (s, endptr);
}





