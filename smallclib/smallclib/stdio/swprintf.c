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
* 		  file : $RCSfile: swprintf.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/*****************************************************************************//**
@File         swprintf.c

@Title        write wide character formatted data to the char buffer

@Author        Imagination Technologies Ltd

@


@Platform     Any

@Synopsis     #include <stdio.h>
              int swprintf(wchar_t *str, size_t size, const wchar_t *fmt, ...)

@Description  <<swprintf>> is like <<wprintf>>, except that output is directed to
              the buffer <[str]> and a terminating NUL is output. And the 
              resulting string length is limited to at most <[size]> wide 
              characters, including the terminating <<NUL>>. The behaviour is 
              undefined if the output <<*<[str]>>> overlaps with one of the 
              arguments.  Behavior is also undefined if the argument for <<%n>> 
              within <<*<[format]>>> overlaps another argument.

@DocVer       1.0 1st Release
********************************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <low/_stdio.h>

int vswprintf (wchar_t *s, size_t n, const wchar_t *fmt, va_list ap);

int swprintf (wchar_t *str, size_t size, const wchar_t *fmt, ...)
{
    int ret;
    
    va_list ap;
    va_start(ap, fmt);
    ret = vswprintf (str, size, fmt, ap);
    va_end(ap);

    return ret;
}

