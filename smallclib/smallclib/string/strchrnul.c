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
*                 file : $RCSfile: strchrnul.c,v $ 
*               author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/*
FUNCTION
  <<strchrnul>>---search for character in string

INDEX
  strchrnul

ANSI_SYNOPSIS
  #include <string.h>
  char * strchrnul(const char *<[string]>, int <[c]>);

TRAD_SYNOPSIS
  #include <string.h>
  char * strchrnul(<[string]>, <[c]>);
  const char *<[string]>;
  int <[c]>;

DESCRIPTION
  This function finds the first occurence of <[c]> (converted to
  a char) in the string pointed to by <[string]> (including the
  terminating null character).

RETURNS
  Returns a pointer to the located character, or a pointer
  to the concluding null byte if <[c]> does not occur in <[string]>.

PORTABILITY
<<strchrnul>> is a GNU extension.

<<strchrnul>> requires no supporting OS subroutines.  It uses
strchr() and strlen() from elsewhere in this library.

QUICKREF
  strchrnul
*/


/* same as newlib */
#include <string.h>

char *
strchrnul(const char *s1, int c)
{
  char *s = strchr(s1, c);

  return s ? s : (char *)s1 + strlen(s1);
}
