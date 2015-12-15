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
*                 file : $RCSfile: wcstok.c,v $ 
*               author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

#include <wchar.h>

wchar_t *
wcstok (register wchar_t *s, register const wchar_t *delim, wchar_t **lasts)
{
  register const wchar_t *spanp;
  register int c, sc;
  wchar_t *tok;

  if (s == NULL && (s = *lasts) == NULL)
	return (NULL);

  /*
   * Skip (span) leading delimiters (s += wcsspn(s, delim), sort of).
   */
cont:
  c = *s++;

  for (spanp = delim; (sc = *spanp++) != L'\0';)
    {
      if (c == sc)  goto cont;
    }

  if (c == L'\0')
    {		/* no non-delimiter characters */
      *lasts = NULL;
      return (NULL);
    }

  tok = s - 1;

  /*
   * Scan token (scan for delimiters: s += wcscspn(s, delim), sort of).
   * Note that delim must have one NUL; we stop if we see that, too.
   */
  for (;;)
    {
      c = *s++;
      spanp = delim;

      do
        {
	  if ((sc = *spanp++) == c)
	    {
              if (c == L'\0')
	        s = NULL;
              else
	        s[-1] = L'\0';

              *lasts = s;
	      return (tok);
	    }
	} while (sc != L'\0');
    }
/* NO RETURN */
}


