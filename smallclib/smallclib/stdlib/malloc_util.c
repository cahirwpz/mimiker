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
*              file : $RCSfile: malloc_util.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

#include <stdlib.h>
#include <low/_malloc.h>

void *_get_sys_mem (size_t size)
{
  size_t new_size = 0;
  unsigned char *new_heap = NULL, *block = NULL, *last_block = NULL;
      
  if (size > ALLOC_SIZE)
    new_size = size * 2;
  else
    new_size = ALLOC_SIZE;

  /* get memory from system */
  new_heap = (unsigned char *) sbrk (new_size);

  if (new_heap == NULL)
    return NULL;

  block = (unsigned char *) new_heap;
  last_block = block + (new_size - ADMIN_SIZE);

  /*
   * Create two blocks, one to keep end-of-chunk information and
   * one for user allocations. The first 4bytes of each block are 
   * used to store size and flags. The last block of the chunk 
   * contains nothing but BLOCK_LAST flag. 
  */
  set_info (block) = ((new_size - (ADMIN_SIZE * 2)) | BLOCK_FREE);
  set_info (last_block) = BLOCK_LAST;    /* last block is never free */

  return new_heap;
}/* _get_sys_mem */
