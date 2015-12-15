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
* 		  file : $RCSfile: fgets.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/*
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <stdio.h>
#include <string.h>
#include <low/_stdio.h>

/* Get character string from a file or stream */
char *fgets (char *buf, int n, FILE *afp)
{
  int ch;
  char *s;
  __smFILE *fp = (__smFILE*) afp;
  
  (void) ch;
  
#if !(defined(PREFER_SIZE_OVER_SPEED) || defined(__OPTIMIZE_SIZE__))
  size_t len;
  unsigned char *p, *t;
#endif

  /* n must be atleast two to store newline and NULL */
  if (n < 2)
    return 0;

  s = buf;

  /* leave space for NULL */
  n--;

#if defined(PREFER_SIZE_OVER_SPEED) || defined(__OPTIMIZE_SIZE__)

  do
    {
      ch = fgetc ((FILE*) fp);
      if (ch == EOF)
        {
          if (s == buf)
            return 0;
          break;
        }
      *s++ = ch;
      n--;
      
      if (ch == '\n')
        break;
    }
  while (n != 0);
  
#else

  do
    {
      /* fill buffer if it is empty */
      if ((len = fp->_rsize) <= 0)
        {
          if (__refill (fp))
            {
              /* EOF: if we haven't read anything */
              if (s == buf)
                return 0;
              break;
            }
            
          len = fp->_rsize;
        }
        
      p = fp->_cptr;

      /*
       * Scan through at most n bytes of the current buffer,
       * looking for '\n'. If found, copy up to and including
       * newline, and stop. Otherwise, copy entire chunk
       * and loop.
      */
      if (len > n)
        len = n;
        
      t = (unsigned char *) memchr (p, '\n', len);
      if (t != 0)
        {
          len = ++t - p;
          fp->_rsize -= len;
          fp->_cptr = t;
          memcpy (s, p, len);
          s[len] = 0;
          return buf;
        }
        
      fp->_rsize -= len;
      fp->_cptr += len;
      memcpy (s, p, len);
      s += len;
    }
  while ((n -= len) != 0);
  
#endif  /* optimize for size */

  *s = 0;

  return buf;
}/* fgets */

