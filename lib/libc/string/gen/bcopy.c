/*	$NetBSD: bcopy.c,v 1.13 2018/02/12 11:14:15 martin Exp $	*/

/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#include <stddef.h>

#if defined(_STANDALONE) || defined(_KERNEL)
#undef bcopy
#undef memcpy
#undef memmove
#endif

/*
 * sizeof(word) MUST BE A POWER OF TWO
 * SO THAT wmask BELOW IS ALL ONES
 */
typedef long word; /* "word" used for optimal copy speed */

#define wsize sizeof(word)
#define wmask (wsize - 1)

/*
 * Copy a block of memory, handling overlap.
 * This is the routine that actually implements
 * (the portable versions of) bcopy, memcpy, and memmove.
 */
__no_sanitize
#if defined(MEMCOPY)
void *memcpy(void *dst0, const void *src0, size_t length)
#elif defined(MEMMOVE)
void *memmove(void *dst0, const void *src0, size_t length)
#else
void bcopy(const void *src0, void *dst0, size_t length)
#endif
{
  char *dst = dst0;
  const char *src = src0;
  size_t t;
  unsigned long u;

  if (length == 0 || dst == src) /* nothing to do */
    goto done;

    /*
     * Macros: loop-t-times; and loop-t-times, t>0
     */
#define TLOOP(s)                                                               \
  if (t)                                                                       \
  TLOOP1(s)
#define TLOOP1(s)                                                              \
  do {                                                                         \
    s;                                                                         \
  } while (--t)

  if ((unsigned long)dst < (unsigned long)src) {
    /*
     * Copy forward.
     */
    u = (unsigned long)src; /* only need low bits */
    if ((u | (unsigned long)dst) & wmask) {
      /*
       * Try to align operands.  This cannot be done
       * unless the low bits match.
       */
      if ((u ^ (unsigned long)dst) & wmask || length < wsize)
        t = length;
      else
        t = wsize - (size_t)(u & wmask);
      length -= t;
      TLOOP1(*dst++ = *src++);
    }
    /*
     * Copy whole words, then mop up any trailing bytes.
     */
    t = length / wsize;
    TLOOP(*(word *)(void *)dst = *(const word *)(const void *)src; src += wsize;
          dst += wsize);
    t = length & wmask;
    TLOOP(*dst++ = *src++);
  } else {
    /*
     * Copy backwards.  Otherwise essentially the same.
     * Alignment works as before, except that it takes
     * (t&wmask) bytes to align, not wsize-(t&wmask).
     */
    src += length;
    dst += length;
    u = (unsigned long)src;
    if ((u | (unsigned long)dst) & wmask) {
      if ((u ^ (unsigned long)dst) & wmask || length <= wsize)
        t = length;
      else
        t = (size_t)(u & wmask);
      length -= t;
      TLOOP1(*--dst = *--src);
    }
    t = length / wsize;
    TLOOP(src -= wsize; dst -= wsize;
          *(word *)(void *)dst = *(const word *)(const void *)src);
    t = length & wmask;
    TLOOP(*--dst = *--src);
  }
done:
#if defined(MEMCOPY) || defined(MEMMOVE)
  return (dst0);
#else
  return;
#endif
}
