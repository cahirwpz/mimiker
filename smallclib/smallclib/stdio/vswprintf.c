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
* 		  file : $RCSfile: vswprintf.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/*****************************************************************************//**
@File         vswprintf.c

@Title        write wide character formatted data to the char buffer

@Author        Imagination Technologies Ltd

@


@Platform     Any

@Synopsis     #include <stdio.h>
              int vswprintf(wchar_t *str, size_t size, const wchar_t *fmt, va_list ap)

@Description  <<vswprintf>> is like <<swprintf>>, It differ only in allowing
              its caller to pass the variable argument list as a <<va_list>> 
              object (initialized by <<va_start>>) rather than directly 
              accepting a variable number of arguments.  The caller is 
              responsible for calling <<va_end>>.

@DocVer       1.0 1st Release
********************************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <low/_stdio.h>

struct cookie 
{
  wchar_t *ws;
  size_t l;
};

static int wide_write (void *p, const char *s, int l)
{
  int i = 0;
  int save_l = l;
  struct cookie *cdata = (struct cookie *) p;

  while (cdata->l && l && (i=mbtowc(cdata->ws, (void *)s, l))>=0) 
    {
      s+=i;
      l-=i;
      cdata->l--;
      cdata->ws++;
    }
  
  *cdata->ws = 0;
  
  return i<0 ? i : save_l;
}

int vswprintf (wchar_t *s, size_t n, const wchar_t *fmt, va_list ap)
{
  int r;
  __smFILE fp;
  funcookie fc;
  unsigned char buf[256];
  struct cookie cdata = {s, n-1};  
  ReadWriteInfo rw;
  
  memset (&fp, 0, sizeof (__smFILE));
  
  fc.cookie = &cdata;
  fc.writefn = (funwrite) wide_write;
  
  fp._base = buf;
  fp._cptr = fp._base;
  fp._bsize = 256;
  fp._flags = __SRW;
  fp._cookie = &fc;
  
  rw.m_handle = (void*) &fp;
  rw.m_fnptr = __low_wprintf;
  rw.m_size = 0;
  
  r = _wformat_parser (&rw, fmt, &ap);

  fflush ((FILE*)&fp);
  
  return r>=n ? -1 : r;
}


