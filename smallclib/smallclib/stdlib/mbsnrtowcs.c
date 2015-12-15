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
*              file : $RCSfile: mbsnrtowcs.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

/*
FUNCTION
<<mbsrtowcs>>, <<mbsnrtowcs>>---convert a character string to a wide-character string

INDEX
	mbsrtowcs
INDEX
	_mbsrtowcs_r
INDEX
	mbsnrtowcs
INDEX
	_mbsnrtowcs_r

ANSI_SYNOPSIS
	#include <wchar.h>
	size_t mbsrtowcs(wchar_t *<[dst]>, const char **<[src]>, size_t <[len]>,
			 mbstate_t *<[ps]>);

	#include <wchar.h>
	size_t _mbsrtowcs_r(struct _reent *<[ptr]>, wchar_t *<[dst]>,
			    const char **<[src]>, size_t <[len]>,
			    mbstate_t *<[ps]>);

	#include <wchar.h>
	size_t mbsnrtowcs(wchar_t *<[dst]>, const char **<[src]>,
			  size_t <[nms]>, size_t <[len]>, mbstate_t *<[ps]>);

	#include <wchar.h>
	size_t _mbsnrtowcs_r(struct _reent *<[ptr]>, wchar_t *<[dst]>,
			     const char **<[src]>, size_t <[nms]>,
			     size_t <[len]>, mbstate_t *<[ps]>);

TRAD_SYNOPSIS
	#include <wchar.h>
	size_t mbsrtowcs(<[dst]>, <[src]>, <[len]>, <[ps]>)
	wchar_t *<[dst]>;
	const char **<[src]>;
	size_t <[len]>;
	mbstate_t *<[ps]>;

	#include <wchar.h>
	size_t _mbsrtowcs_r(<[ptr]>, <[dst]>, <[src]>, <[len]>, <[ps]>)
	struct _reent *<[ptr]>;
	wchar_t *<[dst]>;
	const char **<[src]>;
	size_t <[len]>;
	mbstate_t *<[ps]>;

	#include <wchar.h>
	size_t mbsnrtowcs(<[dst]>, <[src]>, <[nms]>, <[len]>, <[ps]>)
	wchar_t *<[dst]>;
	const char **<[src]>;
	size_t <[nms]>;
	size_t <[len]>;
	mbstate_t *<[ps]>;

	#include <wchar.h>
	size_t _mbsnrtowcs_r(<[ptr]>, <[dst]>, <[src]>, <[nms]>, <[len]>, <[ps]>)
	struct _reent *<[ptr]>;
	wchar_t *<[dst]>;
	const char **<[src]>;
	size_t <[nms]>;
	size_t <[len]>;
	mbstate_t *<[ps]>;

DESCRIPTION
The <<mbsrtowcs>> function converts a sequence of multibyte characters
pointed to indirectly by <[src]> into a sequence of corresponding wide
characters and stores at most <[len]> of them in the wchar_t array pointed
to by <[dst]>, until it encounters a terminating null character ('\0').

If <[dst]> is NULL, no characters are stored.

If <[dst]> is not NULL, the pointer pointed to by <[src]> is updated to point
to the character after the one that conversion stopped at.  If conversion
stops because a null character is encountered, *<[src]> is set to NULL.

The mbstate_t argument, <[ps]>, is used to keep track of the shift state.  If
it is NULL, <<mbsrtowcs>> uses an internal, static mbstate_t object, which
is initialized to the initial conversion state at program startup.

The <<mbsnrtowcs>> function behaves identically to <<mbsrtowcs>>, except that
conversion stops after reading at most <[nms]> bytes from the buffer pointed
to by <[src]>.

RETURNS
The <<mbsrtowcs>> and <<mbsnrtowcs>> functions return the number of wide
characters stored in the array pointed to by <[dst]> if successful, otherwise
it returns (size_t)-1.

PORTABILITY
<<mbsrtowcs>> is defined by the C99 standard.
<<mbsnrtowcs>> is defined by the POSIX.1-2008 standard.
*/

#include <wchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

static mbstate_t mbsrtowcs_state = {0, {0}};

size_t
mbsnrtowcs (wchar_t *dst, const char **src, size_t nms, size_t len, mbstate_t *ps)
{
  wchar_t *ptr = dst;
  size_t max;
  size_t count = 0;
  int bytes;

  if (ps == NULL)
      ps = &mbsrtowcs_state;

  if (dst == NULL)
     return (size_t)-1;

  max = len;
  while (len > 0)
    {
      bytes = mbrtowc (ptr, *src, nms, ps);
      if (bytes > 0)
	{
	  *src += bytes;
	  nms -= bytes;
	  ++count;
	  ptr = (dst == NULL) ? NULL : ptr + 1;
	  --len;
	}
      else if (bytes == -2)
	{
	  *src += nms;
	  return count;
	}
      else if (bytes == 0)
	{
	  *src = NULL;
	  return count;
	}
      else
	{
	  ps->__count = 0;
	  errno = EILSEQ;
	  return (size_t)-1;
	}
    }

  return (size_t)max;
}
