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
* 		  file : $RCSfile: iostreams.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/**************************************************************************//**
@File         iostreams.c

@Title        Standard and user defined IO streams

@Author        Imagination Technologies Ltd

@


@Platform     Any

@Synopsis   

@Description  The file contains definition of stdin, stdout and stderr 
              streams. Macro __STATIC_IO_STREAMS__ enables the static 
              IO streams (max FOPEN_MAX - 3 streams can be opened). 
              Otherwise streams are allocated dynamically.

@DocVer       1.0 1st Release
******************************************************************************/

#include <stdio.h>
#include <low/_stdio.h>

static unsigned char __stdin_buffer[STD_IO_BUFSIZ] = {0,};
static unsigned char __stdout_buffer[STD_IO_BUFSIZ] = {0,};

static __smFILE __local_stdin =  /* stdin */
{
  __stdin_buffer,       /* _base */   
  __stdin_buffer,       /* _cptr */
  __SRD,                /* _flags -> buffered and read only */
  0,                    /* _fileid */
  STD_IO_BUFSIZ,        /* _bsize */
  0,                    /* _rsize */
  0,
};
  
static __smFILE __local_stdout =  /* stdout */
{
  __stdout_buffer,       /* _base */   
  __stdout_buffer,       /* _cptr */
  (__SLBF | __SWR),      /* _flags -> line buffered and write only */
  1,                     /* _fileid */
  STD_IO_BUFSIZ,         /* _bsize */
  0,                     /* _rsize */
  STD_IO_BUFSIZ,         /* _wsize */
  0,
};

static __smFILE __local_stderr =  /* stderr */
{
  &__local_stderr._nbuf,  /* _base */   
  &__local_stderr._nbuf,  /* _cptr */
  (__SNBF | __SWR),       /* _flags -> unbuffered and write only */
  2,                      /* _fileid */
  1,                      /* _bsize */
  0,
}; 

FILE *__stdin = (FILE*) &__local_stdin;
FILE *__stdout = (FILE*) &__local_stdout;
FILE *__stderr = (FILE*) &__local_stderr;

#ifdef __STATIC_IO_STREAMS__

__smFILE __io_streams[FOPEN_MAX - 3] = 
{
  {NULL, NULL, 0, },                   /* user defined streams */
};

#else

__stream_list *__io_streams = NULL;

#endif /* __STATIC_IO_STREAMS__ */

/* Dummy reent structure */
struct _mips_clib_reent
{
  int _errno;
  FILE *_stdin;
  FILE *_stdout;
  FILE *_stderr;
};

static struct _mips_clib_reent const _mips_clib_impure_ptr __attribute__((__section__(".sdata"))) =
{
  0,
  (FILE*) &__local_stdin,
  (FILE*) &__local_stdout,
  (FILE*) &__local_stderr
};

struct _reent *_impure_ptr __attribute__((__section__(".sdata"))) = (struct _reent *) &_mips_clib_impure_ptr;
struct _reent * const _global_impure_ptr __attribute__((__section__(".sdata"))) = (struct _reent *) &_mips_clib_impure_ptr;

