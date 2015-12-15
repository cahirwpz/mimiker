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
*              file : $RCSfile: malloc.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

#include <stdlib.h>
#include <low/_malloc.h>

void *__smlib_heap_start = NULL;

void *malloc (size_t size)
{
  size_t bsize = 0;
  unsigned int info = 0;
  unsigned char *block = NULL, *next_addr = NULL, *ret_addr = NULL;

  /* align the size */
  if (size & (ALIGN_SIZE - 1))
    size += ALIGN_SIZE - (size & (ALIGN_SIZE - 1));

  /* get memory from system */
  if (__smlib_heap_start == NULL)
    __smlib_heap_start = _get_sys_mem (size);
      
  if (__smlib_heap_start == NULL)
    return NULL;

  block = (unsigned char *) __smlib_heap_start;

  /* search for empty block */
  for (;;)
    {
      info = get_info (block);
          
      if (is_free (info))
        {
          bsize = get_block_size (info);
            
          if (bsize < size)
            bsize = _coalesce (block);

          if (bsize >= size)
            {
              ret_addr = block + ADMIN_SIZE;
              set_info (block) = size;  /* size of current block excuding bookkeeping */

              /* spilt this block only when blocksize is bigger than the request */
              if (bsize > size)
                {
                  next_addr = ret_addr + size;  /* next block is at this address */
                  set_info (next_addr) = (bsize - (size + ADMIN_SIZE)) 
                    | BLOCK_FREE;  /* size remaining excuding bookkeeping */
                }
              return (void*) ret_addr;
            }
          else
            goto GetNextBlock;
        }
      else if (is_last (info))
        {
          unsigned char *new_chunk;

          /* check if we have another chunk linked to this block */
          new_chunk = (unsigned char *) get_block_size (info);

          if (new_chunk == NULL)
            {
              /* ask for more space */
              new_chunk = _get_sys_mem (size);
                  
              if (new_chunk == NULL)
                return NULL;
                  
              /* 
               * Keep the address of the next chunk in the
               * last block. This forms a chain of available
               * blocks.
              */
              set_info (block) = (unsigned int) new_chunk | BLOCK_LAST;
            }

          block = new_chunk;  /* we have a chunk linked to last block */
        }
      else
        {
GetNextBlock:
          block = next_block (block);
        } 
    }/* for available heap */
     
  return NULL;
}/* malloc */

