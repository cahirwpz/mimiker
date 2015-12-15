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
* 		  file : $RCSfile: __wformat_parser.c,v $ 
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
#include <wchar.h>
#include <wctype.h>
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

enum
{
  BARE, LPRE, LLPRE, HPRE, HHPRE, BIGLPRE,
  ZTPRE, JPRE,
  STOP,
  PTR, INT, UINT, ULLONG,
  SHORT, USHORT, CHAR, UCHAR,
  DBL, LDBL,
  MAXSTATE
};

static const unsigned char states[8][58] = { /*'z'-'A'+1*/
 {
		DBL,     0,       INT,     0,       DBL,     DBL,     DBL,     0,       
		0,       0,       0,       BIGLPRE, 0,       0,       0,       0,       
		0,       0,       PTR,     0,       0,       0,       0,       UINT,    
		0,       0,       0,       0,       0,       0,       0,       0,       
		DBL,     0,       CHAR,    INT,     DBL,     DBL,     DBL,     HPRE,    
		INT,     JPRE,    0,       LPRE,    0,       PTR,     UINT,    ULONG,   
		0,       0,       PTR,     ZTPRE,   UINT,    0,       0,       UINT,    
		0,       ZTPRE
	},
	{
		0,       0,       0,       0,       0,       0,       0,       0,       
		0,       0,       0,       0,       0,       0,       0,       0,       
		0,       0,       0,       0,       0,       0,       0,       ULONG,   
		0,       0,       0,       0,       0,       0,       0,       0,       
		0,       0,       INT,     LONG,    0,       0,       0,       0,       
		LONG,    0,       0,       LLPRE,   0,       PTR,     ULONG,   0,       
		0,       0,       PTR,     0,       ULONG,   0,       0,       ULONG,   
		0,       0
	},
	{
		0,       0,       0,       0,       0,       0,       0,       0,       
		0,       0,       0,       0,       0,       0,       0,       0,       
		0,       0,       0,       0,       0,       0,       0,       ULLONG,  
		0,       0,       0,       0,       0,       0,       0,       0,       
		0,       0,       0,       ULLONG,  0,       0,       0,       0,       
		ULLONG,  0,       0,       0,       0,       PTR,     ULLONG,  0,       
		0,       0,       0,       0,       ULLONG,  0,       0,       ULLONG,  
		0,       0
	},
	{
		0,       0,       0,       0,       0,       0,       0,       0,       
		0,       0,       0,       0,       0,       0,       0,       0,       
		0,       0,       0,       0,       0,       0,       0,       USHORT,  
		0,       0,       0,       0,       0,       0,       0,       0,       
		0,       0,       0,       SHORT,   0,       0,       0,       HHPRE,   
		SHORT,   0,       0,       0,       0,       PTR,     USHORT,  0,       
		0,       0,       0,       0,       USHORT,  0,       0,       USHORT,  
		0,       0
	},
	{
		0,       0,       0,       0,       0,       0,       0,       0,       
		0,       0,       0,       0,       0,       0,       0,       0,       
		0,       0,       0,       0,       0,       0,       0,       UCHAR,   
		0,       0,       0,       0,       0,       0,       0,       0,       
		0,       0,       0,       CHAR,    0,       0,       0,       0,       
		CHAR,    0,       0,       0,       0,       PTR,     UCHAR,   0,       
		0,       0,       0,       0,       UCHAR,   0,       0,       UCHAR,   
		0,       0
	},
	{
		LDBL,    0,       0,       0,       LDBL,    LDBL,    LDBL,    0,       
		0,       0,       0,       0,       0,       0,       0,       0,       
		0,       0,       0,       0,       0,       0,       0,       0,       
		0,       0,       0,       0,       0,       0,       0,       0,       
		LDBL,    0,       0,       0,       LDBL,    LDBL,    LDBL,    0,       
		0,       0,       0,       0,       0,       PTR,     0,       0,       
		0,       0,       0,       0,       0,       0,       0,       0,       
		0,       0
	},
	{
		0,       0,       0,       0,       0,       0,       0,       0,       
		0,       0,       0,       0,       0,       0,       0,       0,       
		0,       0,       0,       0,       0,       0,       0,       ULONG,   
		0,       0,       0,       0,       0,       0,       0,       0,       
		0,       0,       0,       LONG,    0,       0,       0,       0,       
		LONG,    0,       0,       0,       0,       PTR,     ULONG,   0,       
		0,       0,       0,       0,       ULONG,   0,       0,       ULONG,   
		0,       0
	},
	{
		0,       0,       0,       0,       0,       0,       0,       0,       
		0,       0,       0,       0,       0,       0,       0,       0,       
		0,       0,       0,       0,       0,       0,       0,       ULLONG,  
		0,       0,       0,       0,       0,       0,       0,       0,       
		0,       0,       0,       ULLONG,  0,       0,       0,       0,       
		ULLONG,  0,       0,       0,       0,       PTR,     ULLONG,  0,       
		0,       0,       0,       0,       ULLONG,  0,       0,       ULLONG,  
		0,       0
	},
};

static const char sizeprefix['y'-'a'] = {
['a'-'a']='L', ['e'-'a']='L', ['f'-'a']='L', ['g'-'a']='L',
['d'-'a']='l', ['i'-'a']='l', ['o'-'a']='l', ['u'-'a']='l', ['x'-'a']='l',
['p'-'a']='l'
};

union arg
{
	UINT64 i;
	double f;
	void *p;
};

size_t _wformat_parser(ReadWriteInfo *rw, const wchar_t *fmt, va_list *ap);

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

static int getint(wchar_t **s) {
	int i;
	for (i=0; iswdigit(**s); (*s)++)
		i = 10*i + (**s-'0');
	return i;
}

size_t _wformat_parser(ReadWriteInfo *rw, const wchar_t *fmt, va_list *ap)
{
  wchar_t *a, *z, *s=(wchar_t*)fmt;
  union arg arg;
  char charfmt[16];
  wchar_t wc;
  FILE *f;
  char *bs;
  unsigned int st, ps = 0, n, flags;
  int width, p, t, i, count=0, len=0;
  int (*fnptr)(ReadWriteInfo *, const void *, size_t);
  char space[256];

  fnptr = rw->m_fnptr;
  f = (FILE*) rw->m_handle;
  memset (space, ' ', 256);
  
  for(;;)
   {
    /* update output count */
    count += len;

    /* end loop when fmt is exhausted */
    if (!*s)
     break;

    /* literal text and %% format specifiers */
    for (a=s; *s && *s!=L'%'; s++);

    n = wcsspn(s, L"%");
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
    if ((iswdigit (s[0]) && s[1]=='$'))
     {
      s+=3;
      continue;
     }

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
      if (((unsigned int)(*s) > 'z'))
      break;

      ps=st;
      st=states[st][*s-'A'];
      
      if (st!=0)
      s++;
     } while (st-1<STOP);

    if (!st)
     continue;

    /* argument on stack */
    pop_arg(&arg, st, ap);

    /*z = buf + sizeof(buf);
    prefix = "-+   0X0x";
    pl = 0; */
    t = s[-1];

    /* transform ls,lc -> S,C */
    if (ps && (t&15)==3) 
        t&=~32;

    /* - and 0 flags are mutually exclusive */
    /*if (flags & LEFT_ADJ) 
        flags &= ~ZERO_PAD;*/

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
         case ZTPRE: *(size_t *)arg.p = count; break;
         case JPRE: *(UINT64 *)arg.p = count; break;
        }
       continue;
      case 'c':
       if (width>1 && !(flags&LEFT_ADJ)) fprintf(f, "%.*s", width-1, space);
       fputwc(btowc(arg.i), f);
       len = 1;
       continue;
      case 'C':
       if (width>1 && !(flags&LEFT_ADJ)) fprintf(f, "%.*s", width-1, space);
       fputwc(arg.i, f);
       len = 1;
       continue;
      case 'S':
       a = arg.p;
       z = wmemchr(a, 0, p);
       if (!z) z=a+p;
       else p=z-a;
       if (width<p) width=p;
       if (!(flags&LEFT_ADJ)) fprintf(f, "%.*s", width-p, space);
       fnptr(rw, a, p);
       if ((flags&LEFT_ADJ)) fprintf(f, "%.*s", width-p, space);
       len=width;
       continue;
      case 's':
       bs = arg.p;
       if (p<0) p = INT_MAX;
       for (i=len=0; len<p && (i=mbtowc(&wc, bs, MB_LEN_MAX))>0; bs+=i, len++);
       if (i<0) return -1;
       p=len;
       if (width<p) width=p;
       if (!(flags&LEFT_ADJ)) fprintf(f, "%.*s", width-p, space);
       bs = arg.p;
       while (len--)
        {
         i=mbtowc(&wc, bs, MB_LEN_MAX);
         bs+=i;
         fputwc(wc, f);
        }
       if ((flags&LEFT_ADJ)) fprintf(f, "%.*s", width-p, space);
       len=width;
       continue;
     }


    snprintf(charfmt, sizeof charfmt, "%%%s%s%s%s%s*.*%c",
        "#"+!(flags & ALT_FORM),
        "+"+!(flags & MARK_POS),
        "-"+!(flags & LEFT_ADJ),
        " "+!(flags & PAD_POS),
        "0"+!(flags & ZERO_PAD),
        sizeprefix[(t|32)-'a']);

    switch (t|32)
     {
      case 'd': case 'i': case 'o': case 'u': case 'x': case 'p':
       snprintf(charfmt + strlen (charfmt), sizeof charfmt, "l%c", t);
       break;
      default:
       snprintf(charfmt + strlen (charfmt), sizeof charfmt, "%c", t);
       break;
     }

    switch (t|32)
     {
      case 'a': case 'e': case 'f': case 'g':
       len = fprintf(f, charfmt, width, p, arg.f);
       break;
      case 'd': case 'i': case 'o': case 'u': case 'x': case 'p':
       len = fprintf(f, charfmt, width, p, arg.i);
       break;
     }
    }/* for fmt */

    return count;
}/* format_parser */

