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
*                 file : $RCSfile: memmove.c,v $ 
*               author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

#include <string.h>
#include "low/_flavour.h"

void *
memmove (void *dst_void, const void* src_void, size_t length)
{
  char *dst = (char *)dst_void;
  const  char *src = (const char *)src_void;

  if (src >= dst)
    {

#ifndef __PREFER_SIZE_OVER_SPEED__
    long *aligned_dst;
    const long *aligned_src;
    int blocksize = sizeof (long) << 1;
        
    /* Use optimizing algorithm for a non-destructive copy to closely 
    match memcpy. If the size is small or either SRC or DST is unaligned,
    then punt into the byte copy loop.  This should be rare.  */
    if (length >= blocksize  && 
        !(((long)src & (sizeof (long) - 1)) |
          ((long)dst & (sizeof (long) - 1))))
      {
        aligned_dst = (long*)dst;
        aligned_src = (long*)src;

        /* Copy 4X long words at a time if possible.  */
        while (length >= blocksize)
          {
            *aligned_dst++ = *aligned_src++;
            *aligned_dst++ = *aligned_src++;
            length -= blocksize;
          }

        /* Pick up any residual with a byte copier.  */
        dst = (char*)aligned_dst;
        src = (char*)aligned_src;
      }

#endif /* __PREFER_SIZE_OVER_SPEED__ */  

      while (length)
        {
          *dst++ = *src++;
	  --length;
        }
    }
    
/* Destructive overlap, copy backwards */
  else
    {
      while (length)	    
      {
        --length;
	dst[length] = src[length];	      
      }
      	    
    }	    

  return dst_void;
}
