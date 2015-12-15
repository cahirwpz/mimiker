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
*                 file : $RCSfile: _wcstoxll.c,v $ 
*               author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

#include <stdlib.h>
#include <wctype.h>
#include <errno.h>
#include <limits.h>

/* Pre calculated values to avoid div and mod */
static const unsigned long long cutoff_values [3][3] = {
 { /* uns == 1 ULONG_LONG_MAX */
   ULONG_LONG_MAX/8, 
   ULONG_LONG_MAX/10,
   ULONG_LONG_MAX/16
 },
 { /* neg == 1  -(unsigned long long) LONG_LONG_MIN */
   (- (unsigned long long) LONG_LONG_MIN)/8,
   (- (unsigned long long) LONG_LONG_MIN)/10,
   (- (unsigned long long) LONG_LONG_MIN)/16
 },
 { /* neg == 0 LONG_LONG_MAX */
   LONG_LONG_MAX/8,
   LONG_LONG_MAX/10,
   LONG_LONG_MAX/16
 },
};
 
 static const int cutlim_values [3][3] = {
 { /* uns == 1 ULONG_LONG_MAX */
   ULONG_LONG_MAX %8,
   ULONG_LONG_MAX %10,
   ULONG_LONG_MAX %16
 },
 { /* neg ==1  -(unsigned long long) LONG_LONG_MIN */
   (- (unsigned long long) LONG_LONG_MIN) %8,
   (- (unsigned long long) LONG_LONG_MIN) %10,
   (- (unsigned long long) LONG_LONG_MIN) %16
 },
 { /* neg == 0 LONG_LONG_MAX */
   LONG_LONG_MAX %8,
   LONG_LONG_MAX %10,
   LONG_LONG_MAX %16
 },
};


/* Global errno used */

unsigned long long _wcstoxll (const wchar_t *nptr, wchar_t **endptr, int base, int uns)
{
  register const wchar_t *s = nptr;
  register unsigned long long acc;
  register int c;
  register unsigned long long cutoff;
  register int neg = 0, any, cutlim, index;

  do 
    {
        c = *s++;
    } while (iswspace(c));

  if (c == L'-')
    {
      neg = 1;
      c = *s++;
    }
  else if (c == L'+')
    c = *s++;

  if ((base == 0 || base == 16) &&
       c == L'0' && (*s == L'x' || *s == L'X'))
    {
      c = s[1];
      s += 2;
      base = 16;
    }

  if (base == 0)
      base = c == L'0' ? 8 : 10;
 
  /* use this index to access constants from the arrays */
  if (base == 8)
    index = 0;
  else
    index = base >> 3;

  if (uns == 1)
   {
    cutoff = cutoff_values [0][index];
    cutlim = cutlim_values [0][index];
   }
  else
   {
    cutoff = neg ? cutoff_values [1][index] : cutoff_values [2][index];
    cutlim = neg ? cutlim_values [1][index] : cutlim_values [2][index];
   }

  for (acc = 0, any = 0;; c = *s++)
    {
      if (iswdigit(c))
        c -= L'0';
      else if (iswalpha(c))
        c -= iswupper(c) ? L'A' - 10 : L'a' - 10;
      else
        break;

      if (c >= base)
        break;

      if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
        any -= 2;
      else
        {
          any = 1;
          acc *= base;
          acc += c;
        }
    }

  if (any < 0)
    {
      acc = uns ? ULONG_LONG_MAX : neg ? LONG_LONG_MIN : LONG_LONG_MAX;
      errno = ERANGE;
    }
  else if (neg)
    acc = -acc;

  if (endptr != 0)
    *endptr = (wchar_t *) (any ? (wchar_t *)s - 1 : nptr);

  return (acc);
}/* _wcstoxll */

