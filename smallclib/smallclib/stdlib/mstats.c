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
*              file : $RCSfile: mstats.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <low/_malloc.h>

void mstats (char *s)
{
  int ch_idx = 1, bk_idx = 1;
  size_t bsize = 0;
  unsigned int info;
  unsigned char *block = NULL;
  
  fprintf (stderr, "Memory allocation statistics %s\n", s);
  
  block = (unsigned char *) __smlib_heap_start;
  
  if (block == NULL)
    return;

  fprintf (stderr, "Chunk %d @%p\n", ch_idx++, block);
  
  for (;;)
    {
      info = get_info (block);
      bsize = get_block_size (info);

      if (is_free (info))
        {
          fprintf (stderr, "\tBlock %d @%p size %zd free\n", bk_idx++, block, bsize);
          goto GetNextBlock;
        }
      else if (is_last (info))
        {
          fprintf (stderr, "\tBlock %d @%p size %zd end block\n", bk_idx++, block, bsize);
          
          if (bsize == 0)
            return;
          
          block = (unsigned char*) bsize;  /* we have a chunk linked to last block */
          fprintf (stderr, "Chunk %d @%p\n", ch_idx++, block);
          bk_idx = 1;
        }
      else
        {
          /* this block is allocated */
          fprintf (stderr, "\tBlock %d @%p size %zd allocated\n", bk_idx++, block, bsize);
          
GetNextBlock:
          block = next_block (block);
        } 
    }/* for available heap */

  return;
}/* mstats */

