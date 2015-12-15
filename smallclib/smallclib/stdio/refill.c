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
* 		  file : $RCSfile: refill.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <wchar.h>
#include <low/_stdio.h>

/*
 * Refill stream buffer.
 * Return EOF on eof or error, 0 otherwise.
*/
int __refill (__smFILE * fp)
{
  fp->_rsize = 0;

  if (fp->_flags & __SEOF)
    return EOF;

  /* if not already reading, have to be reading and writing */
  if ((fp->_flags & __SRD) == 0)
    {
      if ((fp->_flags & __SRW) == 0)
        {
          errno = EBADF;
          fp->_flags |= __SERR;
          return EOF;
        }
        
      /* switch to reading */
      if (fp->_flags & __SWR)
        {
          if (fflush ((FILE*) fp))
            return EOF;
          fp->_flags &= ~__SWR;
          fp->_wsize = 0;
        }
        
      fp->_flags |= __SRD;
    }
  else
    {
      /*
       * We were reading. If there is an ungetc buffer,
       * we must have been reading from that. Drop it,
       * and start reading from that buffer.
      */
      if (fp->_ursize > 0)
        {
          fp->_rsize = fp->_ursize;
          fp->_cptr = fp->_ucptr;
          fp->_ucptr = NULL;
          fp->_ursize = 0;
          return 0;
        }
    }

  if (fp->_base == NULL)
    return EOF;

  fp->_cptr = fp->_base;
  fp->_rsize = low_read (fp, (char *) fp->_base, fp->_bsize);

  if (fp->_rsize <= 0)
    {
      if (fp->_rsize == 0)
        fp->_flags |= __SEOF;
      else
        {
          fp->_rsize = 0;
          fp->_flags &= ~__SOFF;
          fp->_flags |= __SERR;
        }
      return EOF;
    }
  else
    fp->_offset += fp->_rsize;  /* current physical offset */
    
  return 0;
}/* __refill */

