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
*                 file : $RCSfile: memrchr.c,v $ 
*               author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/*
FUNCTION
  <<memrchr>>---reverse search for character in memory

INDEX
  memrchr

ANSI_SYNOPSIS
  #include <string.h>
  void *memrchr(const void *<[src]>, int <[c]>, size_t <[length]>);

TRAD_SYNOPSIS
  #include <string.h>
  void *memrchr(<[src]>, <[c]>, <[length]>)
  void *<[src]>;
  void *<[c]>;
  size_t <[length]>;

DESCRIPTION
  This function searches memory starting at <[length]> bytes
  beyond <<*<[src]>>> backwards for the character <[c]>.
  The search only ends with the first occurrence of <[c]>; in
  particular, <<NUL>> does not terminate the search.

RETURNS
  If the character <[c]> is found within <[length]> characters
  of <<*<[src]>>>, a pointer to the character is returned. If
  <[c]> is not found, then <<NULL>> is returned.

PORTABILITY
<<memrchr>> is a GNU extension.

<<memrchr>> requires no supporting OS subroutines.

QUICKREF
  memrchr
*/

#include <string.h>

void *
memrchr (const void *src, int c, size_t n)
{
  register const unsigned char *s = (const unsigned char *) src + n - 1;
  register unsigned char ch = c;
  
  while (n)
    {
      n--;
      if ( *s == ch ) 
        return (void *) s;
      s--;
    }
  return NULL;
}

