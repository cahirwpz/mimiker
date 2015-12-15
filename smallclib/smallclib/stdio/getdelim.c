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
* 		  file : $RCSfile: getdelim.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* Copyright 2002, Red Hat Inc. - all rights reserved */

/* Read a line up to a specified line delimiter */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>

#define MIN_LINE_SIZE       4
#define DEFAULT_LINE_SIZE   128

ssize_t __getdelim (char **bufptr, size_t *n, int delim, FILE *fp)
{
  char *buf, *ptr;
  int pos, ch, cont;
  size_t newsize, numbytes;

  if (fp == NULL || bufptr == NULL || n == NULL)
    {
      errno = EINVAL;
      return -1;
    }

  buf = *bufptr;
  if (buf == NULL || *n < MIN_LINE_SIZE) 
    {
      buf = (char *) realloc (*bufptr, DEFAULT_LINE_SIZE);
      if (buf == NULL)
        return -1;
      *bufptr = buf;
      *n = DEFAULT_LINE_SIZE;
    }

  numbytes = *n;
  ptr = buf;

  cont = 1;

  while (cont)
    {
      /* fill buffer - leaving room for nul-terminator */
      while (--numbytes > 0)
        {
          if ((ch = getc_unlocked (fp)) == EOF)
            {
              cont = 0;
              break;
            }
          else 
            {
              *ptr++ = ch;
              if (ch == delim)
                {
                  cont = 0;
                  break;
                }
            }
        }

      if (cont)
        {
          /* Buffer is too small so reallocate a larger buffer.  */
          pos = ptr - buf;
          newsize = (*n << 1);
          buf = realloc (buf, newsize);
          if (buf == NULL)
            {
              cont = 0;
              break;
            }

          /* After reallocating, continue in new buffer */          
          *bufptr = buf;
          *n = newsize;
          ptr = buf + pos;
          numbytes = newsize - pos;
        }
    }

  /* if no input data, return failure */
  if (ptr == buf)
    return -1;

  /* otherwise, nul-terminate and return number of bytes read */
  *ptr = '\0';
  
  return (ssize_t)(ptr - buf);
}/* __getdelim */

