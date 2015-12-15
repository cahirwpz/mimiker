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
* 		  file : $RCSfile: setvbuf.c,v $ 
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
#include <low/_stdio.h>

/* Specify file or stream buffering */
int setvbuf (FILE *afp, char *buf, int mode, size_t size)
{
  int ret = 0;
  __smFILE *fp = (__smFILE*) afp;

  /* mode must be one of the full/line/no buffer */
  if ((mode != _IOFBF && mode != _IOLBF 
    && mode != _IONBF) || ((int) size < 0))
      return EOF;

  /* flush current buffer */
  fflush ((FILE*) fp);
  fp->_rsize = 0;
  
  if (fp->_flags & __SMBF)
    free (fp->_base);
    
  fp->_flags &= ~(__SLBF | __SNBF | __SMBF);

  if (mode == _IONBF)
    goto no_buffer;

  /* allocate buffer if needed */
  if (buf == NULL)
    {
      if (size == 0) 
        size = BUFSIZ;
        
      if ((buf = malloc (size)) == NULL)
          ret = EOF;

      if (buf == NULL)
        {
          /* make it un-buffered */
no_buffer:
          fp->_flags |= __SNBF;
          fp->_wsize = 0;
          fp->_base = fp->_cptr = &fp->_nbuf;
          fp->_bsize = 1;
          return ret;
        }
        
      fp->_flags |= __SMBF;
    }
    
  switch (mode)
    {
    case _IOLBF:
      fp->_flags |= __SLBF;
      /* no break */

    case _IOFBF:
      fp->_base = fp->_cptr = (unsigned char *) buf;
      fp->_bsize = size;
      break;
    }

  if (fp->_flags & __SWR)
    fp->_wsize = fp->_flags & __SNBF ? 0 : size;

  return 0;
}/* setvbuf */

