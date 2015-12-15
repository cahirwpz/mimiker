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
* 		  file : $RCSfile: fopencookie.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* Copyright (C) 2007 Eric Blake
 * Permission to use, copy, modify, and distribute this software
 * is freely granted, provided that this notice is preserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <low/_stdio.h>

typedef struct fccookie 
{
  void *cookie;
  __smFILE *fp;
  cookie_read_function_t *readfn;
  cookie_write_function_t *writefn;
  cookie_seek_function_t *seekfn;
  cookie_close_function_t *closefn;
} fccookie;

static int fcreader (void *cookie, char *buf, int n)
{
  int result;
  fccookie *c = (fccookie *) cookie;
  result = c->readfn (c->cookie, buf, n);
  return result;
}

static int fcwriter (void *cookie, const char *buf, int n)
{
  int result;
  fccookie *c = (fccookie *) cookie;
  
  if (c->fp->_flags & __SAPP)
      c->seekfn (cookie, 0, SEEK_END);

  result = c->writefn (c->cookie, buf, n);
  return result;
}

static _fpos_t fcseeker (void *cookie, _fpos_t pos, int whence)
{
  fccookie *c = (fccookie *) cookie;
  _off_t offset = (_off_t) pos;
  c->seekfn (c->cookie, &offset, whence);
  return (_fpos_t) offset;
}

static int fccloser (void *cookie)
{
  int result = 0;
  fccookie *c = (fccookie *) cookie;
  
  if (c->closefn)
      result = c->closefn (c->cookie);

  free (c);
  return result;
}

FILE * fopencookie (void *cookie, const char *mode, cookie_io_functions_t functions)
{
  __smFILE *fp;
  fccookie *c;
  funcookie *fc;
  int flags;
  int dummy;

  if ((flags = __parse_mode (mode, &dummy)) == 0)
    return NULL;
    
  if (((flags & (__SRD | __SRW)) && !functions.read)
      || ((flags & (__SWR | __SRW)) && !functions.write))
    {
      errno = EINVAL;
      return NULL;
    }
    
  if ((fp = __find_fp ()) == NULL)
    return NULL;

  fc = (funcookie *) malloc (sizeof (funcookie) + sizeof (fccookie));
    
  if (fc == NULL)
    {
      fp->_flags = 0;
      return NULL;
    }

  c = (fccookie*) (fc + 1);
    
  fp->_fileid = -1;
  fp->_flags = flags;
  fp->_cookie = fc;
  
  fc->cookie = c;
  fc->readfn = fcreader;
  fc->writefn = fcwriter;
  fc->seekfn = fcseeker;
  fc->closefn = fccloser;
  
  c->fp = fp;
  c->cookie = cookie;
  c->readfn = functions.read;
  c->writefn = functions.write;
  c->seekfn = functions.seek;
  c->closefn = functions.close;

  return (FILE*) fp;
}

