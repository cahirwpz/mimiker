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
* 		  file : $RCSfile: __format_parser_int.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>
#include "low/_stdio.h"

typedef long long INT64;
typedef unsigned long long UINT64;

#define INTMAX_MAX  (0x7fffffffffffffffLL)

/* Some useful macros */

#define MAX(a,b) ((a)>(b) ? (a) : (b))
#define CONCAT2(x,y) x ## y
#define CONCAT(x,y) CONCAT2(x,y)

/* Convenient bit representation for modifier flags, which all fall
 * within 31 codepoints of the space character. */

#define PAD_POS    0x00000001	/* ' ' */
#define ALT_FORM   0x00000008	/* '#' */
#define GROUPED    0x00000080	/* '\' */
#define MARK_POS   0x00000800	/* '+' */
#define LEFT_ADJ   0x00002000	/* '-' */
#define ZERO_PAD   0x00010000	/* '0' */

#define FLAGMASK (ALT_FORM|ZERO_PAD|LEFT_ADJ|PAD_POS|MARK_POS|GROUPED)

#define LONG INT
#define ULONG UINT
#define LLONG ULLONG
#define SIZET ULONG
#define IMAX LLONG
#define UMAX ULLONG
#define PDIFF LONG
#define UIPTR ULONG


/* State machine to accept length modifiers + conversion specifiers.
 * Result is 0 on failure, or an argument type to pop on success. */

enum {
  BARE, LPRE, LLPRE, HPRE, HHPRE, BIGLPRE,
  ZTPRE, JPRE,
  STOP,
  PTR, INT, UINT, ULLONG,
  SHORT, USHORT, CHAR, UCHAR,
  MAXSTATE
};

size_t _format_parser_int(ReadWriteInfo *rw, const char *fmt, va_list *ap);

static const unsigned char states[5][35] = { /*'z'-'X'+1*/
 { /* 0: bare types */
  UINT,
  0,       0,       0,       0,       0,       0,       0,       0,       
  0,       0,       CHAR,    INT,     0,       0,       0,       HPRE,    
  INT,     LLPRE,/*j*/0,     LPRE,    0,       PTR,     UINT,    ULONG,   
  0,       0,       PTR,     LPRE/*t*/,UINT,   0,       0,       UINT,    
  0,       LPRE/*z*/
 },
 { /* 1: l-prefixed */
  ULONG,   
  0,       0,       0,       0,       0,       0,       0,       0,       
  0,       0,       0,/*c*/  LONG,    0,       0,       0,       0,       
  LONG,    0,       0,       LLPRE,   0,       PTR,     ULONG,   0,       
  0,       0,       0,/*s*/  0,       ULONG,   0,       0,       ULONG,   
  0,       0
 },
 { /* 2: ll-prefixed */
  ULLONG,  
  0,       0,       0,       0,       0,       0,       0,       0,       
  0,       0,       0,       ULLONG,  0,       0,       0,       0,       
  ULLONG,  0,       0,       0,       0,       PTR,     ULLONG,  ULLONG,  
  0,       0,       0,       0,       ULLONG,  0,       0,       ULLONG,  
  0,       0
 },
 { /* 3: h-prefixed */
  USHORT,  
  0,       0,       0,       0,       0,       0,       0,       0,       
  0,       0,       0,       SHORT,   0,       0,       0,       HHPRE,   
  SHORT,   0,       0,       0,       0,       PTR,     USHORT,  0,       
  0,       0,       0,       0,       USHORT,  0,       0,       USHORT,  
  0,       0
 },
 { /* 4: hh-prefixed */
  UCHAR,   
  0,       0,       0,       0,       0,       0,       0,       0,       
  0,       0,       0,       CHAR,    0,       0,       0,       0,       
  CHAR,    0,       0,       0,       0,       PTR,     UCHAR,   0,       
  0,       0,       0,       0,       UCHAR,   0,       0,       UCHAR,   
  0,       0
 },
};


static const char xdigits[16] = 
{
  "0123456789ABCDEF"
};

union arg
{
  UINT64 i;
  double f;
  void *p;
};

static void pop_arg(union arg *arg, int type, va_list *ap)
{
  switch (type) 
   {
    case PTR:	arg->p = va_arg(*ap, void *); break; 
    case INT:	arg->i = va_arg(*ap, int); break; 
    case UINT:	arg->i = va_arg(*ap, unsigned int); break; 
    case ULLONG:	arg->i = va_arg(*ap, UINT64); break; 
    case SHORT:	arg->i = (short)va_arg(*ap, int); break; 
    case USHORT:	arg->i = (unsigned short)va_arg(*ap, int); break; 
    case CHAR:	arg->i = (signed char)va_arg(*ap, int); break; 
    case UCHAR:	arg->i = (unsigned char)va_arg(*ap, int);
   }
   return;
}

static void pad(ReadWriteInfo *rw, char c, int width, int len, int flags)
{
  char pad[256];
  int (*fnptr)(ReadWriteInfo *, void *, size_t); 
  
  if ((flags & (LEFT_ADJ | ZERO_PAD)) || (len >= width)) 
   return;

  fnptr = rw->m_fnptr;

  len = width - len;
  memset(pad, c, len> sizeof(pad) ? sizeof(pad) : len);

  for (; len >= sizeof(pad); len -= sizeof(pad))
   fnptr(rw, pad, sizeof(pad));

  fnptr(rw, pad, len);

  return;
}

static char *fmt_x(UINT64 x, char *s, int lower)
{
  for (; x; x>>=4) *--s = xdigits[(x&15)]|lower;
  return s;
}

static char *fmt_o(UINT64 x, char *s)
{
  for (; x; x>>=3) *--s = '0' + (x&7);
  return s;
}

static char *fmt_u(UINT64 x, char *s)
{
	UINT64 temp1, temp2;

	while(x !=0)
	 {
	  temp1 = x;
	  x = x >> 2;
	  x = temp1 - x;
	
	  temp2 = x >> 4;
	  x = temp2 +x;
	  temp2 = x >> 8;
	  x = temp2 + x;
	  temp2 = x >> 16;
	  x = temp2 + x;

	  x = x >> 3;
	  temp2 = x << 2;
	  temp2 = temp2+ x;
	  temp2 = temp2 << 1;
	  temp1= temp1- temp2;
	  while (temp1 > 9)
	   {
	    temp1 = temp1 - 10;
	    x = x+1;
	   }
	  *--s ='0'+temp1;
     }
	  return s;
}

static int getint(char **s) 
{
  int i;
  for (i=0; isdigit((int)**s); (*s)++)
   i = 10*i + (**s-'0');
  return i;
}

size_t _format_parser_int(ReadWriteInfo *rw, const char *fmt, va_list *ap)
{
  union arg arg;
  const char *prefix;
  char buf[40];
  char *a, *z, *s=(char*)fmt;
  unsigned int st, ps = 0, n, flags;
  int width, p, t, pl, count=0, len=0;
  int (*fnptr)(ReadWriteInfo *, const void *, size_t); 

  fnptr = rw->m_fnptr;

  for(;;)
   {
    /* update output count */
    count += len;

    /* end loop when fmt is exhausted */
    if (!*s) 
     break;

    /* literal text and %% format specifiers */
    for (a=s; *s && *s!='%'; s++);

    n = strspn(s, "%");
    n = n >> 1;
    z = s+n;
    s += 2*n;
    len = z-a;

    if (len!=0) 
     {
       fnptr(rw, a, len);
       continue;
     }    

    /* consume % */
    s++;

    /* skip positional arguments */
  /*  if ((isdigit((int)s[0]) && s[1]=='$'))
    {
      s+=3;
      continue;
    } */

    /* read modifier flags */
    for (flags=0; ((unsigned int)*s-' ')<32 && (FLAGMASK&(1U<<(*s-' '))); s++)
     flags |= 1U<<(*s-' ');
    
    /* read field width */
    if (*s=='*') 
    {
      width = va_arg(*ap, int);
      s++;

      if (width<0) 
       {
        flags |= LEFT_ADJ; 
        width = -width;
       }
    }
    else 
    if ((width=getint(&s))<0) 
     continue;

    /* read precision */
    if (*s=='.' && s[1]=='*') 
    {
      p = va_arg(*ap, int);
      s += 2;
    }
    else 
    if (*s=='.') 
    {
     s++;
     p = getint(&s);
    } 
    else 
     p = -1;

    /* format specifier state machine */
    st=0;
    do 
    {
        if (((unsigned int)(*s) > 'z') || ((unsigned int)(*s) >= 'A' && (unsigned int)(*s) < 'X')) 
         break;
    
        ps=st;
        st=states[st][*s-'X'];
    
        if (st!=0) 
         s++;
    } while (st-1<STOP);

    if (!st)
        continue;

    /* argument on stack */
    pop_arg(&arg, st, ap);
    
    z = buf + sizeof(buf);
    prefix = "-+   0X0x";
    pl = 0;
    t = s[-1];

    /* transform ls,lc -> S,C */
   // if (ps && (t&15)==3) 
  //   t&=~32;

    /* - and 0 flags are mutually exclusive */
    if (flags & LEFT_ADJ) 
        flags &= ~ZERO_PAD;

    switch(t) 
     {
      case 'n':
       switch(ps) 
        {
         case BARE: *(int *)arg.p = count; break;
         case LPRE: *(long *)arg.p = count; break;
         case LLPRE: *(INT64 *)arg.p = count; break;
         case HPRE: *(unsigned short *)arg.p = count; break;
         case HHPRE: *(unsigned char *)arg.p = count; break;
         //case ZTPRE: *(size_t *)arg.p = count; break;
         //case JPRE: *(UINT64 *)arg.p = count; break;
        }
      continue;
      case 'p':
       p = MAX(p, 2*sizeof(void*));
       t = 'x';
       flags |= ALT_FORM;
      case 'x': case 'X':
       a = fmt_x(arg.i, z, t&32);
       if (arg.i && (flags & ALT_FORM)) { prefix+=(t>>4); pl=2; }
       if (0)
        {
         case 'o':
         a = fmt_o(arg.i, z);
         //if ((flags&ALT_FORM) && arg.i) { prefix+=5; pl=1; }
         if (flags&ALT_FORM) { prefix+=5; pl=1; }
         if (!arg.i && p > 0) p--;
         if (p>=0) flags &= ~ZERO_PAD;
         p = MAX(p, z-a);
         if ((z-a) && p > z-a)
          /* Do not increase precision if not required */
          pl =0;
         break;
        } 
       if (0)
        {
         case 'd': case 'i':
          pl=1;
          if (arg.i>INTMAX_MAX) {
           arg.i=-(INT64)arg.i;
          } else if (flags & MARK_POS) {
             prefix++;
          } else if (flags & PAD_POS) {
            prefix+=2;
          } else pl=0;
         case 'u':
          a = fmt_u(arg.i, z);
        }
       if (p>=0) flags &= ~ZERO_PAD;
       if (!arg.i && !p)
        {
         a=z;
         break;
        }
       p = MAX(p, z-a + !arg.i);
       break;
      case 'c':
       *(a=z-(p=1))=(char)arg.i;
       flags &= ~ZERO_PAD;
       break;
      case 's':
       a = arg.p ? arg.p : "(null)";
       z = memchr(a, 0, p);
       if (!z) z=a+p;
       else p=z-a;
       flags &= ~ZERO_PAD;
       break;
     }

    if (p < z-a) 
        p = z-a;
        
    if (width < pl+p) 
        width = pl+p;

    pad(rw, ' ', width, pl+p, flags);
    fnptr(rw, prefix, pl);
    pad(rw, '0', width, pl+p, flags^ZERO_PAD);
    pad(rw, '0', p, z-a, 0);
    fnptr(rw, a, z-a);
    pad(rw, ' ', width, pl+p, flags^LEFT_ADJ);

    len = width;

   }/* for fmt */

   return count;
}/* format_parser */
