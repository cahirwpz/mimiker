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
*                 file : $RCSfile: qsort.c,v $ 
*               author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/


#include <stdlib.h>

static void swap(char *p, char *q, unsigned int width);

void qsort(void *base, size_t num, size_t width, int (*comp)(const void *, const void *))
{
  char *lo, *hi;
  char *mid;
  char *l, *h;
  unsigned size;
  char *lostk[30], *histk[30];
  int stkptr;

  if (num < 2 || width == 0) return;
  stkptr = 0;

  lo = base;
  hi = (char *) base + width * (num - 1);

recurse:
    size = (hi - lo) / width + 1;

    mid = lo + (size / 2) * width;
    swap(mid, lo, width);

    l = lo;
    h = hi + width;

    for (;;) {
      do { l += width; } while (l <= hi && comp(l, lo) <= 0);
      do { h -= width; } while (h > lo && comp(h, lo) >= 0);
      if (h < l) break;
      swap(l, h, width);
    }

    swap(lo, h, width);

    if (h - 1 - lo >= hi - l) {
      if (lo + width < h) {
        lostk[stkptr] = lo;
        histk[stkptr] = h - width;
        ++stkptr;
      }

      if (l < hi) {
        lo = l;
        goto recurse;
      }
    } else {
      if (l < hi) {
        lostk[stkptr] = l;
        histk[stkptr] = hi;
        ++stkptr;
      }

      if (lo + width < h) {
        hi = h - width;
        goto recurse;
      }
    }

  --stkptr;
  if (stkptr >= 0) {
    lo = lostk[stkptr];
    hi = histk[stkptr];
    goto recurse;
  }
}

static void swap(char *a, char *b, unsigned width) {
  char tmp;

  if (a != b) {
    while (width--) {
      tmp = *a;
      *a++ = *b;
      *b++ = tmp;
    }
  }
}
