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
*                 file : $RCSfile: memmem.c,v $ 
*               author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* Byte-wise substring search, using the Two-Way algorithm.
 * Copyright (C) 2008 Eric Blake
 * Permission to use, copy, modify, and distribute this software
 * is freely granted, provided that this notice is preserved.
 */

/*
FUNCTION
  <<memmem>>---find memory segment

INDEX
  memmem

ANSI_SYNOPSIS
  #include <string.h>
  char *memmem(const void *<[s1]>, size_t <[l1]>, const void *<[s2]>,
         size_t <[l2]>);

DESCRIPTION

  Locates the first occurrence in the memory region pointed to
  by <[s1]> with length <[l1]> of the sequence of bytes pointed
  to by <[s2]> of length <[l2]>.  If you already know the
  lengths of your haystack and needle, <<memmem>> can be much
  faster than <<strstr>>.

RETURNS
  Returns a pointer to the located segment, or a null pointer if
  <[s2]> is not found. If <[l2]> is 0, <[s1]> is returned.

PORTABILITY
<<memmem>> is a newlib extension.

<<memmem>> requires no supporting OS subroutines.

QUICKREF
  memmem pure
*/

#include <string.h>

void *
memmem(const void *haystack, size_t haystacklen, const void *needle, size_t needlelen)
{
  register const void *haystack_r = haystack;
  register const void *needle_r   = needle;

  register size_t haystacklen_r = haystacklen;
  register size_t needlelen_r   = needlelen;

  if ( needlelen_r == 0 )
    /* The first occurrence of the empty string is deemed to occur at
       the beginning of the string.  */
    return (void *) haystack_r;

  while ( needlelen_r <= haystacklen_r )
    {
      if ( !memcmp (haystack_r, needle_r, needlelen_r) )
        return (void *) haystack_r;
      else 
        {
          haystack_r++;
          haystacklen_r--;
        }
    }
  return NULL;
}
