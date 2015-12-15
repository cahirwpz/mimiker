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
* 		  file : $RCSfile: fseeko.c,v $ 
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
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <low/_stdio.h>

/*
 * Set file position
*/
int fseeko (FILE *afp, _off_t offset, int whence)
{
  int ret;
  _fpos_t curoff = 0;
  __smFILE *fp = (__smFILE*) afp;

  /* 
   * In case of append mode (and if we have written something) 
   * flush the buffer and seek to end 
  */
  if ((fp->_flags & __SAPP) && (fp->_flags & __SWR))
      fflush ((FILE*) fp);

  /*
   * Change any SEEK_CUR to SEEK_SET, and check `whence' argument.
   * After this, whence is either SEEK_SET or SEEK_END.
  */
  switch (whence)
    {
    case SEEK_CUR:
      /*
       * In order to seek relative to the current stream offset,
       * we have to first find the current stream offset a la
       * ftell (see ftell for details).
      */
      fflush ((FILE*) fp);   /* may adjust seek offset on append stream */
      
      if (fp->_flags & __SOFF)
        curoff = fp->_offset;
      else
        {
          curoff = low_seek (fp, 0, SEEK_CUR);
          if (curoff == -1L)
            {
              fp->_flags &= ~__SOFF;
              return EOF;
            }
          fp->_flags |= __SOFF;
          fp->_offset = curoff;           
        }
        
      if (fp->_flags & __SRD)
        {
          curoff -= fp->_rsize;
          curoff -= fp->_ursize;
        }
      else if ((fp->_flags & __SWR) && fp->_cptr != NULL)
        curoff += fp->_cptr - fp->_base;

      offset += curoff;
      whence = SEEK_SET;
      break;

    case SEEK_SET:
    case SEEK_END:
      break;

    default:
      errno = EINVAL;
      return EOF;
    }

  ret = fflush ((FILE*) fp);
  curoff = low_seek (fp, offset, whence);
  if (curoff == -1L || ret != 0)
    {
      fp->_flags &= ~__SOFF;
      return EOF;
    }
    
  fp->_flags |= __SOFF;
  fp->_offset = curoff;    
  fp->_ursize = 0;
  fp->_ucptr = 0;
  fp->_cptr = fp->_base;
  fp->_rsize = 0;
  fp->_flags &= ~__SEOF;
  
  return 0;
}/* fseeko */

