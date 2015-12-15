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
*              file : $RCSfile: a64l.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

#include <stdlib.h>
#include <limits.h>

long a64l (const char *input)
{
  const char *ptr;
  char ch;
  int i, digit;
  unsigned long result = 0;

#if ! (defined(PREFER_SIZE_OVER_SPEED) || defined(__OPTIMIZE_SIZE__))
  /* The behavior of a64l() is unspecified if input is a null pointer. */
  if (input == NULL)
    return 0;
#endif

  ptr = input;

  /* it easiest to go from most significant digit to least so find end of input or up
     to 6 characters worth */
  for (i = 0; i < 6; ++i)
    {
      if (*ptr)
        ++ptr;
    }

  while (ptr > input)
    {
      ch = *(--ptr);

      if (ch >= 'a')
        digit = (ch - 'a') + 38;
      else if (ch >= 'A')
        digit = (ch - 'A') + 12;
      else if (ch >= '0')
        digit = (ch - '0') + 2;
      else if (ch == '/')
        digit = 1;
      else
        digit = 0;

      result = (result << 6) + digit;
    }
    
#if ! (defined(PREFER_SIZE_OVER_SPEED) || defined(__OPTIMIZE_SIZE__))
#if LONG_MAX > 2147483647
  /* for implementations where long is > 32 bits, the result must be sign-extended */
  if (result & 0x80000000)
      return (((long)-1 >> 32) << 32) + result;
#endif
#endif

  return result;
}/* a64l */
