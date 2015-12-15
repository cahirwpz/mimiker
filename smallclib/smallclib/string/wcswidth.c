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
*                 file : $RCSfile: wcswidth.c,v $ 
*               author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/*
FUNCTION
	<<wcswidth>>---number of column positions of a wide-character string
	
INDEX
	wcswidth

ANSI_SYNOPSIS
	#include <wchar.h>
	int wcswidth(const wchar_t *<[pwcs]>, size_t <[n]>);

TRAD_SYNOPSIS
	#include <wchar.h>
	int wcswidth(<[pwcs]>, <[n]>)
	wchar_t *<[wc]>;
	size_t <[n]>;

DESCRIPTION
	The <<wcswidth>> function shall determine the number of column
	positions required for <[n]> wide-character codes (or fewer than <[n]>
	wide-character codes if a null wide-character code is encountered
	before <[n]> wide-character codes are exhausted) in the string pointed
	to by <[pwcs]>.

RETURNS
	The <<wcswidth>> function either shall return 0 (if <[pwcs]> points to a
	null wide-character code), or return the number of column positions
	to be occupied by the wide-character string pointed to by <[pwcs]>, or
	return -1 (if any of the first <[n]> wide-character codes in the
	wide-character string pointed to by <[pwcs]> is not a printable
	wide-character code).

PORTABILITY
<<wcswidth>> has been introduced in the Single UNIX Specification Volume 2.
<<wcswidth>> has been marked as an extension in the Single UNIX Specification Volume 3.
*/

#include <low/_flavour.h>
#include <wchar.h>
#include <low/_string.h>

extern wint_t _jp2uc (wint_t c);

int
wcswidth (const wchar_t *pwcs, size_t n)
{
  int w, len = 0;

  if (!pwcs || n == 0)
    return 0;
  
  do {
       wint_t wi = *pwcs;

#ifdef _MB_CAPABLE

  wi = _jp2uc (wi);
  /* First half of a surrogate pair? */
  if (sizeof (wchar_t) == 2 && wi >= 0xd800 && wi <= 0xdbff)
    {
      wint_t wi2;

      /* Extract second half and check for validity. */
      if (--n == 0 || (wi2 = _jp2uc (*++pwcs)) < 0xdc00 || wi2 > 0xdfff)
	return -1;
      /* Compute actual unicode value to use in call to __wcwidth. */
      wi = (((wi & 0x3ff) << 10) | (wi2 & 0x3ff)) + 0x10000;
    }
#endif /* _MB_CAPABLE */
    if ((w = __wcwidth (wi)) < 0)
      return -1;
    len += w;
  } while (*pwcs++ && --n > 0);
  return len;
}
