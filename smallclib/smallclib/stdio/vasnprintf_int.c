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
* 		  file : $RCSfile: vasnprintf_int.c,v $ 
*     		author : $Author Imagination Technologies Ltd
*    date last revised : $
*      current version : $
******************************************************************************/

/*****************************************************************************//**
@File         vasnprintf.c

@Title        write the formatted data to the char buffer

@Author        Imagination Technologies Ltd

@


@Platform     Any

@Synopsis     #include <stdio.h>
              char *vasnprintf(char *str, size_t *size, const char *fmt, va_list ap);

@Description  <<vasnprintf>> is similar to <asnprintf>>. It differ only in allowing
              its caller to pass the variable argument list as a <<va_list>> 
              object (initialized by <<va_start>>) rather than directly 
              accepting a variable number of arguments.  The caller is 
              responsible for calling <<va_end>>.

@DocVer       1.0 1st Release
********************************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <low/_stdio.h>

char *vasnprintf_int (char *str, size_t *size, const char *fmt, va_list ap)
{
  int ret;
  ReadWriteInfo rw;
  rw.m_handle = str;
  rw.m_fnptr = __low_asnprintf;
  rw.m_size = 0;
  rw.m_limit = *size;
  ret = _format_parser_int(&rw, fmt, &ap);
  
  /* Return the length of the result */
  *size = ret;
  
  /* terminate the string */
  if (rw.m_handle)
    *((char *)(rw.m_handle + rw.m_size)) = '\0';
  return (char *)rw.m_handle;
}

