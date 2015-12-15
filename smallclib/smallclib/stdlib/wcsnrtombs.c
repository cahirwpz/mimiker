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
*                 file : $RCSfile: wcsnrtombs.c,v $ 
*               author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/*
FUNCTION
<<wcsrtombs>>, <<wcsnrtombs>>---convert a wide-character string to a character string

INDEX
	wcsrtombs
INDEX
	_wcsrtombs_r
INDEX
	wcsnrtombs
INDEX
	_wcsnrtombs_r

ANSI_SYNOPSIS
	#include <wchar.h>
	size_t wcsrtombs(char *<[dst]>, const wchar_t **<[src]>, size_t <[len]>,
			 mbstate_t *<[ps]>);

	#include <wchar.h>
	size_t _wcsrtombs_r(struct _reent *<[ptr]>, char *<[dst]>,
			    const wchar_t **<[src]>, size_t <[len]>,
			    mbstate_t *<[ps]>);

	#include <wchar.h>
	size_t wcsnrtombs(char *<[dst]>, const wchar_t **<[src]>,
			  size_t <[nwc]>, size_t <[len]>, mbstate_t *<[ps]>);

	#include <wchar.h>
	size_t _wcsnrtombs_r(struct _reent *<[ptr]>, char *<[dst]>,
			     const wchar_t **<[src]>, size_t <[nwc]>,
			     size_t <[len]>, mbstate_t *<[ps]>);

TRAD_SYNOPSIS
	#include <wchar.h>
	size_t wcsrtombs(<[dst]>, <[src]>, <[len]>, <[ps]>)
	char *<[dst]>;
	const wchar_t **<[src]>;
	size_t <[len]>;
	mbstate_t *<[ps]>;

	#include <wchar.h>
	size_t _wcsrtombs_r(<[ptr]>, <[dst]>, <[src]>, <[len]>, <[ps]>)
	struct _rent *<[ptr]>;
	char *<[dst]>;
	const wchar_t **<[src]>;
	size_t <[len]>;
	mbstate_t *<[ps]>;

	#include <wchar.h>
	size_t wcsnrtombs(<[dst]>, <[src]>, <[nwc]>, <[len]>, <[ps]>)
	char *<[dst]>;
	const wchar_t **<[src]>;
	size_t <[nwc]>;
	size_t <[len]>;
	mbstate_t *<[ps]>;

	#include <wchar.h>
	size_t _wcsnrtombs_r(<[ptr]>, <[dst]>, <[src]>, <[nwc]>, <[len]>, <[ps]>)
	struct _rent *<[ptr]>;
	char *<[dst]>;
	const wchar_t **<[src]>;
	size_t <[nwc]>;
	size_t <[len]>;
	mbstate_t *<[ps]>;

DESCRIPTION
The <<wcsrtombs>> function converts a string of wide characters indirectly
pointed to by <[src]> to a corresponding multibyte character string stored in
the array pointed to by <[dst}>.  No more than <[len]> bytes are written to
<[dst}>.

If <[dst}> is NULL, no characters are stored.

If <[dst}> is not NULL, the pointer pointed to by <[src]> is updated to point
to the character after the one that conversion stopped at.  If conversion
stops because a null character is encountered, *<[src]> is set to NULL.

The mbstate_t argument, <[ps]>, is used to keep track of the shift state.  If
it is NULL, <<wcsrtombs>> uses an internal, static mbstate_t object, which
is initialized to the initial conversion state at program startup.

The <<wcsnrtombs>> function behaves identically to <<wcsrtombs>>, except that
conversion stops after reading at most <[nwc]> characters from the buffer
pointed to by <[src]>.

RETURNS
The <<wcsrtombs>> and <<wcsnrtombs>> functions return the number of bytes
stored in the array pointed to by <[dst]> (not including any terminating
null), if successful, otherwise it returns (size_t)-1.

PORTABILITY
<<wcsrtombs>> is defined by C99 standard.
<<wcsnrtombs>> is defined by the POSIX.1-2008 standard.
*/

#include <wchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "low/_stdlib.h"

static mbstate_t wcsrtombs_state = {0, {0}};

size_t
wcsnrtombs (char *dst, const wchar_t **src, size_t nwc, size_t len, mbstate_t *ps)
{
  char *ptr = dst;
  char buff[10];
  wchar_t *pwcs;
  size_t n;
  int i;

  if (ps == NULL)
      ps = &wcsrtombs_state;

  /* If no dst pointer, treat len as maximum possible value. */
  if (dst == NULL)
    len = (size_t)-1;

  n = 0;
  pwcs = (wchar_t *)(*src);

  while (n < len && nwc-- > 0)
    {
      int count = ps->__count;
      wint_t wch = ps->__value.__wch;
      int bytes = __common_wctomb (buff, *pwcs, __locale_charset (), ps);
      if (bytes == -1)
	{
	  errno = EILSEQ;
	  ps->__count = 0;
	  return (size_t)-1;
	}
      if (n + bytes <= len)
	{
          n += bytes;
	  if (dst)
	    {
	      for (i = 0; i < bytes; ++i)
	        *ptr++ = buff[i];
	      ++(*src);
	    }
	  if (*pwcs++ == 0x00)
	    {
	      if (dst)
	        *src = NULL;
	      ps->__count = 0;
	      return n - 1;
	    }
	}
      else
	{
	  /* not enough room, we must back up state to before __wctomb call */
	  ps->__count = count;
	  ps->__value.__wch = wch;
          len = 0;
	}
    }

  return n;
} 
