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
* 		  file : $RCSfile: __format_parser_float.c,v $ 
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
  DBL, LDBL,
  MAXSTATE
};

size_t _format_parser_float(ReadWriteInfo *rw, const char *fmt, va_list *ap);

 static const unsigned char states[6][58] = { /*'z'-'A'+1*/
 { /* 0: bare types */
  DBL,     0,       INT,     0,       DBL,     DBL,     DBL,     0,       
  0,       0,       0,       BIGLPRE, 0,       0,       0,       0,       
  0,       0,       PTR,     0,       0,       0,       0,       UINT,    
  0,       0,       0,       0,       0,       0,       0,       0,       
  DBL,     0,       CHAR,    INT,     DBL,     DBL,     DBL,     HPRE,    
  INT,     LLPRE,    0,      LPRE,    0,       PTR,     UINT,    ULONG,   
  0,       0,       PTR,     LPRE,    UINT,    0,       0,       UINT,    
  0,       LPRE
 },
 { /* 1: l-prefixed */
  DBL,     0,       0,       0,       DBL,     DBL,     DBL,     0,       
  0,       0,       0,       0,       0,       0,       0,       0,       
  0,       0,       0,       0,       0,       0,       0,       ULONG,   
  0,       0,       0,       0,       0,       0,       0,       0,       
  DBL,     0,       INT,     LONG,    DBL,     DBL,     DBL,     0,       
  LONG,    0,       0,       LLPRE,   0,       PTR,     ULONG,   0,       
  0,       0,       PTR,     0,       ULONG,   0,       0,       ULONG,   
  0,       0
 },
 { /* 2: ll-prefixed */
  DBL,     0,       0,       0,       DBL,     DBL,     DBL,     0,       
  0,       0,       0,       0,       0,       0,       0,       0,       
  0,       0,       0,       0,       0,       0,       0,       ULLONG,  
  0,       0,       0,       0,       0,       0,       0,       0,       
  DBL,     0,       0,       ULLONG,  DBL,     DBL,     DBL,     0,       
  ULLONG,  0,       0,       0,       0,       PTR,     ULLONG,  ULLONG,  
  0,       0,       0,       0,       ULLONG,  0,       0,       ULLONG,  
  0,       0
 },
 { /* 3: h-prefixed */
  0,       0,       0,       0,       0,       0,       0,       0,       
  0,       0,       0,       0,       0,       0,       0,       0,       
  0,       0,       0,       0,       0,       0,       0,       USHORT,  
  0,       0,       0,       0,       0,       0,       0,       0,       
  0,       0,       0,       SHORT,   0,       0,       0,       HHPRE,   
  SHORT,   0,       0,       0,       0,       PTR,     USHORT,  0,       
  0,       0,       0,       0,       USHORT,  0,       0,       USHORT,  
  0,       0
 },
 { /* 4: hh-prefixed */
  0,       0,       0,       0,       0,       0,       0,       0,       
  0,       0,       0,       0,       0,       0,       0,       0,       
  0,       0,       0,       0,       0,       0,       0,       UCHAR,   
  0,       0,       0,       0,       0,       0,       0,       0,       
  0,       0,       0,       CHAR,    0,       0,       0,       0,       
  CHAR,    0,       0,       0,       0,       PTR,     UCHAR,   0,       
  0,       0,       0,       0,       UCHAR,   0,       0,       UCHAR,   
  0,       0
 },
 { /* 5: L-prefixed */
  LDBL,    0,       0,       0,       LDBL,    LDBL,    LDBL,    0,       
  0,       0,       0,       0,       0,       0,       0,       0,       
  0,       0,       0,       0,       0,       0,       0,       0,       
  0,       0,       0,       0,       0,       0,       0,       0,       
  LDBL,    0,       0,       0,       LDBL,    LDBL,    LDBL,    0,       
  0,       0,       0,       0,       0,       PTR,     0,       0,       
  0,       0,       0,       0,       0,       0,       0,       0,       
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
    case UCHAR:	arg->i = (unsigned char)va_arg(*ap, int); break;
    case DBL:	arg->f = va_arg(*ap, double); break; 
    case LDBL:	arg->f = va_arg(*ap, double);
   }
   return;
}
static unsigned udiv10(UINT64 *val)
{
 UINT64 temp1, temp2, x;
 x = *val;
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
  *val = x;
  return (unsigned)temp1;
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
	unsigned digit;
	while(x !=0)
	{
		digit = udiv10(&x);
		*--s ='0'+digit;
    }
  return s;
}

static int fmt_fp(ReadWriteInfo *rw, double y, int width, int p, int flags, int t)
{
  unsigned int big[120];
  unsigned int *a, *d, *r, *z;
  int e2=0, e, i, j, len;
  char buf[22];
  const char *prefix="-0X+0X 0X-0x+0x 0x";
  int pl;
  int (*fnptr)(ReadWriteInfo *, const void *, size_t);

  fnptr = rw->m_fnptr;

  /* we output only in %f format */
  t = 'f';

  pl=1;
  if (y<0 || 1/y<0)
   y=-y;
  else 
  if (flags & MARK_POS)
   prefix+=3;
  else 
  if (flags & PAD_POS)
   prefix+=6;
  else 
   prefix++, pl=0;

  if (! isfinite (y)) 
   {
    char *s = (t&32)?"inf":"INF";
    if (y!=y) s=(t&32)?"nan":"NAN", pl=0;
    pad(rw, ' ', width, 3+pl, flags&~ZERO_PAD);
    fnptr(rw, prefix, pl);
    fnptr(rw, s, 3);
    pad(rw, ' ', width, 3+pl, flags^LEFT_ADJ);
    return MAX(width, 3+pl);
   }

  y = frexp(y, &e2) * 2;
  if (y) 
   e2--;

   if (p<0) p=6;

  if (y) {
    y *= /*268435456.0; */ 0x1p28;
    e2-=28;
  }

  if (e2<0) a=r=z=big;
  else a=r=z=big+sizeof(big)/sizeof(*big) - 53 - 1;

  do
   {
    *z = (unsigned int)y;
    y = 1000000000*(y-*z++);
   } while (y);

  while (e2>0)
   {
    unsigned int carry=0;
    int sh=MIN(29,e2);
    for (d=z-1; d>=a; d--)
    {
	  unsigned i=0, mod=0, j=0, pow, digit;
      UINT64 x = ((UINT64)*d<<sh)+carry;
	  UINT64 tempx =x;
      for (i=0; i < 9; i++)
      {
		digit = udiv10(&tempx);
		if (i==0)
			mod = digit;
		else
		{
			pow =1;
			for(j=0; j<i; j++)
				pow = pow*10;
			mod = pow*digit+mod;
		}
	  }
	 *d = mod;
	 carry = (unsigned int)tempx;
    }
    if (!z[-1] && z>a) z--;
    if (carry) *--a = carry;
    e2-=sh;
   }
  while (e2<0)
   {
    unsigned int carry=0, *z2;
    int sh=MIN(9,-e2);
    for (d=a; d<z; d++) 
     {
       unsigned int rm = *d & ((1<<sh)-1);
       *d = (*d>>sh) + carry;
       carry = (1000000000>>sh) * rm;
     }
    if (!*a) a++;
    if (carry) *z++ = carry;
    /* Avoid (slow!) computation past requested precision */
    z2 = ((t|32)=='f' ? r : a) + 2 + p/9;
    z = MIN(z, z2);
    e2+=sh;
   }

  if (a<z) 
   for (i=10, e=9*(r-a); *a>=(unsigned int)i; i*=10, e++);
  else 
   e=0;

  /* Perform rounding: j is precision after the radix (possibly neg) */
  //j = p - ((t|32)!='f')*e - ((t|32)=='g' && p);
  j = p ;

  if (j < 9*(z-r-1))
   {
    unsigned int x;

    /* We avoid C's broken division of negative numbers */
    d = r + 1 + (j+9*1024)/9 - 1024;
    j += 9*1024;
    j %= 9;
    for (i=10, j++; j<9; i*=10, j++);
    x = *d % i;

    /* Are there any significant digits past j? */
    if (x || d+1!=z)
     {
       double round = CONCAT(0x1p,53);
       double small;

      if (*d/i & 1) 
          round += 2;
      if (x<(unsigned int)(i/2)) 
          small=/*0.5;*/   0x0.8p0;
      else 
      if (x==i/2 && d+1==z) 
          small= /*1.0;*/  0x1.0p0;
      else 
          small= /*1.5; */ 0x1.8p0;

      if (pl && *prefix=='-')
       {
        round*=-1; small*=-1;
       }
      *d -= x;
      /* Decide whether to round by probing round+small */
      if (round+small != round)
       {
        *d = *d + i;
        while (*d > 999999999)
         {
          *d--=0;
          (*d)++;
         }
       if (d<a) a=d;
       for (i=10, e=9*(r-a); *a>=(unsigned int)i; i*=10, e++);
       }
     }
    if (z>d+1)
     z=d+1;
    for (; !z[-1] && z>a; z--);
   }

  len = 1 + p + (p || (flags&ALT_FORM));
  if (e>0) len+=e;


  pad(rw, ' ', width, pl+len, flags);
  fnptr(rw, prefix, pl);
  pad(rw, '0', width, pl+len, flags^ZERO_PAD);

   if (a>r) a=r;
   for (d=a; d<=r; d++) {
    char *s = fmt_u(*d, buf+9);
    if (d!=a) while (s>buf) *--s='0';
    else if (s==buf+9) *--s='0';
    fnptr(rw, s, buf+9-s);
   }
   if (p || (flags&ALT_FORM)) fnptr(rw, ".", 1);
   for (; d<z && p>0; d++, p-=9) {
    char *s = fmt_u(*d, buf+9);
    while (s>buf) *--s='0';
    fnptr(rw, s, MIN(9,p));
   }
   pad(rw, '0', p+9, 9, 0);

  pad(rw, ' ', width, pl+len, flags^LEFT_ADJ);

  return MAX(width, pl+len);
}

static int getint(char **s) 
{
  int i;
  for (i=0; isdigit((int)**s); (*s)++)
   i = 10*i + (**s-'0');
  return i;
}


size_t _format_parser_float (ReadWriteInfo *rw, const char *fmt, va_list *ap)
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
        //if (((unsigned int)(*s) > 'z') || ((unsigned int)(*s) >= 'A' && (unsigned int)(*s) < 'X')) 
        if ((unsigned int)(*s) > 'z')
         break;
    
        ps=st;
        //st=states[st][*s-'X'];
        st=states[st][*s-'A'];
    
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
      case 'e': case 'f': case 'g': case 'a':
      case 'E': case 'F': case 'G': case 'A':
       len = fmt_fp(rw, arg.f, width, p, flags, t);
       continue;
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

