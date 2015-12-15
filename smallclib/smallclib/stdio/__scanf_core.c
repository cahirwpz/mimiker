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
* 		  file : $RCSfile: __scanf_core.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <wchar.h>
#include <wctype.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <float.h>
#include <stdint.h>
#include "low/_math.h"
#include "low/_stdio.h"

#define SIZE_hh -2
#define SIZE_h  -1
#define SIZE_def 0
#define SIZE_l   1
#define SIZE_ll  2


static void store_int (void *dest, int size, int neg, uint64_t i)
{
  if (!dest) return;
  if (neg) 
   i = -(__int64_t)i;
  switch (size)
   {
     case SIZE_hh:
       *(char *)dest = (char)i;
       break;
     case SIZE_h:
       *(short *)dest = (short)i;
       break;
     case SIZE_def:
     case SIZE_l:
       *(int *)dest = (int)i;
       break;
     case SIZE_ll:
       *(__int64_t *)dest = i;
       break;
   }
}

static int readwc(int c, wchar_t **wcs, mbstate_t *st)
{
  char ch = c;
  wchar_t wc;
  switch (mbrtowc(&wc, &ch, 1, st))
   {
    case -1:
     return -1;
    case -2:
     break;
    default:
     if (*wcs)
      *(*wcs)++ = wc;
   }
  return 0;
}

int __scanf_core (ReadWriteInfo *rw, const char *fmt, va_list ap, unsigned int flags)
{
  int size, c, t, m, invert, width, category;
  const char *p, *z;
  char *s;
  wchar_t *wcs;
  mbstate_t st;
  void *dest=NULL;
  uint64_t i=0;
  int neg=0, matches=0;
  double f;
  int (*read)(ReadWriteInfo *rw);

  c = -1;
  read = rw->m_fnptr;

  for (p=fmt; *p; )
  {
    if (isspace((int)*p))
     {
      while (isspace((int)*p))
      p++;

        do 
         {
           c = read(rw);
         } while (isspace(c));
        
        continue;
     }

    if (*p != '%' || p[1]=='%')
     {
      if (*p == '%') p++;
      if (c==-1)
         c = read(rw);
    
    /*  if (c < 0)
        goto input_fail;
      if (*p++ != c)
        goto match_fail;
    */
      if (*p++ != c)
       {
         if (c < 0)
           goto input_fail;
         goto match_fail;
        }
      c = -1;
      continue;
     }

    p++;

    if (*p == '*')
     {
      dest = NULL;
      p++;
     } 
    else 
      dest = va_arg(ap, void *);

    if (!*p)
      goto fmt_fail;
    
    for (width =0; isdigit((int)*p); p++)
     {
       width = 10*width + *p - '0';
     }

    size = SIZE_def;
    category = 0;

    for (;;)
     {
       t = *p++;
       t |= 32;

       switch (t)
        {
          case 'h':
            if (*p == 'h')
             {
               p++; size = SIZE_hh; 
             }
             else 
               size = SIZE_h;
             continue;
          case 'l':
            if (*p == 'l') 
            {
              p++; size = SIZE_ll;
            }
            else
             size = SIZE_l;
            continue;
          case 'j':
            size = SIZE_ll;
            continue;
          case 'z':
          case 't':
            size = SIZE_l;
            continue;
          case 'd': case 'i': case 'o': case 'u': case 'x': case 'p':
            category = 1;
          case 'c': case 's': case '{':
            goto loop_end;
          case 'e': case 'f': case 'g':
            if (flags & FLAG_INTONLY)
             goto fmt_fail;
          case 'n':
            goto loop_end;
          default:
            goto fmt_fail;
        }
     }
loop_end:

    /* Transform ls,lc -> S,C */
    if (size==SIZE_l && (t&15)==3)
     t&=~32;

    if (c==-1 && t != 'n') 
     {
       c = read(rw);
       if (c < 0) 
        goto input_fail;
     }

    s = dest;

    switch (t)
     {
      case 'n':
       {
        int ttt = rw->m_size;
        if (c!=-1)
         ttt--;
        store_int(dest, size, 0, ttt);
        continue;   /* do not increment match count, etc! */
       }
      case 'c':
        width = width ? width : 1;
        while (c >= 0 && width) {
        if (dest) *s++ = c;
         c = read (rw);
         width--;
        }
        //c = -1;
        goto char_finish;
      case 'C':
        wcs = dest;
        st = (mbstate_t){ 0 };
        if (readwc(c, &wcs, &st) < 0)
         goto input_fail;
        c = -1;
        if (!mbsinit(&st)) goto input_fail;
        goto char_finish;
      case '{':
        s = (size == SIZE_def) ? dest : 0;
        wcs = (size == SIZE_l) ? dest : 0;

        width = width ? width : INT_MAX;
        invert = 0;
        if (*p == '^')
         {
          p++;
          invert = 1;
         }

         {
           unsigned char scanset[257];
           memset(scanset, invert, sizeof scanset);
           scanset[0] = 0;
           for (z=p; *z && (*z != ']' || z==p); z++)
            scanset[1+*z] = 1-invert;
           if (!*z) 
               goto fmt_fail;
           p=z+1;
           st = (mbstate_t){0};
           for (m=0; scanset[c+1] && width; m=1)
           {
             if (wcs)
              {
                if (readwc(c, &wcs, &st) < 0)
                  goto input_fail;
              }
             else
              {
               *s++ = c;
              }
             c = read(rw);
             width--;
           }
         }

        if (!m)
          goto match_fail;
        if (wcs) *wcs++ = 0;
        if (s) *s++ = 0;
          goto char_finish;
      default:
        while (isspace(c)) /* ignore spaces */
          c = read(rw);
        if (c < 0) 
          goto input_fail;
     }

    if (category==1)  // d, i, o, u, x, p
    {
      neg = 2;
      if (c == '-')
       neg=1;
      else 
       if (c == '+')
        neg=0;

      if (neg!=2)
       c = read(rw);  /* read beyond + or - */
      else
       neg=0;
    }

    i = m = 0;

    if (t=='p')
        t = 'x';

    if (t=='i' || t=='x')
     {
      if (c != '0')
       {
        if (t == 'i')
          t = 'd';
          goto int_start;
       }
      c = read(rw);
      if ((c|0x20) != 'x')
       {
        if (t == 'i')
         {
          t = 'o';
          /* lone 0 is valid octal */
          if ((unsigned)(c-'0') >= 8)
           goto match_fail;
         }
         goto int_start;
       }
       c = read(rw);
       t = 'x';
     }
int_start:
    switch (t)
     {
      case 'd':
      case 'u':
        width = width ? width : INT_MAX;
        for (m=0; isdigit(c) && width; m=1)
         {
          i = 10*i + c-'0';
          c = read(rw);
          width--;
         }
        goto int_finish;
      case 'o':
        for (m=0; (unsigned)(c)-'0' < 8; m=1)
         {
          i = (i<<3) + c-'0';
          c = read(rw);
         }
        goto int_finish;
      case 'p':
      case 'x':
        for (m=0; ; m=1)
         {
          if (isdigit(c))
            i = (i<<4) + c-'0';
          else
          if ((unsigned)(c|0x20)-'a' < 6)
            i = (i<<4) + (c|0x20)-'a'+10;
          else
            break;
          c = read(rw);
         }
int_finish:
        if (!m)
         goto match_fail;
         store_int(dest, size, neg, i);
         break;
      case 'e':
      case 'f':
      case 'g':
       f = 0.0;
       m = 0;
      
       for (; isdigit(c); m=1)
        {
         f = 10.0 * f + (c-'0');
         c = read(rw);
        }

       if (c=='.')
        {
         double mag = 10.0;
         for (; isdigit(c=read(rw)); mag*=10.0)
          {
           f += (c-'0')/mag;
          }
        }

      if ((c|0x20)=='e')
       {
        int ex=0, en=0;
        m = 0;

        c = read(rw);

        en = 2;
        if (c=='-')
            en=1;
        else 
        if (c=='+')
            en=0;

        if (en!=2)
            c = read(rw);   /* read beyond + or - */
        else
            en = 0;

        for (; isdigit(c); m=1)
        {
         ex = 10 * ex + (c-'0');
         c = read(rw);
        }

        {
         double dt = 10.0;

        if (en) 
            dt = 1.0/10.0;

        while (ex--)
            f *= dt;
        }  
       }
      else
      if ((c|32)=='i')   /* check for INF/NaN */
       {
        c = read(rw);
        if ((c|32)=='n')
         {
          c = read(rw);
          if ((c|32)=='f')
           {
             m = 1;
             c = -1;
             INSERT_WORDS(f,0x7ff00000,0x0);
           }
         }
       }
      else
      if ((c|32)=='n')
       {
        c = read(rw);
        if ((c|32)=='a')
         {
          c = read(rw);
          if ((c|32)=='n')
           {
            m = 1;
            c = -1;
            INSERT_WORDS(f,0x7ff80000,0x0);
           }
         }
       }

      if (!m) 
       goto match_fail;
      if (neg) 
       f *= -1.0;
      if (dest) 
       {
        switch (size)
         {
          case SIZE_def:
           *(float *)dest = (float)f;
           break;
          case SIZE_l:
           *(double *)dest = f;
           break;
         }
       }
      break;
      case 's':
       width = width ? width : INT_MAX ;
       s = dest;
       while(c >= 0 && width)
        {
         if (isspace(c))
          break;
         if (dest) *s++ = c;
          c = read(rw);
          width--;
        }
       if (dest) *s++ = 0;
        break;
      case 'S':
       wcs = dest;
       st = (mbstate_t){ 0 };
       while (!isspace(c) && c!=EOF)
        {
         if (readwc(c, &wcs, &st) < 0)
          goto input_fail;
         c = read (rw);
        }
       if (!mbsinit(&st)) goto input_fail;
       if (dest) *wcs++ = 0;
       break;
     }

char_finish:
     if (dest) 
         matches++;
    }

  goto match_success;

fmt_fail:
input_fail:
  if (!matches) 
   matches--;
match_fail:
match_success:
  if ((flags & FLAG_UNGETC) && (c != -1))
    ungetc (c, rw->m_handle);
  return matches;
}
