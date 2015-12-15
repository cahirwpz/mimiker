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
*              file : $RCSfile: mbstowcs.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

#include <low/_flavour.h>
#include <stdlib.h>
#include <wchar.h>
#include <locale.h>
#include <low/_stdlib.h>

size_t mbstowcs (wchar_t *pwcs, const char *s, size_t n)
{
#ifndef _MB_CAPABLE
  int count = 0;

  if (n != 0) 
  {
    do 
    {
      if ((*pwcs++ = (wchar_t) *s++) == 0)
        break;
      count++;
    } while (--n != 0);
  }

  return count;
  
#else
  mbstate_t state;
  size_t ret = 0;
  char *t = (char *)s;
  int bytes;

  state.__count = 0;
  
  if (!pwcs)
    n = (size_t) 1; /* Value doesn't matter as long as it's not 0. */
    
  while (n > 0)
    {
      bytes = __common_mbtowc (pwcs, t, MB_CUR_MAX, __locale_charset (), &state);
      if (bytes < 0)
        {
          state.__count = 0;
          return -1;
        }
      else if (bytes == 0)
        break;

      t += bytes;
      ++ret;

      if (pwcs)
        {
          ++pwcs;
          --n;
        }
    }
  return ret;
#endif
}/* mbstowcs */
