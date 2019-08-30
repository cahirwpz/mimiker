/*	$NetBSD: stdio.h,v 1.97 2016/03/17 00:42:49 christos Exp $	*/

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
 *
 *	@(#)stdio.h	8.5 (Berkeley) 4/29/95
 */

#ifndef _STDIO_H_
#define _STDIO_H_

#include <sys/types.h>

typedef off_t fpos_t;

#define _FSTDIO /* Define for new stdio with functions. */

/*
 * NB: to fit things in six character monocase externals, the stdio
 * code uses the prefix `__s' for stdio objects, typically followed
 * by a three-character attempt at a mnemonic.
 */

/* stdio buffers */
struct __sbuf {
  unsigned char *_base;
  int _size;
};

/*
 * stdio state variables.
 *
 * The following always hold:
 *
 *	if (_flags&(__SLBF|__SWR)) == (__SLBF|__SWR),
 *		_lbfsize is -_bf._size, else _lbfsize is 0
 *	if _flags&__SRD, _w is 0
 *	if _flags&__SWR, _r is 0
 *
 * This ensures that the getc and putc macros (or inline functions) never
 * try to write or read from a file that is in `read' or `write' mode.
 * (Moreover, they can, and do, automatically switch from read mode to
 * write mode, and back, on "r+" and "w+" files.)
 *
 * _lbfsize is used only to make the inline line-buffered output stream
 * code as compact as possible.
 *
 * _ub, _up, and _ur are used when ungetc() pushes back more characters
 * than fit in the current _bf, or when ungetc() pushes back a character
 * that does not match the previous one in _bf.  When this happens,
 * _ub._base becomes non-nil (i.e., a stream has ungetc() data iff
 * _ub._base!=NULL) and _up and _ur save the current values of _p and _r.
 *
 * NB: see WARNING above before changing the layout of this structure!
 */
typedef struct __sFILE {
  unsigned char *_p;     /* current position in (some) buffer */
  int _r;                /* read space left for getc() */
  int _w;                /* write space left for putc() */
  unsigned short _flags; /* flags, below; this FILE is free if 0 */
  short _file;           /* fileno, if Unix descriptor, else -1 */
  struct __sbuf _bf;     /* the buffer (at least 1 byte, if !NULL) */
  int _lbfsize;          /* 0 or -_bf._size, for inline putc */

  /* operations */
  void *_cookie; /* cookie passed to io functions */
  int (*_close)(void *);
  ssize_t (*_read)(void *, void *, size_t);
  off_t (*_seek)(void *, off_t, int);
  ssize_t (*_write)(void *, const void *, size_t);

  /* file extension */
  struct __sbuf _ext;

  /* separate buffer for long sequences of ungetc() */
  unsigned char *_up; /* saved _p when _p is doing ungetc data */
  int _ur;            /* saved _r when _r is counting ungetc data */

  /* tricks to meet minimum requirements even when malloc() fails */
  unsigned char _ubuf[3]; /* guarantee an ungetc() buffer */
  unsigned char _nbuf[1]; /* guarantee a getc() buffer */

  int (*_flush)(void *);
  /* Formerly used by fgetln/fgetwln; kept for binary compatibility */
  char _lb_unused[sizeof(struct __sbuf) - sizeof(int (*)(void *))];

  /* Unix stdio files get aligned to block boundaries on fseek() */
  int _blksize;  /* stat.st_blksize (may be != _bf._size) */
  off_t _offset; /* current lseek offset */
} FILE;

__BEGIN_DECLS
extern FILE __sF[3];
__END_DECLS

#define __SLBF 0x0001 /* line buffered */
#define __SNBF 0x0002 /* unbuffered */
#define __SRD 0x0004  /* OK to read */
#define __SWR 0x0008  /* OK to write */
                      /* RD and WR are never simultaneously asserted */
#define __SRW 0x0010  /* open for reading & writing */
#define __SEOF 0x0020 /* found EOF */
#define __SERR 0x0040 /* found error */
#define __SMBF 0x0080 /* _buf is from malloc */
#define __SAPP 0x0100 /* fdopen()ed in append mode */
#define __SSTR 0x0200 /* this is an sprintf/snprintf string */
#define __SOPT 0x0400 /* do fseek() optimization */
#define __SNPT 0x0800 /* do not do fseek() optimization */
#define __SOFF 0x1000 /* set iff _offset is in fact correct */
#define __SMOD 0x2000 /* true => fgetln modified _p text */
#define __SALC 0x4000 /* allocate string space dynamically */

/*
 * The following three definitions are for ANSI C, which took them
 * from System V, which brilliantly took internal interface macros and
 * made them official arguments to setvbuf(), without renaming them.
 * Hence, these ugly _IOxxx names are *supposed* to appear in user code.
 *
 * Although numbered as their counterparts above, the implementation
 * does not rely on this.
 */
#define _IOFBF 0 /* setvbuf should set fully buffered */
#define _IOLBF 1 /* setvbuf should set line buffered */
#define _IONBF 2 /* setvbuf should set unbuffered */

#define BUFSIZ 1024 /* size of buffer used by setbuf */
#define EOF (-1)

/*
 * FOPEN_MAX is a minimum maximum, and is the number of streams that
 * stdio can provide without attempting to allocate further resources
 * (which could fail).  Do not use this for anything.
 */
/* must be == _POSIX_STREAM_MAX <limits.h> */
#define FOPEN_MAX 20      /* must be <= OPEN_MAX <sys/syslimits.h> */
#define FILENAME_MAX 1024 /* must be <= PATH_MAX <sys/syslimits.h> */

/* System V/ANSI C; this is the wrong way to do this, do *not* use these. */
#define P_tmpdir "/var/tmp/"
#define L_tmpnam 1024 /* XXX must be == PATH_MAX */
/* Always ensure that this is consistent with <limits.h> */
#ifndef TMP_MAX
#define TMP_MAX 308915776 /* Legacy */
#endif

/* Always ensure that these are consistent with <fcntl.h> and <unistd.h>! */
#ifndef SEEK_SET
#define SEEK_SET 0 /* set file offset to offset */
#endif
#ifndef SEEK_CUR
#define SEEK_CUR 1 /* set file offset to current plus offset */
#endif
#ifndef SEEK_END
#define SEEK_END 2 /* set file offset to EOF plus offset */
#endif

#define stdin (&__sF[0])
#define stdout (&__sF[1])
#define stderr (&__sF[2])

/*
 * Functions defined in ANSI C standard.
 */
__BEGIN_DECLS
void clearerr(FILE *);
int fclose(FILE *);
int feof(FILE *);
int ferror(FILE *);
int fflush(FILE *);
int fgetc(FILE *);
char *fgets(char *__restrict, int, FILE *__restrict);
FILE *fopen(const char *__restrict, const char *__restrict);
int fprintf(FILE *__restrict, const char *__restrict, ...) __printflike(2, 3);
int fputc(int, FILE *);
int fputs(const char *__restrict, FILE *__restrict);
size_t fread(void *__restrict, size_t, size_t, FILE *__restrict);
FILE *freopen(const char *__restrict, const char *__restrict, FILE *__restrict);
int fscanf(FILE *__restrict, const char *__restrict, ...) __scanflike(2, 3);
int fseek(FILE *, long, int);
long ftell(FILE *);
size_t fwrite(const void *__restrict, size_t, size_t, FILE *__restrict);
int getc(FILE *);
int getchar(void);
void perror(const char *);
int printf(const char *__restrict, ...) __printflike(1, 2);
int putc(int, FILE *);
int putchar(int);
int puts(const char *);
int remove(const char *);
void rewind(FILE *);
int scanf(const char *__restrict, ...) __scanflike(1, 2);
void setbuf(FILE *__restrict, char *__restrict);
int setvbuf(FILE *__restrict, char *__restrict, int, size_t);
int sscanf(const char *__restrict, const char *__restrict, ...)
  __scanflike(2, 3);
FILE *tmpfile(void);
int ungetc(int, FILE *);
int vfprintf(FILE *__restrict, const char *__restrict, __va_list)
  __printflike(2, 0);
int vprintf(const char *__restrict, __va_list) __printflike(1, 0);

char *gets(char *);
int sprintf(char *__restrict, const char *__restrict, ...) __printflike(2, 3);
char *tmpnam(char *);
int vsprintf(char *__restrict, const char *__restrict, __va_list)
  __printflike(2, 0);

int rename(const char *, const char *);
__END_DECLS

/*
 * IEEE Std 1003.1-90
 */
__BEGIN_DECLS
FILE *fdopen(int, const char *);
int fileno(FILE *);
__END_DECLS

/*
 * Functions defined in ISO XPG4.2, ISO C99, POSIX 1003.1-2001 or later.
 */
__BEGIN_DECLS
int snprintf(char *__restrict, size_t, const char *__restrict, ...)
  __printflike(3, 4);
int vsnprintf(char *__restrict, size_t, const char *__restrict, __va_list)
  __printflike(3, 0);
__END_DECLS

/*
 * X/Open CAE Specification Issue 5 Version 2
 */

__BEGIN_DECLS
int fseeko(FILE *, off_t, int);
off_t ftello(FILE *);
__END_DECLS

/*
 * Functions defined in ISO C99.
 */
__BEGIN_DECLS
int vscanf(const char *__restrict, __va_list) __scanflike(1, 0);
int vfscanf(FILE *__restrict, const char *__restrict, __va_list)
  __scanflike(2, 0);
int vsscanf(const char *__restrict, const char *__restrict, __va_list)
  __scanflike(2, 0);
__END_DECLS

#ifndef __LOCALE_T_DECLARED
typedef struct _locale *locale_t;
#define __LOCALE_T_DECLARED
#endif

__BEGIN_DECLS
int fprintf_l(FILE *__restrict, locale_t, const char *__restrict, ...)
  __printflike(3, 4);
int vfprintf_l(FILE *__restrict, locale_t, const char *__restrict, __va_list)
  __printflike(3, 0);
int printf_l(locale_t, const char *__restrict, ...) __printflike(2, 3);
int snprintf_l(char *__restrict, size_t, locale_t, const char *__restrict, ...)
  __printflike(4, 5);

int vsscanf_l(const char *__restrict, locale_t, const char *__restrict,
              __va_list) __scanflike(3, 0);

int snprintf_ss(char *restrict, size_t, const char *__restrict, ...)
  __printflike(3, 4);
int vsnprintf_ss(char *restrict, size_t, const char *__restrict, __va_list)
  __printflike(3, 0);
__END_DECLS

/*
 * Functions internal to the implementation.
 */
__BEGIN_DECLS
int __srget(FILE *);
int __swbuf(int, FILE *);
__END_DECLS

#define __sgetc(p) (--(p)->_r < 0 ? __srget(p) : (int)(*(p)->_p++))
#define __sputc(c, p)                                                          \
  (--(p)->_w < 0 ? (p)->_w >= (p)->_lbfsize                                    \
   ? (*(p)->_p = (c)),                                                         \
   *(p)->_p != '\n' ? (int)*(p)->_p++ : __swbuf('\n', p)                       \
   : __swbuf((int)(c), p)                                                      \
   : (*(p)->_p = (c), (int)*(p)->_p++))

#define __sfeof(p) (((p)->_flags & __SEOF) != 0)
#define __sferror(p) (((p)->_flags & __SERR) != 0)
#define __sclearerr(p)                                                         \
  ((void)((p)->_flags &= (unsigned short)~(__SERR | __SEOF)))
#define __sfileno(p) ((p)->_file == -1 ? -1 : (int)(unsigned short)(p)->_file)

#endif /* !_STDIO_H_ */
