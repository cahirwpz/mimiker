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
* 		  file : $RCSfile: freopen.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/*
 * Copyright (c) 1990, 2007 The Regents of the University of California.
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
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <low/_stdio.h>

/*
 * Open a file using an existing file descriptor
*/
FILE *freopen (const char *file, const char *mode, FILE *afp)
{
  int f, flags, flags2, oflags, e = 0;
  __smFILE *fp = (__smFILE*) afp;

  if ((flags = __parse_mode (mode, &oflags)) == 0)
    {
      fclose ((FILE*) fp);
      return NULL;
    }

  flags2 = fp->_flags;
  
  if (fp->_flags == 0)
    fp->_flags = __SEOF;  /* hold on to it */
  else
    {
      if (fp->_flags & __SWR)
        fflush ((FILE*) fp);
      if (file != NULL)
        fclose ((FILE*) fp);
    }

  /*
   * Now get a new descriptor to refer to the new file, or reuse the
   * existing file descriptor if file is NULL.
  */
  if (file != NULL)
    {
      f = open ((char *) file, oflags, 0666);
      e = errno;
    }
  else
    {
      f = -1;
      e = EBADF;
      fclose ((FILE*) fp);
    }

  /*
   * Finish closing fp. Even if the open succeeded above,
   * we cannot keep fp->_base: it may be the wrong size.
   * This loses the effect of any setbuffer calls,
   * but stdio has always done this before.
  */
  if (flags2 & __SMBF)
    free (fp->_base);
  
  __INIT_FILE_PTR (fp);

  /* did not get it after all */
  if (f < 0)
    {
      fp->_flags = 0;   /* set it free */
      errno = e;        /* restore in case _close clobbered */
      return NULL;
    }

  fp->_flags = flags;
  fp->_fileid = f;
  fp->_cptr = fp->_base;

  return (FILE*) fp;
}/* freopen */

