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
*              file : $RCSfile: l64a.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

/* l64a - convert long to radix-64 ascii string
 *~~~~~~~~~~
 * Conversion is performed on at most 32-bits of input value starting~
 * from least significant bits to the most significant bits.
 *
 * The routine splits the input value into groups of 6 bits for up to~
 * 32 bits of input.  This means that the last group may be 2 bits~
 * (bits 30 and 31).
 *~
 * Each group of 6 bits forms a value from 0-63 which is converted into~
 * a character as follows:
 *         0 = '.'
 *         1 = '/'
 *         2-11 = '0' to '9'
 *        12-37 = 'A' to 'Z'
 *        38-63 = 'a' to 'z'
 *
 * When the remaining bits are zero or all 32 bits have been translated,~
 * a nul terminator is appended to the resulting string.  An input value of~
 * 0 results in an empty string.
 */

/* same as nwelib 2.0 */

#include <stdlib.h>

static const char R64_ARRAY[] = "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

static char l64a_buf[8] = {'\0'};

char *
l64a (long value)
{
  char *ptr;
  char *result;
  int i, index;
  unsigned long tmp = (unsigned long)value & 0xffffffff;

  result = l64a_buf;
  ptr = result;

  for (i = 0; i < 6; ++i)
    {
      if (tmp == 0)
	{
	  *ptr = '\0';
	  break;
	}

      /* least significant 6 bits */
      index = tmp & 0x3f;
      *ptr++ = R64_ARRAY[index];
      tmp >>= 6;
    }

  return result;
}
