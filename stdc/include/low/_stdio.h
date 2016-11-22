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
*                 file : $RCSfile: _stdio.h,v $ 
*               author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/*****************************************************************************//**
Description  Standard IO structures, macros used in the SmallLib
********************************************************************************/

#ifndef _LOW_STDIO_H_
#define _LOW_STDIO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <low/_flavour.h>
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>

#define STD_IO_BUFSIZ  128           /* for stdio and stdout buffer */
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX_UBUF 4                   /* max length of unget buffer */

#define FLAG_INTONLY (1 << 0)             /* for integer only versions of formatted IO */
#define FLAG_UNGETC  (1 << 1)             /* used in scanf parser to allow ungetc */

typedef struct 
{
  unsigned char *_base;               /* IO buffer */
  unsigned char *_cptr;               /* current position in the buffer */
  short _flags;                       /* flags */
  short _fileid;                      /* file ID */
  int _bsize;                         /* size of the buffer (BUFSIZ or user defined) */
  int _rsize;                         /* read size remaining (0 when __SWR) */
  int _wsize;                         /* write size remaining (0 when __SRD) */
  int _offset;                        /* physical offset */
  unsigned char _ungetbuf[MAX_UBUF];  /* ungetc buffer */
  unsigned char *_ucptr;              /* saved _cptr */
  int _ursize;                        /* unget buffer remaining */
  void *_cookie;                      /* for funopen() function */
  char *_tmpfname;                    /* name of the temporary file */
  _mbstate_t _mbstate;                /* multi-byte state */
  short _flags2;                      /* for wide/byte orientation */
  unsigned char _nbuf;                /* used when stream is un-buffered */
} __smFILE;

#define __INIT_FILE_PTR(FP)           \
  {                                   \
    FP->_rsize = 0;                   \
    FP->_wsize = 0;                   \
    FP->_offset = 0;                  \
    FP->_ucptr = 0;                   \
    FP->_cookie = 0;                  \
    FP->_flags2 = 0;                  \
    FP->_tmpfname = 0;                \
    FP->_mbstate.__count = 0;         \
    FP->_mbstate.__value.__wch = 0;   \
  }

typedef int (*funread) (void *_cookie, char *_buf, int _n);
typedef int (*funwrite) (void *_cookie, const char *_buf, int _n);
typedef fpos_t (*funseek) (void *_cookie, fpos_t _off, int _whence);
typedef int (*funclose) (void *_cookie);

typedef struct funcookie 
{
  void *cookie;
  funread readfn;
  funwrite writefn;
  funseek seekfn;
  funclose closefn;
} funcookie;

typedef struct __read_write_info
{
    void *m_fnptr;                    /* function used for performing operation */ 
    void *m_handle;                   /* where to perform the operation */
    size_t m_size;
    size_t m_limit;
} ReadWriteInfo;


#ifdef __STATIC_IO_STREAMS__
extern __smFILE __io_streams[FOPEN_MAX - 3];
#else

typedef struct __stlist
{
  __smFILE *fp;
  struct __stlist *next;
} __stream_list;

extern __stream_list *__io_streams;
#endif

/* Local functions used for IO */
int __parse_mode (const char *, int *);
__smFILE *__find_fp (void);
int canwrite (__smFILE *);
int canread (__smFILE *);
int __refill (__smFILE *);

int low_read (__smFILE *fp, char* base, int size);
int low_write (__smFILE *fp, char* base, int size);
_fpos_t low_seek (__smFILE *fp, int off, int dir);
int low_close (__smFILE *fp);

int __low_printf(ReadWriteInfo *rw, const void *src, size_t len);
int __low_fprintf(ReadWriteInfo *rw, const void *src, size_t len);
int __low_scanf(ReadWriteInfo *rw);
int __low_sscanf(ReadWriteInfo *rw);
int __low_sprintf(ReadWriteInfo *rw, const void *src, size_t len);
int __low_snprintf(ReadWriteInfo *rw, const void *src, size_t len);
int __low_fscanf(ReadWriteInfo *rw);
int __low_asprintf (ReadWriteInfo *rw, const void *src, size_t len);
int __low_asnprintf (ReadWriteInfo *rw, const void *src, size_t len);
int __low_wprintf (ReadWriteInfo *rw, const wchar_t *s, size_t len);
int __low_swprintf (ReadWriteInfo *rw, const void *src, size_t len);
wint_t  __low_wscanf (ReadWriteInfo *rw);
wint_t __low_swscanf(ReadWriteInfo *rw);
wint_t  __low_fwscanf (ReadWriteInfo *rw);
size_t _format_parser(ReadWriteInfo *rw, const char *fmt, va_list *ap, unsigned int integeronly);
size_t _wformat_parser(ReadWriteInfo *rw, const wchar_t *fmt, va_list *ap);
int __scanf_core (ReadWriteInfo *rw, const char *fmt, va_list ap, unsigned int flags);
int __wscanf_core(ReadWriteInfo *rw, const wchar_t *fmt, va_list ap, unsigned int flags);

void * __local_memcpy (void *dst0, const void *src0, size_t len0);
size_t _format_parser_float (ReadWriteInfo *rw, const char *fmt, va_list *ap);
size_t _format_parser_int (ReadWriteInfo *rw, const char *fmt, va_list *ap);
int __scanf_core_float (ReadWriteInfo *rw, const char *fmt, va_list ap, unsigned int flags);
int __scanf_core_int (ReadWriteInfo *rw, const char *fmt, va_list ap, unsigned int flags);

extern FILE *__stdin;
extern FILE *__stdout;
extern FILE *__stderr;

#ifdef __cplusplus
}
#endif

#endif /* _LOW_STDIO_H_ */


