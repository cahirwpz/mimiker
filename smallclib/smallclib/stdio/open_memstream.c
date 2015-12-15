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
* 		  file : $RCSfile: open_memstream.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* Copyright (C) 2007 Eric Blake
 * Permission to use, copy, modify, and distribute this software
 * is freely granted, provided that this notice is preserved.
 */

/* Open a write stream around an arbitrary-length string */

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <low/_stdio.h>

#define OFF_T long

/* Describe details of an open memstream.  */
typedef struct memstream 
{
  void *storage;    /* storage to free on close */
  char **pbuf;      /* pointer to the current buffer */
  size_t *psize;    /* pointer to the current size, smaller of pos or eof */
  size_t pos;       /* current position */
  size_t eof;       /* current file size */
  size_t max;       /* current malloc buffer size, always > eof */
  union {
    char c;
    wchar_t w;
  } saved;          /* saved character that lived at *psize before NUL */
  int8_t wide;      /* wide-oriented (>0) or byte-oriented (<0) */
} memstream;

/* 
 * Write up to non-zero N bytes of BUF into the stream described by COOKIE,
 * returning the number of bytes written or EOF on failure.  
*/
static int memwriter (void *cookie, const char *buf, int n)
{
  memstream *c = (memstream *) cookie;
  char *cbuf = *c->pbuf;

  /* size_t is unsigned, but off_t is signed.  Don't let stream get so
     big that user cannot do ftello.  */
  if (sizeof (OFF_T) == sizeof (size_t) && (size_t) (c->pos + n) < 0)
    {
      errno = EFBIG;
      return EOF;
    }
  /* Grow the buffer, if necessary.  Choose a geometric growth factor
     to avoid quadratic realloc behavior, but use a rate less than
     (1+sqrt(5))/2 to accomodate malloc overhead.  Overallocate, so
     that we can add a trailing \0 without reallocating.  The new
     allocation should thus be max(prev_size*1.5, c->pos+n+1). */
  if (c->pos + n >= c->max)
    {
      size_t newsize = c->max * 3 / 2;
      if (newsize < c->pos + n + 1)
        newsize = c->pos + n + 1;
      cbuf = realloc (cbuf, newsize);
      if (! cbuf)
        return EOF; /* errno already set to ENOMEM */
      *c->pbuf = cbuf;
      c->max = newsize;
    }
  /* If we have previously done a seek beyond eof, ensure all
     intermediate bytes are NUL.  */
  if (c->pos > c->eof)
    memset (cbuf + c->eof, '\0', c->pos - c->eof);
  memcpy (cbuf + c->pos, buf, n);
  c->pos += n;
  /* If the user has previously written further, remember what the
     trailing NUL is overwriting.  Otherwise, extend the stream.  */
  if (c->pos > c->eof)
    c->eof = c->pos;
  else if (c->wide > 0)
    c->saved.w = *(wchar_t *)(cbuf + c->pos);
  else
    c->saved.c = cbuf[c->pos];
  cbuf[c->pos] = '\0';
  *c->psize = (c->wide > 0) ? c->pos / sizeof (wchar_t) : c->pos;
  return n;
}/* memwriter */

/* Seek to position POS relative to WHENCE within stream described by
   COOKIE; return resulting position or fail with EOF.  */
static _fpos_t memseeker (void *cookie, _fpos_t pos, int whence)
{
  memstream *c = (memstream *) cookie;
  OFF_T offset = (OFF_T) pos;

  if (whence == SEEK_CUR)
    offset += c->pos;
  else if (whence == SEEK_END)
    offset += c->eof;
  if (offset < 0)
    {
      errno = EINVAL;
      offset = -1;
    }
  else if ((size_t) offset != offset)
    {
      errno = ENOSPC;
      offset = -1;
    }
  else
    {
      if (c->pos < c->eof)
        {
          if (c->wide > 0)
            *(wchar_t *)((*c->pbuf) + c->pos) = c->saved.w;
          else
            (*c->pbuf)[c->pos] = c->saved.c;
          c->saved.w = L'\0';
        }
      c->pos = offset;
      if (c->pos < c->eof)
        {
          if (c->wide > 0)
            {
              c->saved.w = *(wchar_t *)((*c->pbuf) + c->pos);
              *(wchar_t *)((*c->pbuf) + c->pos) = L'\0';
              *c->psize = c->pos / sizeof (wchar_t);
            }
          else
            {
              c->saved.c = (*c->pbuf)[c->pos];
              (*c->pbuf)[c->pos] = '\0';
              *c->psize = c->pos;
            }
        }
      else if (c->wide > 0)
        *c->psize = c->eof / sizeof (wchar_t);
      else
        *c->psize = c->eof;
    }
  return (_fpos_t) offset;
}/* memseeker */

/* Reclaim resources used by stream described by COOKIE.  */
static int memcloser (void *cookie)
{
  memstream *c = (memstream *) cookie;
  char *buf;

  /* Be nice and try to reduce any unused memory.  */
  buf = realloc (*c->pbuf, c->wide > 0 ? (*c->psize + 1) * sizeof (wchar_t) : *c->psize + 1);
  if (buf)
    *c->pbuf = buf;
  free (c->storage);
  return 0;
}/* memcloser */

/* Open a memstream that tracks a dynamic buffer in BUF and SIZE.
   Return the new stream, or fail with NULL.  */
static FILE * internal_open_memstream (char **buf, size_t *size, int wide)
{
  __smFILE *fp;
  memstream *c;
  funcookie *fc;

  if (!buf || !size)
    {
      errno = EINVAL;
      return NULL;
    }
    
  if ((fp = __find_fp ()) == NULL)
    return NULL;
  
  fc = (funcookie*) malloc (sizeof (funcookie) + sizeof (memstream));
  
  if (fc == NULL)
    {
      fp->_flags = 0;   /* release */
      return NULL;
    }

  c = (memstream*) (fc + 1);
    
  /* Use *size as a hint for initial sizing, but bound the initial
     malloc between 64 bytes (same as asprintf, to avoid frequent
     mallocs on small strings) and 64k bytes (to avoid overusing the
     heap if *size was garbage).  */
  c->max = *size;
  if (wide == 1)
    c->max *= sizeof(wchar_t);
  if (c->max < 64)
    c->max = 64;
#if (SIZE_MAX >= 64 * 1024)
  else if (c->max > 64 * 1024)
    c->max = 64 * 1024;
#endif
  *size = 0;
  *buf = malloc (c->max);
  if (!*buf)
    {
      fp->_flags = 0; /* release */
      free (c);
      return NULL;
    }
  if (wide == 1)
    **((wchar_t **)buf) = L'\0';
  else
    **buf = '\0';

  c->storage = c;
  c->pbuf = buf;
  c->psize = size;
  c->eof = 0;
  c->saved.w = L'\0';
  c->wide = (int8_t) wide;

  fp->_fileid = -1;
  fp->_flags = __SWR;
  fp->_cookie = fc;
  
  fc->cookie = c;
  fc->readfn = NULL;
  fc->writefn = memwriter;
  fc->seekfn = memseeker;
  fc->closefn = memcloser;
  
  /* set orientation to wide */
  if (wide == 1)
    {
      fp->_flags |= __SORD;
      fp->_flags2 |= __SWID;
    }

  return (FILE*) fp;
}/* internal_open_memstream */

FILE * open_memstream (char **buf, size_t *size)
{
  return internal_open_memstream (buf, size, -1);
}

FILE * open_wmemstream (wchar_t **buf, size_t *size)
{
  return internal_open_memstream ((char **)buf, size, 1);
}

