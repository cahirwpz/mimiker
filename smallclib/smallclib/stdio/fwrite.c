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
* 		  file : $RCSfile: fwrite.c,v $ 
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
#include <unistd.h>
#include <errno.h>
#include <low/_stdio.h>


/*
 * Fill partially full buffer or write directly.
 * Return 1 on error otherwise 0.
*/
static int __write_buf (__smFILE *fp, char *buf, int len, int *aw)
{
  int w = *aw;
  
  /* if buffer is partially full then fill it */
  if ((fp->_cptr > fp->_base) > 0 && len > w)
    {
      memmove (fp->_cptr, buf, w);
      fp->_cptr += w;
      if (fflush ((FILE*) fp))
        return 1;
    }
  else if (len >= fp->_bsize)
    {
      /* 
       * Write directly if buffer is empty (fp->_cptr == fp->_base) 
       * and input-buf is more than the BUFSIZ. 
      */
      w = low_write (fp, buf, fp->_bsize);
      if (w <= 0)
        return 1;
      *aw = w;
    }
  else
    {
      /* Fill the buffer */
      memmove (fp->_cptr, buf, len);
      fp->_wsize -= len;
      fp->_cptr += len;
      *aw = len;
    }
    
  return 0;
}/* __write_buf */

/*
 * Write array elements to a file
*/
size_t fwrite (const void * abuf, size_t size, size_t count, FILE * afp)
{
  int total, len, w;
  char *buf = (char*) abuf;
  __smFILE *fp = (__smFILE*) afp;

  total = len = count * size;
  
  /* nothing to write */
  if (len == 0)
    return 0;
    
  /* if stream is not open for writing */
  if (canwrite (fp))
    return EOF;
    
  if (fp->_base == NULL && (fp->_flags & __SMBF))
    {
      fp->_flags |= __SERR;
      return EOF;
    }   

  /* if unbuffered */
  if (fp->_flags & __SNBF)
    {
      do
        {
          w = low_write (fp, buf, MIN (len, BUFSIZ));
          if (w <= 0)
            goto err;
          buf += w;
        }
      while ((len -= w) != 0);
    }/* unbuffered */
  else if ((fp->_flags & __SLBF) == 0)  /* fully buffered */
    {
      do 
        {
          w = fp->_wsize;
          
          if (__write_buf (fp, buf, len, &w))
            goto err;
          
          buf += w;
          len -= w;
        } while (len != 0);
    
    }/* fully buffered */
  else
    {
      char *nl;
      int nldist = 0, nlknown = 0;
      
      do 
      {
        nl = memchr (buf, '\n', len);
        if (nl)
          {
            nldist = nl + 1 - buf;
            nlknown = 1;
          }
        else
          nldist = len;
          
          w = fp->_wsize;
          
          if (__write_buf (fp, buf, nldist, &w))
            goto err;
            
          /* if we have found newline in the buffer */
          if (nlknown != 0)
            {
              if (fflush ((FILE*) fp))
                goto err;
              nlknown = 0;
            }
            
          buf += w;
          len -= w;          
      } while (len != 0);
      
    }/* line buffered */
    
  return count;
  
err:
  fp->_flags |= __SERR;
  return (total - len) / size;
  
}/* fwrite */


