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
* 		  file : $RCSfile: fmemopen.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* Copyright (C) 2007 Eric Blake
 * Permission to use, copy, modify, and distribute this software
 * is freely granted, provided that this notice is preserved.
 */

/* Open a stream around a fixed-length string */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <low/_stdio.h>

/* Describe details of an open memstream */
typedef struct fmemcookie 
{
  void *storage;      /* storage to free on close */
  char *buf;          /* buffer start */
  size_t pos;         /* current position */
  size_t eof;         /* current file size */
  size_t max;         /* maximum file size */
  char append;        /* nonzero if appending */
  char writeonly;     /* 1 if write-only */
  char saved;         /* saved character that lived at pos before write-only NUL */
} fmemcookie;

/* 
 * Read up to non-zero N bytes into BUF from stream 
 * described by COOKIE; return number of bytes read (0 on EOF). 
*/
static int fmemreader (void *cookie, char *buf, int n)
{
  fmemcookie *c = (fmemcookie *) cookie;
  
  /* 
   * Can't read beyond current size, 
   * but EOF condition is not an error. 
  */
  if (c->pos > c->eof)
    return 0;
    
  if (n >= c->eof - c->pos)
    n = c->eof - c->pos;
    
  memcpy (buf, c->buf + c->pos, n);
  c->pos += n;
  
  return n;
}/* fmemreader */

/* 
 * Write up to non-zero N bytes of BUF into the stream 
 * described by COOKIE, returning the number of bytes written 
 * or EOF on failure.
*/
static int fmemwriter (void *cookie, const char *buf, int n)
{
  fmemcookie *c = (fmemcookie *) cookie;
  int adjust = 0; /* true if at EOF, but still need to write NUL.  */

  /* 
   * Append always seeks to eof; otherwise, if we have previously done
   * a seek beyond eof, ensure all intermediate bytes are NUL.  
  */
  if (c->append)
    c->pos = c->eof;
  else if (c->pos > c->eof)
    memset (c->buf + c->eof, '\0', c->pos - c->eof);
    
  /* Do not write beyond EOF; saving room for NUL on write-only stream. */
  if (c->pos + n > c->max - c->writeonly)
    {
      adjust = c->writeonly;
      n = c->max - c->pos;
    }
    
  /*
   * Now n is the number of bytes being modified, and adjust is 1 if
   * the last byte is NUL instead of from buf.  Write a NUL if
   * write-only; or if read-write, eof changed, and there is still
   * room.  When we are within the file contents, remember what we
   * overwrite so we can restore it if we seek elsewhere later.
  */
  if (c->pos + n > c->eof)
    {
      c->eof = c->pos + n;
      if (c->eof - adjust < c->max)
        c->saved = c->buf[c->eof - adjust] = '\0';
    }
  else if (c->writeonly)
    {
      if (n)
        {
          c->saved = c->buf[c->pos + n - adjust];
          c->buf[c->pos + n - adjust] = '\0';
        }
      else
        adjust = 0;
    }
    
  c->pos += n;
  if (n - adjust)
    memcpy (c->buf + c->pos - n, buf, n - adjust);
  else
    {
      errno = ENOSPC;
      return EOF;
    }
    
  return n;
}/* fmemwriter */

/* 
 * Seek to position POS relative to WHENCE within stream 
 * described by COOKIE; return resulting position or fail 
 * with EOF.  
*/
static _fpos_t fmemseeker (void *cookie, _fpos_t pos, int whence)
{
  fmemcookie *c = (fmemcookie *) cookie;
  long offset = (long) pos;

  if (whence == SEEK_CUR)
    offset += c->pos;
  else if (whence == SEEK_END)
    offset += c->eof;
    
  if (offset < 0)
    {
      errno = EINVAL;
      offset = -1;
    }
  else if (offset > c->max)
    {
      errno = ENOSPC;
      offset = -1;
    }
  else
    {
      if (c->writeonly && c->pos < c->eof)
        {
          c->buf[c->pos] = c->saved;
          c->saved = '\0';
        }
      c->pos = offset;
      if (c->writeonly && c->pos < c->eof)
        {
          c->saved = c->buf[c->pos];
          c->buf[c->pos] = '\0';
        }
    }
    
  return (_fpos_t) offset;
}/* fmemseeker */

/* Reclaim resources used by stream described by COOKIE. */
static int fmemcloser (void *cookie)
{
  fmemcookie *c = (fmemcookie *) cookie;
  free (c->storage);
  return 0;
}/* fmemcloser */

/* 
 * Open a memstream around buffer BUF of SIZE bytes, using MODE.
 * Return the new stream, or fail with NULL.  
*/
FILE *fmemopen (void *buf, size_t size, const char *mode)
{
  __smFILE *fp;
  fmemcookie *c;
  funcookie *fc;
  int flags;
  int dummy;

  if ((flags = __parse_mode (mode, &dummy)) == 0)
    return NULL;
    
  if (!size || !(buf || (flags & __SRW)))
    {
      errno = EINVAL;
      return NULL;
    }
    
  if ((fp = __find_fp ()) == NULL)
    return NULL;
  
  fc = (funcookie *) malloc (sizeof (funcookie) + sizeof (fmemcookie) 
    + (buf ? 0 : size));
  if (fc == NULL)
    {
      fp->_flags = 0;
      return NULL;
    }
  
  c = (fmemcookie*) (fc + 1);
  c->storage = fc;
  c->max = size;
  c->writeonly = (flags & __SWR) != 0;
  c->saved = '\0';
  
  if (!buf)
    {
      /* r+/w+/a+, and no buf: file starts empty.  */
      c->buf = (char *) (c + 1);
      c->buf[0] = '\0';
      c->pos = c->eof = 0;
      c->append = (flags & __SAPP) != 0;
    }
  else
    {
      c->buf = (char *) buf;
      switch (*mode)
        {
        case 'a':
          /* a/a+ and buf: position and size at first NUL.  */
          buf = memchr (c->buf, '\0', size);
          c->eof = c->pos = buf ? (char *) buf - c->buf : size;
          if (!buf && c->writeonly)
            /* a: guarantee a NUL within size even if no writes.  */
            c->buf[size - 1] = '\0';
          c->append = 1;
          break;
        case 'r':
          /* r/r+ and buf: read at beginning, full size available.  */
          c->pos = c->append = 0;
          c->eof = size;
          break;
        case 'w':
          /* w/w+ and buf: write at beginning, truncate to empty.  */
          c->pos = c->append = c->eof = 0;
          *c->buf = '\0';
          break;
        default:
          abort ();
        }
    }

  fp->_fileid = -1;
  fp->_flags = flags;
  fp->_cookie = fc;
  fc->cookie = c;
  fc->readfn = flags & (__SRD | __SRW) ? fmemreader : NULL;
  fc->writefn = flags & (__SWR | __SRW) ? fmemwriter : NULL;
  fc->seekfn = fmemseeker;
  fc->closefn = fmemcloser;

  return (FILE*) fp;
}/* fmemopen */

