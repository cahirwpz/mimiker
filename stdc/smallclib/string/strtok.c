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
*                 file : $RCSfile: strtok.c,v $ 
*               author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

#include <stdio.h>
#include "low/_flavour.h"

char *
strtok_r (char *s, const char *delim, char **lasts)
{
  register char *spanp;
  register int c, sc;
  char *tok;

  if (s == NULL)
    s = *lasts;

  if (s == NULL)
    return NULL;

  /* Skip leading delimiters */
cont:
  c = *s++;
  for (spanp = (char *)delim; (sc = *spanp++) != 0;)
    {
      if (c == sc)
        goto cont;
    }

  /* No non-delimiter characters */
  if (c == 0)
    {
      *lasts = NULL;
      return NULL;
    }

  tok = s - 1;

  /*
   * Scan token. Note that delim must have one NUL;
   * we stop if we see that, too.
  */
  for (;;)
    {
      c = *s++;
      spanp = (char *)delim;
      do
        {
          if ((sc = *spanp++) == c)
            {
              if (c == 0)
                s = NULL;
              else
                s[-1] = 0;
              *lasts = s;
              return (tok);
            }
        } while (sc != 0);
    }
  /* No Return */
}

static char *end_ptr = NULL;

char *
strtok (char *s1, const char *s2)
{
  return strtok_r (s1, s2, &end_ptr);
}
