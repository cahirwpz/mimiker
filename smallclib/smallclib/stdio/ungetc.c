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
* 		  file : $RCSfile: ungetc.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/**************************************************************************//**
@File         ungetc.c

@Title        Push character back into a stream

@Author        Imagination Technologies Ltd

@


@Platform     Any

@Synopsis     #include <stdio.h>
              int ungetc(int c, FILE *afp);

@Description  The ungetc is used to return bytes back to AFP to be read again.
              If C is EOF, the stream is unchanged. Otherwise, the unsigned
              char C is put back on the stream.

@DocVer       1.0 1st Release
******************************************************************************/

#include <stdio.h>
#include <low/_stdio.h>

int ungetc (int c, FILE *afp)
{
  register __smFILE *fp = (__smFILE*) afp;
  
  if (c == EOF)
    return EOF;

  if (canread (fp))
    return EOF;
    
  /* 
   * As per the ISO standard, one character of pushback is guaranteed.
   * If the ungetc function is called too many times on the same stream 
   * without an intervening read or file positioning operation on that
   * stream, the operation may fail. In SmallLib the size of unget buffer
   * is just 2 bytes.
  */
  fp = (__smFILE*) afp;
  
  /* if we already setup unget buffer */
  if (fp->_ucptr != NULL)
    {
      /* unget buffer should not overflow */
      if (fp->_rsize >= MAX_UBUF)
        return EOF;
      *--fp->_cptr = c;
      fp->_rsize++;
      return c;
    }
  
  /* keep the unget chars in reverse order */
  fp->_ursize = fp->_rsize;
  fp->_ucptr = fp->_cptr;
  fp->_ungetbuf[MAX_UBUF - 1] = c;
  fp->_cptr = &fp->_ungetbuf[MAX_UBUF - 1];
  fp->_rsize = 1;
  fp->_flags &= ~__SEOF;    /* clear the EOF flag as we have one char in the buffer */
  
  return c;
}/* ungetc */

