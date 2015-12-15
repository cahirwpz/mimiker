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
* 		  file : $RCSfile: fgetws.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/*-
 * Copyright (c) 2002-2004 Tim J. Robbins.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* Get wide character string from a file or stream */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <wchar.h>
#include <low/_stdio.h>

wchar_t * fgetws (wchar_t * ws, int n, FILE *afp)
{
  wchar_t *wsp;
  size_t nconv;
  const char *src;
  unsigned char *nl;
  __smFILE *fp = (__smFILE*) afp;

  /* set orientation to wide */
  if ((fp->_flags & __SORD) == 0)
    {
      fp->_flags |= __SORD;
      fp->_flags2 |= __SWID;
    }

  if (n <= 0)
    {
      errno = EINVAL;
      goto error;
    }

  if (fp->_rsize <= 0 && __refill (fp))
    goto error;
    
  wsp = ws;
  do
    {
      src = (char *) fp->_cptr;
      nl = memchr (fp->_cptr, '\n', fp->_rsize);
      nconv = mbsnrtowcs (wsp, &src,
            /* Read all bytes up to the next NL, or up to the
               end of the buffer if there is no NL. */
                nl != NULL ? (nl - fp->_cptr + 1) : fp->_rsize,
            /* But never more than n - 1 wide chars. */
                n - 1,
                &fp->_mbstate);
                
      /* Conversion error */
      if (nconv == (size_t) -1)
        goto error;
        
      if (src == NULL)
        {
          /*
           * We hit a null byte. Increment the character count,
           * since mbsnrtowcs()'s return value doesn't include
           * the terminating null, then resume conversion
           * after the null.
           */
          nconv++;
          src = memchr (fp->_cptr, '\0', fp->_rsize);
          src++;
        }
        
      fp->_rsize -= (unsigned char *) src - fp->_cptr;
      fp->_cptr = (unsigned char *) src;
      n -= nconv;
      wsp += nconv;
    }
  while ((wsp[-1] != L'\n') && (n > 1)
    && (fp->_rsize > 0 || __refill (fp) == 0));
   
  /* EOF */   
  if (wsp == ws)
    goto error;
    
  /* Incomplete character */
  if (! mbsinit (&fp->_mbstate))
    goto error;
    
  *wsp++ = L'\0';
  return ws;

error:
  return NULL;
}/* fgetws */

