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
*                 file : $RCSfile: _gpinst.h,v $ 
*               author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/* Internal header - macros to access general purpose instructions */

#define __inst_set_bits(o,i,pos,num)  __asm__ ("ins %0, %1, %2, %3" :	\
					       "+r" (o) :		\
					       "r" (i), "K" (pos), "K" (num));

#define __inst_set_hi_bits(o,i,num)  __asm__ ("ins %0, %1, %2, %3" :	\
					    "+r" (o) :			\
					    "r" (i), "K" (32-num), "K" (num));

#define __inst_set_lo_bits(o,i,num)  __asm__ ("ins %0, %1, %2, %3" :	\
					    "+r" (o) :			\
					    "r" (i), "K" (0), "K" (num));

#define __inst_count_lead_zeros(o,i)  __asm__ ("clz %0, %1" :	\
					       "=r" (o) :		\
					       "r" (i));

#define __inst_clear_bits(o,pos,num)  __asm__ ("ins %0, $0, %1, %2" :	\
					       "+r" (o) :		\
					       "K" (pos), "K" (num));

#define __inst_clear_hi_bits(o,num)  __asm__ ("ins %0, $0, %1, %2" :	\
					      "+r" (o) :		\
					      "K" (32-num), "K" (num));

#define __inst_clear_lo_bits(o,num)  __asm__ ("ins %0, $0, %1, %2" :	\
					      "+r" (o) :		\
					      "K" (0), "K" (num));

