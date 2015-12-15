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

#include <stdio.h>
#include <stdlib.h>
#include <low/_malloc.h>

void *__smlib_heap_start = NULL;
static unsigned int total_mem_size = 0;

/* Start of heap */
extern char _end[];

/* Size of the memory has been defined .ld file */
extern char __memory_size[] __attribute__((weak));

void *malloc (size_t size)
{
  size_t bsize = 0;
  unsigned int info = 0;
  unsigned char *block = NULL, *next_addr = NULL, *ret_addr = NULL;

  /* 
   * In TINY mode, acquire entire heap so that there
   * won't be any calls to sbrk().
  */
  if (__smlib_heap_start == NULL)
    {
      unsigned char *last_block = NULL;
      
      __smlib_heap_start = (void *) _end;
      
      if (__smlib_heap_start == NULL)
        return NULL;
      
      total_mem_size = (unsigned int) __memory_size;
      
      /* 
       * Assume 1M memory is available if __memory_size 
       * is not defined in the .ld file 
      */
      if (total_mem_size == 0)
        total_mem_size = 0x100000U;
      
      block = (unsigned char *) __smlib_heap_start;
      last_block = block + (total_mem_size - ADMIN_SIZE);

      /*
       * Create two blocks, one to keep end-of-chunk information and
       * one for user allocations. The first 4bytes of each block are 
       * used to store size and flags. The last block of the chunk 
       * contains nothing but BLOCK_LAST flag. 
      */
      set_info (block) = ((total_mem_size - (ADMIN_SIZE * 2)) | BLOCK_FREE);
      set_info (last_block) = BLOCK_LAST;    /* last block is never free */      
    }
      
  /* align the size */
  if (size & (ALIGN_SIZE - 1))
    size += ALIGN_SIZE - (size & (ALIGN_SIZE - 1));
    
  block = (unsigned char *) __smlib_heap_start;

  /* search for empty block */
  for (;;)
    {
      info = get_info (block);
          
      if (is_free (info))
        {
          bsize = get_block_size (info);

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
        }
      else if (is_last (info))
        return NULL;

      block = next_block (block);
    }/* for available heap */
     
  return NULL;
}/* malloc */

