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
* 		  file : $RCSfile: findfp.c,v $ 
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
#include <errno.h>
#include <low/_stdio.h>

/*
 * Find a free FILE for fopen et al.
*/
__smFILE *__find_fp (void)
{
  __smFILE *fp;
  
#ifdef __STATIC_IO_STREAMS__
  int i;

  /* find empty FILE */
  for (i = 0; i < (FOPEN_MAX - 3); i++)
    {
      fp = &__io_streams[i];

      if (fp->_fileid == 0)
        {
          __INIT_FILE_PTR (fp);
          
          fp->_base = (unsigned char *) malloc (BUFSIZ);
          
          if (fp->_base == NULL)
            {
              fp->_flags = __SNBF;  /* unbuffered */
              fp->_base = &fp->_nbuf;
              fp->_bsize = 1;
            }
          else
            {
              fp->_flags = __SMBF;
              fp->_bsize = BUFSIZ;
            }
            
          fp->_cptr = fp->_base;
          return fp;
        }
      
    }/* for FOPEN_MAX */
  
  return NULL;    /* no more streams could be opened */
  
#else
  int found = 0;
  __stream_list *tmp, *prev = NULL;
  
  for (tmp = __io_streams; tmp != NULL; tmp = tmp->next)
    {
      fp = tmp->fp;
      
      if (fp->_fileid == 0)
        {
          found = 1;
          break;
        }
      
      prev = tmp;
    }/* for all available streams */

  if (found == 0)
    {
      tmp = (__stream_list*) malloc (sizeof (__stream_list));
      if (tmp == NULL)
        return NULL;
        
      tmp->next = NULL;
      
      fp = (__smFILE*) malloc (sizeof (__smFILE));
      if (fp == NULL)
        {
          free (tmp);
          return NULL;
        }

      __INIT_FILE_PTR (fp);
      
      tmp->fp = fp;
      found = 1;
      
      if (__io_streams != NULL)
        prev->next = tmp;
      else
        __io_streams = tmp;
    }
    
  if (found == 1)
    {
      fp->_base = (unsigned char *) malloc (BUFSIZ);
      
      if (fp->_base == NULL)
        {
          fp->_flags = __SNBF;  /* unbuffered */
          fp->_base = &fp->_nbuf;
          fp->_bsize = 1;
        }
      else
        {
          fp->_flags = __SMBF;
          fp->_bsize = BUFSIZ;
        }
      fp->_cptr = fp->_base;
      return fp;
    }
  
  return NULL;  /* cannot create stream */
  
#endif  /* __STATIC_IO_STREAMS__ */

}/* __find_fp */

