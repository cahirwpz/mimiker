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
* 		  file : $RCSfile: ___low_snprintf.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/**************************************************************************//**
@File         ___low_snprintf.c
 
@Title        Low level write used in snprintf and vsnprintf

@Author        Imagination Technologies Ltd

@


@Platform     Any

@Synopsis     int __low_snprintf(ReadWriteInfo *rw, const void *src, size_t len)

@Description  Writes src to the stream given by rw->m_handle

@DocVer       1.0 1st Release
******************************************************************************/
#include <string.h>
#include <low/_stdio.h>

int __low_snprintf (ReadWriteInfo *rw, const void *src, size_t len)
{
  int ret, maxbytes = 0;
  char *dst;
  const char *src0 = (const char *) src;

  if (len && rw->m_handle)
    {
      maxbytes = MIN ((rw->m_limit - rw->m_size)-1, len);
      ret = maxbytes;
      dst = (char*)(rw->m_handle + rw->m_size);
      while (maxbytes)
      {
        *dst++ = *src0++;
        --maxbytes;
      }
      //memcpy (rw->m_handle + rw->m_size, src, maxbytes);
      rw->m_size += ret;
    }
    
  return len;
}
