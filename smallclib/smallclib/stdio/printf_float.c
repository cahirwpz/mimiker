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
* 		  file : $RCSfile: printf_float.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/*****************************************************************************//**
@File         printf.c

@Title        write the formatted data to stdout

@Author        Imagination Technologies Ltd

@


@Platform     Any

@Synopsis     #include <stdio.h>
              int printf(const char *format, arg, ...)

@Description  <<printf>> accepts a series of arguments, applies to each a
              format specifier from <<*<[format]>>>, and writes the
              formatted data to <<stdout>>, without a terminating NUL
              character.  The behavior of <<printf>> is undefined if there
              are not enough arguments for the format.  <<printf>> returns
              when it reaches the end of the format string.  If there are
              more arguments than the format requires, excess arguments are
              ignored.

@DocVer       1.0 1st Release
********************************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <low/_stdio.h>

int printf_float (const char *fmt, ...)
{
    int ret;
    ReadWriteInfo rw;
    va_list ap;
    va_start(ap, fmt);
    rw.m_fnptr = __low_printf;
    ret = _format_parser_float (&rw, fmt, &ap);
    va_end(ap);
    return ret;
}
