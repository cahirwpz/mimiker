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
*                 file : $RCSfile: _malloc.h,v $ 
*               author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

#ifndef __LOW_MALLOC_H_
#define __LOW_MALLOC_H_

#define ALLOC_SIZE  (1024*4)
#define ADMIN_SIZE  (16)
#define ALIGN_SIZE  (16)

#define BLOCK_FREE  1
#define BLOCK_LAST  2

#define is_free(INFO)     ((((INFO) & 3) & BLOCK_FREE) == BLOCK_FREE)
#define is_last(INFO)     ((((INFO) & 3) & BLOCK_LAST) == BLOCK_LAST)
#define get_block_size(INFO)  (((INFO) & (~3)))
#define set_info(BLOCK)       (*((unsigned int *)(BLOCK)))
#define get_info(BLOCK)       (*((unsigned int *)(BLOCK)))
#define next_block(BLOCK)     ((BLOCK) + get_block_size (get_info ((BLOCK))) + ADMIN_SIZE)

/* malloc helper functions */
void * _get_sys_mem (size_t size);
unsigned int _coalesce (unsigned char * block);
void * sbrk (size_t incr);

extern void *__smlib_heap_start;

#endif /* __LOW_MALLOC_H_ */

