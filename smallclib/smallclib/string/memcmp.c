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
*                 file : $RCSfile: memcmp.c,v $ 
*               author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

#include <string.h>
#include "low/_flavour.h"

int 
memcmp (const void *s1, const void *s2, size_t n)
{
  register int r = 0;
  register const unsigned char *r1 = (const unsigned char *) s1;
  register const unsigned char *r2 = (const unsigned char *) s2;

#ifndef  __PREFER_SIZE_OVER_SPEED__
  register const unsigned long *a1;
  register const unsigned long *a2;

  /* If both S1 and S2 are word-aligned then do word compare */
  if (! (((long)r1 & (sizeof (long) - 1)) | ((long)r2 & (sizeof (long) - 1))))
    {
      a1 = (const unsigned long *) r1;
      a2 = (const unsigned long *) r2;
      
      /* If N is more than the size of word */
      while (n >= sizeof (long))
        {
          if (*a1 != *a2) 
            break;
          a1++;
          a2++;
          n -= sizeof (long);
        }

      r1 = (const unsigned char*) a1;
      r2 = (const unsigned char*) a2;
    }
#endif /* __PREFER_SIZE_OVER_SPEED__ */
  
  /* Byte compare */
  while (n-- && ((r = ((int)(*r1++)) - *r2++) == 0));
  
  return r;
}

